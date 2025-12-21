#ifndef PARSER_HPP
#define PARSER_HPP

#include <chrono>
#include <cctype>
#include <cerrno>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <variant>
#include <functional>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <cstddef>
#include <cstdlib>

#include "flag.hpp"
#include "utils.hpp"

namespace clasp {

class Parser {
public:
    struct Options {
        bool allowUnknownFlags{false};
        bool shortFlagGrouping{true}; // -abc
        bool boolNegation{true};      // --no-foo
        bool suggestFlags{true};
        std::size_t suggestionsMinimumDistance{2};
        bool disableFlagParsing{false};
        std::function<std::string(std::string)> normalizeKey{};
    };

    Parser(int argc, char** argv, const std::vector<Flag>& flags) : Parser(argc, argv, flags, Options{}) {}

    Parser(int argc, char** argv, const std::vector<Flag>& flags, Options options) : options_(options) {
        if (options_.disableFlagParsing) {
            for (int i = 1; i < argc; ++i) positionals_.push_back(argv[i]);
            return;
        }

        // Built-ins (even if not declared on a command).
        aliases_["--help"] = "--help";
        aliases_["-h"] = "--help";
        kinds_["--help"] = Kind::Bool;
        defaults_["--help"] = "false";

        aliases_["--version"] = "--version";
        kinds_["--version"] = Kind::Bool;
        defaults_["--version"] = "false";

        // Build a minimal alias map (-m -> --message) from declared flags.
        for (const auto& f : flags) {
            registerFlag(f);
        }

        bool positionalOnly = false;
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (!positionalOnly && arg == "--") {
                positionalOnly = true;
                continue;
            }

            if (!positionalOnly && isFlagToken(arg)) {
                std::string key;
                std::string value;

                if (options_.boolNegation && arg.rfind("--no-", 0) == 0) {
                    const std::string base = std::string("--") + arg.substr(5);
                    const auto canonical = normalizeKey(base);
                    const auto kindIt = kinds_.find(canonical);
                    if (kindIt != kinds_.end() && kindIt->second == Kind::Bool) {
                        recordFlagValue(canonical, "false");
                        continue;
                    }
                }

                if (options_.shortFlagGrouping && isShortGroupToken(arg)) {
                    if (!parseShortGroup(arg, i, argc, argv)) continue;
                    continue;
                }

                // Support --k=v
                const auto eq = arg.find('=');
                if (eq != std::string::npos) {
                    key = arg.substr(0, eq);
                    value = arg.substr(eq + 1);
                } else {
                    key = arg;
                    const auto canonical = normalizeKey(key);
                    const auto kindIt = kinds_.find(canonical);
                    if (kindIt == kinds_.end() && !options_.allowUnknownFlags) {
                        failUnknownFlag(key);
                        continue;
                    }

                    const bool isBool = (kindIt != kinds_.end() && kindIt->second == Kind::Bool);
                    const bool isCount = (countKeys_.find(canonical) != countKeys_.end());
                    if (isBool) {
                        if (i + 1 < argc && isBoolLiteral(argv[i + 1])) {
                            value = argv[++i];
                        } else {
                            value = "true";
                        }
                    } else if (isCount) {
                        // pflag Count: treat as bool-like (no separate arg); each occurrence increments by 1.
                        value = "1";
                    } else {
                        const auto noOptIt = noOptDefaults_.find(canonical);
                        if (i + 1 >= argc) {
                            if (noOptIt != noOptDefaults_.end()) {
                                value = noOptIt->second;
                            } else {
                                ok_ = false;
                                error_ = "flag needs an argument: " + key;
                                continue;
                            }
                        } else {
                            const std::string_view next = argv[i + 1];
                            if (noOptIt != noOptDefaults_.end() && isFlagToken(std::string(next))) {
                                value = noOptIt->second;
                            } else {
                                value = argv[++i];
                            }
                        }
                    }
                }

                key = normalizeKey(std::move(key));
                const auto kindIt2 = kinds_.find(key);
                if (kindIt2 == kinds_.end() && !options_.allowUnknownFlags) {
                    failUnknownFlag(key);
                    continue;
                }
                if (kindIt2 != kinds_.end()) {
                    if (!normalizeValue(key, value, kindIt2->second)) continue;
                }
                recordFlagValue(key, std::move(value));
                continue;
            }

            positionals_.push_back(std::move(arg));
        }
    }

    bool hasFlag(const std::string& flag) const {
        const auto it = flagValues_.find(resolveKey(flag));
        return it != flagValues_.end() && !it->second.empty();
    }

    bool hasValue(const std::string& flag) const {
        const auto key = resolveKey(flag);
        const auto it = flagValues_.find(key);
        if (it != flagValues_.end() && !it->second.empty()) return true;
        const auto multiIt = externalMultiValues_.find(key);
        if (multiIt != externalMultiValues_.end() && !multiIt->second.empty()) return true;
        if (externalValues_.find(key) != externalValues_.end()) return true;
        return false;
    }

    void setExternalValues(std::unordered_map<std::string, std::string> values) { externalValues_ = std::move(values); }

    void setExternalValuesMulti(std::unordered_map<std::string, std::vector<std::string>> values) {
        externalMultiValues_ = std::move(values);
    }

    // Like setExternalValues, but validates values against declared flag types.
    // Returns an error string on the first invalid value.
    std::optional<std::string> setExternalValuesChecked(std::unordered_map<std::string, std::string> values) {
        std::unordered_map<std::string, std::string> normalized;
        normalized.reserve(values.size());
        for (auto& [k, v] : values) {
            const auto key = normalizeKey(k);
            const auto kindIt = kinds_.find(key);
            if (kindIt != kinds_.end()) {
                if (!normalizeValue(key, v, kindIt->second)) return error_;
            }
            normalized[key] = std::move(v);
        }
        externalValues_ = std::move(normalized);
        return std::nullopt;
    }

    std::optional<std::string> setExternalValuesMultiChecked(std::unordered_map<std::string, std::vector<std::string>> values) {
        std::unordered_map<std::string, std::vector<std::string>> normalized;
        normalized.reserve(values.size());
        for (auto& [k, vec] : values) {
            const auto key = normalizeKey(k);
            const auto kindIt = kinds_.find(key);
            if (kindIt != kinds_.end()) {
                for (auto& v : vec) {
                    if (!normalizeValue(key, v, kindIt->second)) return error_;
                }
            }
            normalized[key] = std::move(vec);
        }
        externalMultiValues_ = std::move(normalized);
        return std::nullopt;
    }

    template <typename T>
    T getFlag(const std::string& flag, T defaultValue = T()) const {
        const auto key = resolveKey(flag);
        const auto it = flagValues_.find(key);
        if (it != flagValues_.end() && !it->second.empty()) return parse<T>(it->second.back(), std::move(defaultValue));

        const auto extMultiIt = externalMultiValues_.find(key);
        if (extMultiIt != externalMultiValues_.end() && !extMultiIt->second.empty()) {
            return parse<T>(extMultiIt->second.back(), std::move(defaultValue));
        }

        const auto extIt = externalValues_.find(key);
        if (extIt != externalValues_.end()) return parse<T>(extIt->second, std::move(defaultValue));

        const auto defIt = defaults_.find(key);
        if (defIt != defaults_.end()) return parse<T>(defIt->second, std::move(defaultValue));

        return defaultValue;
    }

    // Count helper: sums all parsed integer values for this flag. Intended for Count-style flags that emit "1" per occurrence.
    int getCount(const std::string& flag, int defaultValue = 0) const {
        const auto key = resolveKey(flag);
        const auto it = flagValues_.find(key);
        if (it != flagValues_.end() && !it->second.empty()) {
            int total = 0;
            for (const auto& v : it->second) total += parse<int>(v, 0);
            return total;
        }

        const auto extMultiIt = externalMultiValues_.find(key);
        if (extMultiIt != externalMultiValues_.end() && !extMultiIt->second.empty()) {
            int total = 0;
            for (const auto& v : extMultiIt->second) total += parse<int>(v, 0);
            return total;
        }

        const auto extIt = externalValues_.find(key);
        if (extIt != externalValues_.end()) return parse<int>(extIt->second, defaultValue);

        const auto defIt = defaults_.find(key);
        if (defIt != defaults_.end()) return parse<int>(defIt->second, defaultValue);

        return defaultValue;
    }

    std::size_t occurrences(const std::string& flag) const {
        const auto key = resolveKey(flag);
        const auto it = flagValues_.find(key);
        if (it == flagValues_.end()) return 0;
        return it->second.size();
    }

    // True if the flag has a value from CLI or external sources (env/config), excluding defaults.
    bool hasExplicitValue(const std::string& flag) const {
        const auto key = resolveKey(flag);
        const auto it = flagValues_.find(key);
        if (it != flagValues_.end() && !it->second.empty()) return true;

        const auto extMultiIt = externalMultiValues_.find(key);
        if (extMultiIt != externalMultiValues_.end() && !extMultiIt->second.empty()) return true;

        const auto extIt = externalValues_.find(key);
        if (extIt != externalValues_.end()) return true;

        return false;
    }

    // Returns values from CLI or external sources (env/config), excluding defaults.
    std::vector<std::string> getExplicitFlagValues(const std::string& flag) const {
        const auto key = resolveKey(flag);
        const auto it = flagValues_.find(key);
        if (it != flagValues_.end() && !it->second.empty()) return it->second;

        const auto extMultiIt = externalMultiValues_.find(key);
        if (extMultiIt != externalMultiValues_.end() && !extMultiIt->second.empty()) return extMultiIt->second;

        const auto extIt = externalValues_.find(key);
        if (extIt != externalValues_.end()) return {extIt->second};

        return {};
    }

    std::vector<std::string> getFlagValues(const std::string& flag) const {
        const auto key = resolveKey(flag);
        const auto it = flagValues_.find(key);
        if (it != flagValues_.end() && !it->second.empty()) return it->second;

        const auto extMultiIt = externalMultiValues_.find(key);
        if (extMultiIt != externalMultiValues_.end() && !extMultiIt->second.empty()) return extMultiIt->second;

        const auto extIt = externalValues_.find(key);
        if (extIt != externalValues_.end()) return {extIt->second};

        const auto defIt = defaults_.find(key);
        if (defIt != defaults_.end()) return {defIt->second};

        return {};
    }

    std::vector<std::string> getFlagValuesSplit(const std::string& flag, char sep = ',') const {
        std::vector<std::string> out;
        for (const auto& v : getFlagValues(flag)) {
            std::size_t start = 0;
            while (start <= v.size()) {
                const auto pos = v.find(sep, start);
                const auto part = (pos == std::string::npos) ? v.substr(start) : v.substr(start, pos - start);
                if (!part.empty()) out.push_back(part);
                if (pos == std::string::npos) break;
                start = pos + 1;
            }
        }
        return out;
    }

    template <typename T>
    std::vector<T> getFlagValuesAs(const std::string& flag, T defaultValue = T()) const {
        std::vector<T> out;
        for (const auto& v : getFlagValues(flag)) {
            out.push_back(parse<T>(v, defaultValue));
        }
        return out;
    }

    std::unordered_map<std::string, std::string> getFlagMap(const std::string& flag,
                                                            char entrySep = ',',
                                                            char kvSep = '=') const {
        std::unordered_map<std::string, std::string> out;
        for (const auto& entry : getFlagValuesSplit(flag, entrySep)) {
            const auto pos = entry.find(kvSep);
            if (pos == std::string::npos) continue;
            const auto k = entry.substr(0, pos);
            const auto v = entry.substr(pos + 1);
            if (k.empty()) continue;
            out[k] = v;
        }
        return out;
    }

    // pflag-like helpers.
    // Slice: split each occurrence by `sep` and concatenate.
    std::vector<std::string> getStringSlice(const std::string& flag, char sep = ',') const { return getFlagValuesSplit(flag, sep); }
    std::vector<bool> getBoolSlice(const std::string& flag, char sep = ',') const { return getFlagValuesAsFromSplit<bool>(flag, sep); }
    std::vector<int> getIntSlice(const std::string& flag, char sep = ',') const { return getFlagValuesAsFromSplit<int>(flag, sep); }
    std::vector<std::int64_t> getInt64Slice(const std::string& flag, char sep = ',') const {
        return getFlagValuesAsFromSplit<std::int64_t>(flag, sep);
    }
    std::vector<std::uint64_t> getUint64Slice(const std::string& flag, char sep = ',') const {
        return getFlagValuesAsFromSplit<std::uint64_t>(flag, sep);
    }
    std::vector<float> getFloatSlice(const std::string& flag, char sep = ',') const { return getFlagValuesAsFromSplit<float>(flag, sep); }
    std::vector<double> getDoubleSlice(const std::string& flag, char sep = ',') const { return getFlagValuesAsFromSplit<double>(flag, sep); }
    std::vector<std::chrono::milliseconds> getDurationSlice(const std::string& flag, char sep = ',') const {
        return getFlagValuesAsFromSplit<std::chrono::milliseconds>(flag, sep);
    }

    // Array: do not split on commas; each occurrence is one element.
    // If only a scalar default exists and it's empty, treat it as an empty array.
    std::vector<std::string> getStringArray(const std::string& flag) const {
        return getArrayRaw(flag);
    }

    std::vector<bool> getBoolArray(const std::string& flag) const { return getArrayAs<bool>(flag, false); }
    std::vector<int> getIntArray(const std::string& flag) const { return getArrayAs<int>(flag, 0); }
    std::vector<std::int64_t> getInt64Array(const std::string& flag) const { return getArrayAs<std::int64_t>(flag, 0); }
    std::vector<std::uint64_t> getUint64Array(const std::string& flag) const { return getArrayAs<std::uint64_t>(flag, 0); }
    std::vector<float> getFloatArray(const std::string& flag) const { return getArrayAs<float>(flag, 0.0f); }
    std::vector<double> getDoubleArray(const std::string& flag) const { return getArrayAs<double>(flag, 0.0); }
    std::vector<std::chrono::milliseconds> getDurationArray(const std::string& flag) const {
        return getArrayAs<std::chrono::milliseconds>(flag, std::chrono::milliseconds{0});
    }

    // Map helpers: parse "k=v" entries (comma-separated across occurrences).
    std::unordered_map<std::string, std::string> getStringToString(const std::string& flag,
                                                                    char entrySep = ',',
                                                                    char kvSep = '=') const {
        return getFlagMap(flag, entrySep, kvSep);
    }

    std::unordered_map<std::string, int> getStringToInt(const std::string& flag, char entrySep = ',', char kvSep = '=') const {
        std::unordered_map<std::string, int> out;
        for (const auto& [k, v] : getFlagMap(flag, entrySep, kvSep)) {
            out[k] = parse<int>(v, 0);
        }
        return out;
    }

    std::unordered_map<std::string, std::int64_t> getStringToInt64(const std::string& flag,
                                                                   char entrySep = ',',
                                                                   char kvSep = '=') const {
        std::unordered_map<std::string, std::int64_t> out;
        for (const auto& [k, v] : getFlagMap(flag, entrySep, kvSep)) {
            out[k] = parse<std::int64_t>(v, 0);
        }
        return out;
    }

    std::unordered_map<std::string, std::uint64_t> getStringToUint64(const std::string& flag,
                                                                     char entrySep = ',',
                                                                     char kvSep = '=') const {
        std::unordered_map<std::string, std::uint64_t> out;
        for (const auto& [k, v] : getFlagMap(flag, entrySep, kvSep)) {
            out[k] = parse<std::uint64_t>(v, 0);
        }
        return out;
    }

    std::unordered_map<std::string, bool> getStringToBool(const std::string& flag, char entrySep = ',', char kvSep = '=') const {
        std::unordered_map<std::string, bool> out;
        for (const auto& [k, v] : getFlagMap(flag, entrySep, kvSep)) {
            out[k] = parse<bool>(v, false);
        }
        return out;
    }

    const std::vector<std::string>& positionals() const { return positionals_; }

    bool ok() const { return ok_; }
    const std::string& error() const { return error_; }

private:
    std::vector<std::string> getArrayRaw(const std::string& flag) const {
        const auto key = resolveKey(flag);
        const auto it = flagValues_.find(key);
        if (it != flagValues_.end() && !it->second.empty()) return it->second;

        const auto extMultiIt = externalMultiValues_.find(key);
        if (extMultiIt != externalMultiValues_.end() && !extMultiIt->second.empty()) return extMultiIt->second;

        const auto extIt = externalValues_.find(key);
        if (extIt != externalValues_.end()) return {extIt->second};

        const auto defIt = defaults_.find(key);
        if (defIt != defaults_.end()) {
            if (defIt->second.empty()) return {};
            return {defIt->second};
        }

        return {};
    }

    template <typename T>
    std::vector<T> getArrayAs(const std::string& flag, T defaultElement) const {
        std::vector<T> out;
        for (const auto& v : getArrayRaw(flag)) out.push_back(parse<T>(v, defaultElement));
        return out;
    }

    template <typename T>
    std::vector<T> getFlagValuesAsFromSplit(const std::string& flag, char sep) const {
        std::vector<T> out;
        for (const auto& v : getFlagValuesSplit(flag, sep)) {
            out.push_back(parse<T>(v, T{}));
        }
        return out;
    }

    static bool isFlagToken(const std::string& s) {
        return s.size() >= 2 && s[0] == '-' && s != "-";
    }

    static bool isShortGroupToken(const std::string& s) {
        if (s.size() < 3) return false;
        if (s.rfind("--", 0) == 0) return false;
        if (s.find('=') != std::string::npos) return false;
        return s[0] == '-' && s[1] != '-';
    }

    static bool isBoolLiteral(std::string_view s) {
        return s == "1" || s == "0" || s == "true" || s == "false" || s == "True" || s == "False" ||
               s == "TRUE" || s == "FALSE" || s == "on" || s == "off" || s == "yes" || s == "no";
    }

    static std::string_view trimWs(std::string_view s) {
        std::size_t start = 0;
        while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
        std::size_t end = s.size();
        while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
        return s.substr(start, end - start);
    }

    std::string normalizeKey(std::string k) const {
        if (options_.normalizeKey) k = options_.normalizeKey(std::move(k));
        const auto it = aliases_.find(k);
        if (it != aliases_.end()) return it->second;
        return k;
    }

    std::string resolveKey(const std::string& k) const {
        std::string key = k;
        if (options_.normalizeKey) key = options_.normalizeKey(std::move(key));
        const auto it = aliases_.find(key);
        if (it != aliases_.end()) return it->second;
        return key;
    }

    template <typename T>
    static T parse(const std::string& s, T defaultValue) {
        if constexpr (std::is_same_v<T, bool>) {
            return parseBool(s, defaultValue);
        } else if constexpr (std::is_same_v<T, std::string>) {
            return s;
        } else if constexpr (std::is_same_v<T, std::chrono::milliseconds>) {
            return parseDuration(s, defaultValue);
        } else if constexpr (std::is_integral_v<T>) {
            if constexpr (std::is_signed_v<T>) {
                T out{};
                if (!tryParseSignedInt<T>(s, out)) return defaultValue;
                return out;
            } else {
                T out{};
                if (!tryParseUnsignedInt<T>(s, out)) return defaultValue;
                return out;
            }
        } else if constexpr (std::is_floating_point_v<T>) {
            T out{};
            if (!tryParseFloat<T>(s, out)) return defaultValue;
            return out;
        } else {
            std::istringstream iss(s);
            T out{};
            if (!(iss >> out)) return defaultValue;
            return out;
        }
    }

    static bool parseBool(const std::string& s, bool defaultValue) {
        if (s.empty()) return defaultValue;
        if (s == "1" || s == "true" || s == "True" || s == "TRUE" || s == "on" || s == "yes") return true;
        if (s == "0" || s == "false" || s == "False" || s == "FALSE" || s == "off" || s == "no") return false;
        return defaultValue;
    }

    static bool tryParseBool(std::string_view s, bool& out) {
        const auto t = trimWs(s);
        if (t.empty()) return false;
        if (t == "1" || t == "true" || t == "True" || t == "TRUE" || t == "on" || t == "yes") {
            out = true;
            return true;
        }
        if (t == "0" || t == "false" || t == "False" || t == "FALSE" || t == "off" || t == "no") {
            out = false;
            return true;
        }
        return false;
    }

    template <typename T>
    static bool tryParseSignedInt(std::string_view s, T& out) {
        static_assert(std::numeric_limits<T>::is_integer && std::numeric_limits<T>::is_signed, "signed integer required");
        const auto t = trimWs(s);
        if (t.empty()) return false;
        const std::string tmp(t);
        char* end = nullptr;
        errno = 0;
        const long long v = std::strtoll(tmp.c_str(), &end, 0);
        if (errno != 0) return false;
        if (!end || static_cast<std::size_t>(end - tmp.c_str()) != tmp.size()) return false;
        if (v < static_cast<long long>(std::numeric_limits<T>::min()) || v > static_cast<long long>(std::numeric_limits<T>::max())) {
            return false;
        }
        out = static_cast<T>(v);
        return true;
    }

    template <typename T>
    static bool tryParseUnsignedInt(std::string_view s, T& out) {
        static_assert(std::numeric_limits<T>::is_integer && !std::numeric_limits<T>::is_signed, "unsigned integer required");
        const auto t = trimWs(s);
        if (t.empty()) return false;
        if (!t.empty() && t.front() == '-') return false;
        const std::string tmp(t);
        char* end = nullptr;
        errno = 0;
        const unsigned long long v = std::strtoull(tmp.c_str(), &end, 0);
        if (errno != 0) return false;
        if (!end || static_cast<std::size_t>(end - tmp.c_str()) != tmp.size()) return false;
        if (v > static_cast<unsigned long long>(std::numeric_limits<T>::max())) return false;
        out = static_cast<T>(v);
        return true;
    }

    template <typename T>
    static bool tryParseFloat(std::string_view s, T& out) {
        static_assert(std::is_floating_point_v<T>, "floating point required");
        const auto t = trimWs(s);
        if (t.empty()) return false;
        const std::string tmp(t);
        char* end = nullptr;
        errno = 0;
        if constexpr (std::is_same_v<T, float>) {
            const float v = std::strtof(tmp.c_str(), &end);
            if (errno != 0) return false;
            if (!end || static_cast<std::size_t>(end - tmp.c_str()) != tmp.size()) return false;
            out = v;
            return true;
        } else {
            const double v = std::strtod(tmp.c_str(), &end);
            if (errno != 0) return false;
            if (!end || static_cast<std::size_t>(end - tmp.c_str()) != tmp.size()) return false;
            out = static_cast<T>(v);
            return true;
        }
    }

    static bool tryParseDuration(std::string_view s, std::chrono::milliseconds& out) {
        const auto sv = trimWs(s);
        if (sv.empty()) return false;

        std::size_t pos = 0;
        int sign = 1;
        if (sv[pos] == '+' || sv[pos] == '-') {
            if (sv[pos] == '-') sign = -1;
            ++pos;
        }
        if (pos >= sv.size()) return false;

        // Go-style: unitless "0" is allowed.
        if (sv.substr(pos) == "0") {
            out = std::chrono::milliseconds(0);
            return true;
        }

        double totalMs = 0.0;
        while (pos < sv.size()) {
            const std::size_t numStart = pos;
            bool seenDigit = false;
            bool seenDot = false;
            for (; pos < sv.size(); ++pos) {
                const char ch = sv[pos];
                if (std::isdigit(static_cast<unsigned char>(ch))) {
                    seenDigit = true;
                    continue;
                }
                if (ch == '.' && !seenDot) {
                    seenDot = true;
                    continue;
                }
                break;
            }
            if (!seenDigit) return false;
            const std::size_t numEnd = pos;
            if (pos >= sv.size()) return false; // unit required

            double multiplier = 0.0; // ms
            std::string_view unit;
            const auto rest = sv.substr(pos);
            if (rest.rfind("ns", 0) == 0) {
                unit = "ns";
                multiplier = 0.000001;
            } else if (rest.rfind("us", 0) == 0) {
                unit = "us";
                multiplier = 0.001;
            } else if (rest.rfind("µs", 0) == 0) {
                unit = "µs";
                multiplier = 0.001;
            } else if (rest.rfind("ms", 0) == 0) {
                unit = "ms";
                multiplier = 1.0;
            } else if (rest.rfind("s", 0) == 0) {
                unit = "s";
                multiplier = 1000.0;
            } else if (rest.rfind("m", 0) == 0) {
                unit = "m";
                multiplier = 60.0 * 1000.0;
            } else if (rest.rfind("h", 0) == 0) {
                unit = "h";
                multiplier = 60.0 * 60.0 * 1000.0;
            } else {
                return false;
            }

            double value = 0.0;
            try {
                value = std::stod(std::string(sv.substr(numStart, numEnd - numStart)));
            } catch (...) {
                return false;
            }

            totalMs += value * multiplier;
            pos += unit.size();
        }

        totalMs *= static_cast<double>(sign);
        if (totalMs > static_cast<double>(std::numeric_limits<std::int64_t>::max())) return false;
        if (totalMs < static_cast<double>(std::numeric_limits<std::int64_t>::min())) return false;
        const auto asInt = static_cast<std::int64_t>(totalMs >= 0 ? (totalMs + 0.5) : (totalMs - 0.5));
        out = std::chrono::milliseconds(asInt);
        return true;
    }

    static std::chrono::milliseconds parseDuration(const std::string& s, std::chrono::milliseconds defaultValue) {
        std::chrono::milliseconds out{};
        if (!tryParseDuration(s, out)) return defaultValue;
        return out;
    }

    enum class Kind { Bool, Int, Int64, Uint64, Float, Double, Duration, String, Unknown };

    static Kind kindFromDefault(const FlagValue& v) {
        switch (v.index()) {
        case 0: return Kind::Bool;
        case 1: return Kind::Int;    // int
        case 2: return Kind::Int64;  // int64
        case 3: return Kind::Uint64; // uint64
        case 4: return Kind::Float;  // float
        case 5: return Kind::Double; // double
        case 6: return Kind::Duration; // duration
        case 7: return Kind::String; // std::string
        default: return Kind::Unknown;
        }
    }

    static bool isTruthyAnnotation(std::string_view v) {
        return v == "1" || v == "true" || v == "True" || v == "TRUE" || v == "yes" || v == "on";
    }

    static bool tryParseBytes(std::string_view s, std::uint64_t& out) {
        auto toUpperAscii = [](std::string_view v) {
            std::string out;
            out.reserve(v.size());
            for (const auto ch : v) out.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
            return out;
        };

        const auto sv = trimWs(s);
        if (sv.empty()) return false;
        if (sv.front() == '-') return false;

        // First, allow base-0 integer forms (e.g. 0x10, 077) with no unit.
        std::uint64_t parsed{};
        if (tryParseUnsignedInt<std::uint64_t>(sv, parsed)) {
            out = parsed;
            return true;
        }

        const std::string tmp(sv);
        const char* start = tmp.c_str();
        char* end = nullptr;
        errno = 0;
        const double value = std::strtod(start, &end);
        if (errno != 0) return false;
        if (!end || end == start) return false;

        std::string_view rest(start + (end - start));
        rest = trimWs(rest);

        std::uint64_t multiplier = 1;
        if (!rest.empty()) {
            const std::string unit = toUpperAscii(rest);
            if (unit == "B") {
                multiplier = 1ULL;
            } else if (unit == "K" || unit == "KB" || unit == "KI" || unit == "KIB") {
                multiplier = 1024ULL;
            } else if (unit == "M" || unit == "MB" || unit == "MI" || unit == "MIB") {
                multiplier = 1024ULL * 1024ULL;
            } else if (unit == "G" || unit == "GB" || unit == "GI" || unit == "GIB") {
                multiplier = 1024ULL * 1024ULL * 1024ULL;
            } else if (unit == "T" || unit == "TB" || unit == "TI" || unit == "TIB") {
                multiplier = 1024ULL * 1024ULL * 1024ULL * 1024ULL;
            } else if (unit == "P" || unit == "PB" || unit == "PI" || unit == "PIB") {
                multiplier = 1024ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL;
            } else if (unit == "E" || unit == "EB" || unit == "EI" || unit == "EIB") {
                multiplier = 1024ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL;
            } else {
                return false;
            }
        }

        const long double bytes = static_cast<long double>(value) * static_cast<long double>(multiplier);
        if (!(bytes >= 0.0L)) return false;
        if (bytes > static_cast<long double>(std::numeric_limits<std::uint64_t>::max())) return false;
        const auto asInt = static_cast<std::uint64_t>(bytes + 0.5L);
        out = asInt;
        return true;
    }

    bool normalizeValue(const std::string& key, std::string& value, Kind kind) {
        if (!ok_) return false;
        const std::string original = value;
        bool valid = true;
        switch (kind) {
        case Kind::String:
            valid = true;
            break;
        case Kind::Bool: {
            bool parsed{};
            valid = tryParseBool(value, parsed);
            if (valid) value = parsed ? "true" : "false";
            break;
        }
        case Kind::Int: {
            int parsed{};
            valid = tryParseSignedInt<int>(value, parsed);
            break;
        }
        case Kind::Int64: {
            std::int64_t parsed{};
            valid = tryParseSignedInt<std::int64_t>(value, parsed);
            break;
        }
        case Kind::Uint64: {
            std::uint64_t parsed{};
            if (bytesKeys_.find(key) != bytesKeys_.end()) {
                valid = tryParseBytes(value, parsed);
                if (valid) value = std::to_string(parsed);
            } else {
                valid = tryParseUnsignedInt<std::uint64_t>(value, parsed);
            }
            break;
        }
        case Kind::Float: {
            float parsed{};
            valid = tryParseFloat<float>(value, parsed);
            break;
        }
        case Kind::Double: {
            double parsed{};
            valid = tryParseFloat<double>(value, parsed);
            break;
        }
        case Kind::Duration: {
            std::chrono::milliseconds parsed{};
            valid = tryParseDuration(value, parsed);
            break;
        }
        default:
            valid = true;
            break;
        }

        if (!valid) {
            ok_ = false;
            error_ = "invalid argument \"" + original + "\" for \"" + key + "\"";
            return false;
        }
        return true;
    }

    static std::string toString(const FlagValue& v) {
        return std::visit(
            [](const auto& x) -> std::string {
                using T = std::decay_t<decltype(x)>;
                if constexpr (std::is_same_v<T, bool>) return x ? "true" : "false";
                if constexpr (std::is_same_v<T, int>) return std::to_string(x);
                if constexpr (std::is_same_v<T, std::int64_t>) return std::to_string(x);
                if constexpr (std::is_same_v<T, std::uint64_t>) return std::to_string(x);
                if constexpr (std::is_same_v<T, float>) return std::to_string(x);
                if constexpr (std::is_same_v<T, double>) return std::to_string(x);
                if constexpr (std::is_same_v<T, std::chrono::milliseconds>) return std::to_string(x.count()) + "ms";
                if constexpr (std::is_same_v<T, std::string>) return x;
                return {};
            },
            v);
    }

    void registerFlag(const Flag& f) {
        if (!f.longName().empty()) {
            aliases_[f.longName()] = f.longName();
            kinds_[f.longName()] = kindFromDefault(f.defaultValue());
            defaults_[f.longName()] = toString(f.defaultValue());
            if (f.noOptDefaultValue().has_value()) noOptDefaults_[f.longName()] = *f.noOptDefaultValue();
            const auto it = f.annotations().find("count");
            if (it != f.annotations().end() && (it->second == "true" || it->second == "1" || it->second == "yes")) {
                countKeys_.insert(f.longName());
            }
            const auto bytesIt = f.annotations().find("bytes");
            if (bytesIt != f.annotations().end() && isTruthyAnnotation(bytesIt->second)) {
                bytesKeys_.insert(f.longName());
            }
            knownKeys_.push_back(f.longName());
        }
        if (!f.shortName().empty() && !f.longName().empty()) {
            aliases_[f.shortName()] = f.longName();
            knownKeys_.push_back(f.shortName());
        }
    }

    void recordFlagValue(const std::string& canonicalKey, std::string value) {
        flagValues_[canonicalKey].push_back(std::move(value));
    }

    bool parseShortGroup(const std::string& group, int& i, int argc, char** argv) {
        for (std::size_t pos = 1; pos < group.size(); ++pos) {
            const std::string key = std::string("-") + group[pos];
            const auto canonical = normalizeKey(key);
            const auto kindIt = kinds_.find(canonical);
            if (kindIt == kinds_.end() && !options_.allowUnknownFlags) {
                failUnknownFlag(key);
                return false;
            }

            if (kindIt != kinds_.end() && kindIt->second == Kind::Bool) {
                recordFlagValue(canonical, "true");
                continue;
            }

            const bool isCount = (countKeys_.find(canonical) != countKeys_.end());
            if (isCount) {
                // Support -vvv as three occurrences, and -v3 as an explicit increment amount.
                if (pos + 1 < group.size()) {
                    const auto remainder = group.substr(pos + 1);
                    bool numeric = true;
                    for (std::size_t j = 0; j < remainder.size(); ++j) {
                        const char ch = remainder[j];
                        if (j == 0 && (ch == '+' || ch == '-')) continue;
                        if (ch < '0' || ch > '9') {
                            numeric = false;
                            break;
                        }
                    }
                    if (!remainder.empty() && numeric) {
                        auto v = remainder;
                        if (kindIt != kinds_.end() && !normalizeValue(canonical, v, kindIt->second)) return false;
                        recordFlagValue(canonical, std::move(v));
                        return true;
                    }
                }
                recordFlagValue(canonical, "1");
                continue;
            }

            // Needs a value: -ovalue OR -o value
            std::string value;
            if (pos + 1 < group.size()) {
                value = group.substr(pos + 1);
            } else {
                const auto noOptIt = noOptDefaults_.find(canonical);
                if (i + 1 >= argc) {
                    if (noOptIt != noOptDefaults_.end()) {
                        value = noOptIt->second;
                    } else {
                        ok_ = false;
                        error_ = "flag needs an argument: " + key;
                        return false;
                    }
                } else {
                    const std::string_view next = argv[i + 1];
                    if (noOptIt != noOptDefaults_.end() && isFlagToken(std::string(next))) {
                        value = noOptIt->second;
                    } else {
                        value = argv[++i];
                    }
                }
            }
            if (kindIt != kinds_.end() && !normalizeValue(canonical, value, kindIt->second)) return false;
            recordFlagValue(canonical, std::move(value));
            return true;
        }
        return true;
    }

    void failUnknownFlag(const std::string& key) {
        ok_ = false;
        error_ = "unknown flag: " + key;
        if (!options_.suggestFlags || knownKeys_.empty()) return;
        const auto suggestions = utils::suggest(key, knownKeys_, /*maxResults=*/3, options_.suggestionsMinimumDistance);
        if (suggestions.empty()) return;
        error_ += "\n\nDid you mean this?\n";
        for (const auto& s : suggestions) error_ += "  " + s + "\n";
    }

    std::unordered_map<std::string, std::vector<std::string>> flagValues_;
    std::unordered_map<std::string, std::vector<std::string>> externalMultiValues_;
    std::unordered_map<std::string, std::string> externalValues_;
    std::unordered_map<std::string, std::string> aliases_;
    std::unordered_map<std::string, Kind> kinds_;
    std::unordered_map<std::string, std::string> defaults_;
    std::unordered_map<std::string, std::string> noOptDefaults_;
    std::unordered_set<std::string> countKeys_;
    std::unordered_set<std::string> bytesKeys_;
    std::vector<std::string> knownKeys_;
    std::vector<std::string> positionals_;
    bool ok_{true};
    std::string error_;
    Options options_;
};

} // namespace clasp

#endif // PARSER_HPP
