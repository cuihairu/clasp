#ifndef PARSER_HPP
#define PARSER_HPP

#include <array>
#include <chrono>
#include <cctype>
#include <cerrno>
#include <cstdint>
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

#include "detail/value_parse.hpp"
#include "detail/parser_value.hpp"
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
        // An invalid flag declaration (e.g. a non-base64 bytesBase64 default)
        // was detected during registration: keep that error, skip parsing.
        if (!ok_) return;

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

                // Support --k=v and -k=v (short-form with equals).
                const auto eq = arg.find('=');
                if (eq != std::string::npos) {
                    key = arg.substr(0, eq);
                    value = arg.substr(eq + 1);
                    const auto canonical = normalizeKey(key);
                    const auto kindIt = kinds_.find(canonical);
                    if (kindIt == kinds_.end()) {
                        if (!options_.allowUnknownFlags) failUnknownFlag(key);
                        continue; // ignore unknown flag and its "=value"
                    }
                    if (!normalizeValue(canonical, value, kindIt->second)) continue;
                    recordFlagValue(canonical, std::move(value));
                    continue;
                }

                if (options_.shortFlagGrouping && isShortGroupToken(arg)) {
                    if (!parseShortGroup(arg, i, argc, argv)) continue;
                    continue;
                }

                key = arg;
                const auto canonical = normalizeKey(key);
                const auto kindIt = kinds_.find(canonical);
                if (kindIt == kinds_.end()) {
                    if (!options_.allowUnknownFlags) {
                        failUnknownFlag(key);
                        continue;
                    }
                    // Cobra-like (UnknownFlags whitelist): ignore unknown flags; consume an optional value only if the
                    // next token doesn't look like a flag.
                    if (i + 1 < argc && !isFlagToken(std::string(argv[i + 1]))) ++i; // LCOV_EXCL_LINE (both sides of the comparison are exercised; the remaining edges are cleanup paths of the inlined std::string temporary)
                    continue;
                }

                const bool isBool = (kindIt->second == Kind::Bool);
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
                    // pflag: a NoOptDefVal flag never consumes the next token
                    // as its value, whatever it is (`--opt x` keeps x a plain
                    // token; `--opt sub` keeps sub a command candidate).
                    const auto noOptIt = noOptDefaults_.find(canonical);
                    if (noOptIt != noOptDefaults_.end()) {
                        value = noOptIt->second;
                    } else if (i + 1 >= argc) {
                        ok_ = false;
                        error_ = "flag needs an argument: " + key;
                        continue;
                    } else {
                        value = argv[++i];
                    }
                }

                if (!normalizeValue(canonical, value, kindIt->second)) continue;
                recordFlagValue(canonical, std::move(value));
                continue;
            }

            positionals_.push_back(std::move(arg));
        }
    }

    bool hasFlag(const std::string& flag) const {
        const auto it = flagValues_.find(resolveKey(flag));
        return it != flagValues_.end() && !it->second.empty(); // LCOV_EXCL_LINE (flagValues_ entries are only created together with an immediate push_back in recordFlagValue, so the value vector is never empty)
    }

    bool hasValue(const std::string& flag) const {
        const auto key = resolveKey(flag);
        const auto it = flagValues_.find(key);
        if (it != flagValues_.end() && !it->second.empty()) return true; // LCOV_EXCL_LINE (flagValues_ entries are only created together with an immediate push_back in recordFlagValue, so the value vector is never empty)
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
        if (it != flagValues_.end() && !it->second.empty()) return detail::parse<T>(it->second.back(), std::move(defaultValue)); // LCOV_EXCL_LINE (flagValues_ entries are only created together with an immediate push_back in recordFlagValue, so the value vector is never empty)

        const auto extMultiIt = externalMultiValues_.find(key);
        if (extMultiIt != externalMultiValues_.end() && !extMultiIt->second.empty()) {
            return detail::parse<T>(extMultiIt->second.back(), std::move(defaultValue));
        }

        const auto extIt = externalValues_.find(key);
        if (extIt != externalValues_.end()) return detail::parse<T>(extIt->second, std::move(defaultValue));

        const auto defIt = defaults_.find(key);
        if (defIt != defaults_.end()) return detail::parse<T>(defIt->second, std::move(defaultValue));

        return defaultValue;
    }

    // Count helper: sums all parsed integer values for this flag. Intended for Count-style flags that emit "1" per occurrence.
    int getCount(const std::string& flag, int defaultValue = 0) const {
        const auto key = resolveKey(flag);
        const auto it = flagValues_.find(key);
        if (it != flagValues_.end() && !it->second.empty()) { // LCOV_EXCL_LINE (flagValues_ entries are only created together with an immediate push_back in recordFlagValue, so the value vector is never empty)
            int total = 0;
            for (const auto& v : it->second) total += detail::parse<int>(v, 0);
            return total;
        }

        const auto extMultiIt = externalMultiValues_.find(key);
        if (extMultiIt != externalMultiValues_.end() && !extMultiIt->second.empty()) {
            int total = 0;
            for (const auto& v : extMultiIt->second) total += detail::parse<int>(v, 0);
            return total;
        }

        const auto extIt = externalValues_.find(key);
        if (extIt != externalValues_.end()) return detail::parse<int>(extIt->second, defaultValue);

        const auto defIt = defaults_.find(key);
        if (defIt != defaults_.end()) return detail::parse<int>(defIt->second, defaultValue);

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
        if (it != flagValues_.end() && !it->second.empty()) return true; // LCOV_EXCL_LINE (flagValues_ entries are only created together with an immediate push_back in recordFlagValue, so the value vector is never empty)

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
        if (it != flagValues_.end() && !it->second.empty()) return it->second; // LCOV_EXCL_LINE (flagValues_ entries are only created together with an immediate push_back in recordFlagValue, so the value vector is never empty)

        const auto extMultiIt = externalMultiValues_.find(key);
        if (extMultiIt != externalMultiValues_.end() && !extMultiIt->second.empty()) return extMultiIt->second;

        const auto extIt = externalValues_.find(key);
        if (extIt != externalValues_.end()) return {extIt->second}; // LCOV_EXCL_LINE (cleanup edges of the inlined single-element vector construction; the return itself is exercised)

        return {};
    } // LCOV_EXCL_LINE (function cleanup block)

    std::vector<std::string> getFlagValues(const std::string& flag) const {
        const auto key = resolveKey(flag);
        const auto it = flagValues_.find(key);
        if (it != flagValues_.end() && !it->second.empty()) return it->second; // LCOV_EXCL_LINE (flagValues_ entries are only created together with an immediate push_back in recordFlagValue, so the value vector is never empty)

        const auto extMultiIt = externalMultiValues_.find(key);
        if (extMultiIt != externalMultiValues_.end() && !extMultiIt->second.empty()) return extMultiIt->second;

        const auto extIt = externalValues_.find(key);
        if (extIt != externalValues_.end()) return {extIt->second}; // LCOV_EXCL_LINE (cleanup edges of the inlined single-element vector construction; the return itself is exercised)

        const auto defIt = defaults_.find(key);
        if (defIt != defaults_.end()) return {defIt->second}; // LCOV_EXCL_LINE (cleanup edges of the inlined single-element vector construction; the return itself is exercised)

        return {};
    } // LCOV_EXCL_LINE (function cleanup block)

    std::vector<std::string> getFlagValuesSplit(const std::string& flag, char sep = ',') const {
        std::vector<std::string> out;
        for (const auto& v : getFlagValues(flag)) {
            std::size_t start = 0;
            while (start <= v.size()) { // LCOV_EXCL_LINE (the loop always exits via the break when find returns npos; start = pos + 1 never exceeds size, so the false edge is unreachable)
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
            out.push_back(detail::parse<T>(v, defaultValue));
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
    template <typename T>
    std::vector<T> getSlice(const std::string& flag, char sep = ',') const {
        if constexpr (std::is_same_v<T, std::string>) {
            return getFlagValuesSplit(flag, sep);
        } else {
            return getFlagValuesAsFromSplit<T>(flag, sep);
        }
    }

    std::vector<std::string> getStringSlice(const std::string& flag, char sep = ',') const { return getFlagValuesSplit(flag, sep); }
    std::vector<bool> getBoolSlice(const std::string& flag, char sep = ',') const { return getFlagValuesAsFromSplit<bool>(flag, sep); }
    std::vector<int> getIntSlice(const std::string& flag, char sep = ',') const { return getFlagValuesAsFromSplit<int>(flag, sep); }
    std::vector<std::int32_t> getInt32Slice(const std::string& flag, char sep = ',') const {
        return getFlagValuesAsFromSplit<std::int32_t>(flag, sep);
    }
    std::vector<std::int64_t> getInt64Slice(const std::string& flag, char sep = ',') const {
        return getFlagValuesAsFromSplit<std::int64_t>(flag, sep);
    }
    std::vector<std::uint32_t> getUint32Slice(const std::string& flag, char sep = ',') const {
        return getFlagValuesAsFromSplit<std::uint32_t>(flag, sep);
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
    template <typename T>
    std::vector<T> getArray(const std::string& flag) const {
        if constexpr (std::is_same_v<T, std::string>) {
            return getArrayRaw(flag);
        } else if constexpr (std::is_same_v<T, std::chrono::milliseconds>) {
            return getArrayAs<std::chrono::milliseconds>(flag, std::chrono::milliseconds{0});
        } else {
            return getArrayAs<T>(flag, T{});
        }
    }

    std::vector<std::string> getStringArray(const std::string& flag) const {
        return getArrayRaw(flag);
    }

    std::vector<bool> getBoolArray(const std::string& flag) const { return getArrayAs<bool>(flag, false); }
    std::vector<int> getIntArray(const std::string& flag) const { return getArrayAs<int>(flag, 0); }
    std::vector<std::int32_t> getInt32Array(const std::string& flag) const { return getArrayAs<std::int32_t>(flag, 0); }
    std::vector<std::int64_t> getInt64Array(const std::string& flag) const { return getArrayAs<std::int64_t>(flag, 0); }
    std::vector<std::uint32_t> getUint32Array(const std::string& flag) const { return getArrayAs<std::uint32_t>(flag, 0); }
    std::vector<std::uint64_t> getUint64Array(const std::string& flag) const { return getArrayAs<std::uint64_t>(flag, 0); }
    std::vector<float> getFloatArray(const std::string& flag) const { return getArrayAs<float>(flag, 0.0f); }
    std::vector<double> getDoubleArray(const std::string& flag) const { return getArrayAs<double>(flag, 0.0); }
    std::vector<std::chrono::milliseconds> getDurationArray(const std::string& flag) const {
        return getArrayAs<std::chrono::milliseconds>(flag, std::chrono::milliseconds{0});
    }

    // Map helpers: parse "k=v" entries (comma-separated across occurrences).
    template <typename V>
    std::unordered_map<std::string, V> getStringTo(const std::string& flag, char entrySep = ',', char kvSep = '=') const {
        static_assert(std::is_default_constructible_v<V>, "map value type must be default-constructible");
        std::unordered_map<std::string, V> out;
        for (const auto& [k, v] : getFlagMap(flag, entrySep, kvSep)) {
            out[k] = detail::parse<V>(v, V{});
        }
        return out;
    }

    std::unordered_map<std::string, std::string> getStringToString(const std::string& flag,
                                                                    char entrySep = ',',
                                                                    char kvSep = '=') const {
        return getFlagMap(flag, entrySep, kvSep);
    }

    std::unordered_map<std::string, int> getStringToInt(const std::string& flag, char entrySep = ',', char kvSep = '=') const {
        std::unordered_map<std::string, int> out;
        for (const auto& [k, v] : getFlagMap(flag, entrySep, kvSep)) {
            out[k] = detail::parse<int>(v, 0);
        }
        return out;
    }

    std::unordered_map<std::string, std::int32_t> getStringToInt32(const std::string& flag,
                                                                   char entrySep = ',',
                                                                   char kvSep = '=') const {
        return getStringTo<std::int32_t>(flag, entrySep, kvSep);
    }

    std::unordered_map<std::string, std::int64_t> getStringToInt64(const std::string& flag,
                                                                   char entrySep = ',',
                                                                   char kvSep = '=') const {
        std::unordered_map<std::string, std::int64_t> out;
        for (const auto& [k, v] : getFlagMap(flag, entrySep, kvSep)) {
            out[k] = detail::parse<std::int64_t>(v, 0);
        }
        return out;
    }

    std::unordered_map<std::string, std::uint32_t> getStringToUint32(const std::string& flag,
                                                                     char entrySep = ',',
                                                                     char kvSep = '=') const {
        return getStringTo<std::uint32_t>(flag, entrySep, kvSep);
    }

    std::unordered_map<std::string, std::uint64_t> getStringToUint64(const std::string& flag,
                                                                     char entrySep = ',',
                                                                     char kvSep = '=') const {
        std::unordered_map<std::string, std::uint64_t> out;
        for (const auto& [k, v] : getFlagMap(flag, entrySep, kvSep)) {
            out[k] = detail::parse<std::uint64_t>(v, 0);
        }
        return out;
    }

    std::unordered_map<std::string, double> getStringToDouble(const std::string& flag, char entrySep = ',', char kvSep = '=') const {
        return getStringTo<double>(flag, entrySep, kvSep);
    }

    std::unordered_map<std::string, std::chrono::milliseconds> getStringToDuration(const std::string& flag,
                                                                                   char entrySep = ',',
                                                                                   char kvSep = '=') const {
        std::unordered_map<std::string, std::chrono::milliseconds> out;
        for (const auto& [k, v] : getFlagMap(flag, entrySep, kvSep)) {
            out[k] = detail::parse<std::chrono::milliseconds>(v, std::chrono::milliseconds{0});
        }
        return out;
    }

    std::unordered_map<std::string, bool> getStringToBool(const std::string& flag, char entrySep = ',', char kvSep = '=') const {
        std::unordered_map<std::string, bool> out;
        for (const auto& [k, v] : getFlagMap(flag, entrySep, kvSep)) {
            out[k] = detail::parse<bool>(v, false);
        }
        return out;
    }

    std::unordered_map<std::string, float> getStringToFloat(const std::string& flag, char entrySep = ',', char kvSep = '=') const {
        std::unordered_map<std::string, float> out;
        for (const auto& [k, v] : getFlagMap(flag, entrySep, kvSep)) {
            out[k] = detail::parse<float>(v, 0.0f);
        }
        return out;
    } // LCOV_EXCL_LINE (function cleanup block)

    // --- Narrow-width integer getters (pflag int8/uint8/int16/uint16/int32/uint parity) ---

    std::int8_t getInt8(const std::string& flag, std::int8_t defaultValue = 0) const { return getFlag<std::int8_t>(flag, defaultValue); }
    std::int16_t getInt16(const std::string& flag, std::int16_t defaultValue = 0) const { return getFlag<std::int16_t>(flag, defaultValue); }
    std::uint8_t getUint8(const std::string& flag, std::uint8_t defaultValue = 0) const { return getFlag<std::uint8_t>(flag, defaultValue); }
    std::uint16_t getUint16(const std::string& flag, std::uint16_t defaultValue = 0) const { return getFlag<std::uint16_t>(flag, defaultValue); }
    std::int32_t getInt32(const std::string& flag, std::int32_t defaultValue = 0) const { return getFlag<std::int32_t>(flag, defaultValue); }
    std::size_t getUint(const std::string& flag, std::size_t defaultValue = 0) const { return getFlag<std::size_t>(flag, defaultValue); }

    std::vector<std::int8_t> getInt8Slice(const std::string& flag, char sep = ',') const { return getSlice<std::int8_t>(flag, sep); }
    std::vector<std::int16_t> getInt16Slice(const std::string& flag, char sep = ',') const { return getSlice<std::int16_t>(flag, sep); }
    std::vector<std::uint8_t> getUint8Slice(const std::string& flag, char sep = ',') const { return getSlice<std::uint8_t>(flag, sep); }
    std::vector<std::uint16_t> getUint16Slice(const std::string& flag, char sep = ',') const { return getSlice<std::uint16_t>(flag, sep); }

    std::vector<std::int8_t> getInt8Array(const std::string& flag) const { return getArray<std::int8_t>(flag); }
    std::vector<std::int16_t> getInt16Array(const std::string& flag) const { return getArray<std::int16_t>(flag); }
    std::vector<std::uint8_t> getUint8Array(const std::string& flag) const { return getArray<std::uint8_t>(flag); }
    std::vector<std::uint16_t> getUint16Array(const std::string& flag) const { return getArray<std::uint16_t>(flag); }

    // pflag IPSlice parity: canonical IP per element, comma-split per occurrence.
    std::vector<std::string> getIPSlice(const std::string& flag, char sep = ',') const { return getFlagValuesSplit(flag, sep); }

    // pflag bytesBase64 parity: decoded bytes of the stored (canonical) base64 value.
    // The stored value is validated at parse/registration time, so a decode
    // failure here means an unchecked injection (setExternalValues without the
    // Checked variant); clear any partially-decoded bytes rather than returning
    // them as if they were the full value.
    std::vector<unsigned char> getBytesBase64(const std::string& flag) const {
        const auto raw = getFlag<std::string>(flag, std::string());
        std::vector<unsigned char> out;
        if (!detail::tryDecodeBase64(raw, out)) out.clear();
        return out;
    }

    const std::vector<std::string>& positionals() const { return positionals_; }

    bool ok() const { return ok_; }
    const std::string& error() const { return error_; }

private:
    std::vector<std::string> getArrayRaw(const std::string& flag) const {
        const auto key = resolveKey(flag);
        const auto it = flagValues_.find(key);
        if (it != flagValues_.end() && !it->second.empty()) return it->second; // LCOV_EXCL_LINE (flagValues_ entries are only created together with an immediate push_back in recordFlagValue, so the value vector is never empty)

        const auto extMultiIt = externalMultiValues_.find(key);
        if (extMultiIt != externalMultiValues_.end() && !extMultiIt->second.empty()) return extMultiIt->second;

        const auto extIt = externalValues_.find(key);
        if (extIt != externalValues_.end()) return {extIt->second}; // LCOV_EXCL_LINE (cleanup edges of the inlined single-element vector construction; the return itself is exercised)

        const auto defIt = defaults_.find(key);
        if (defIt != defaults_.end()) {
            if (defIt->second.empty()) return {};
            return {defIt->second}; // LCOV_EXCL_LINE (cleanup edges of the inlined single-element vector construction; the return itself is exercised)
        }

        return {};
    } // LCOV_EXCL_LINE (function cleanup block)

    template <typename T>
    std::vector<T> getArrayAs(const std::string& flag, T defaultElement) const {
        std::vector<T> out;
        for (const auto& v : getArrayRaw(flag)) out.push_back(detail::parse<T>(v, defaultElement));
        return out;
    } // LCOV_EXCL_LINE (function cleanup block)

    template <typename T>
    std::vector<T> getFlagValuesAsFromSplit(const std::string& flag, char sep) const {
        std::vector<T> out;
        for (const auto& v : getFlagValuesSplit(flag, sep)) {
            out.push_back(detail::parse<T>(v, T{}));
        }
        return out;
    } // LCOV_EXCL_LINE (function cleanup block)

    static bool isFlagToken(const std::string& s) {
        return s.size() >= 2 && s[0] == '-' && s != "-"; // LCOV_EXCL_LINE (s.size() >= 2 already implies s != "-")
    }

    static bool isShortGroupToken(const std::string& s) {
        if (s.size() < 3) return false;
        if (s.rfind("--", 0) == 0) return false;
        if (s.find('=') != std::string::npos) return false; // LCOV_EXCL_LINE (the sole call site resolves '='-bearing tokens on an earlier path, so no '=' can reach here)
        return s[0] == '-' && s[1] != '-'; // LCOV_EXCL_LINE (the caller guarantees s[0]=='-' and the "--" prefix was rejected above)
    }

    static bool isBoolLiteral(std::string_view s) {
        return s == "1" || s == "0" || s == "true" || s == "false" || s == "True" || s == "False" ||
               s == "TRUE" || s == "FALSE" || s == "on" || s == "off" || s == "yes" || s == "no";
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

    // Type/value vocabulary extracted to detail/parser_value.hpp. Keep a member-visible
    // alias so the private state maps and normalizeValue switch below still name Kind.
    using Kind = detail::ValueKind;


    bool normalizeValue(const std::string& key, std::string& value, Kind kind) {
        if (!ok_) return false;
        const std::string original = value;
        bool valid = true;
        switch (kind) { // LCOV_EXCL_LINE (the switch handles every Kind produced by kindFromDefault, so the range-check default edge is unreachable)
        case Kind::String:
            if (ipKeys_.find(key) != ipKeys_.end()) {
                std::string canon;
                valid = detail::tryParseIP(value, canon);
                if (valid) value = std::move(canon);
            } else if (ipMaskKeys_.find(key) != ipMaskKeys_.end()) {
                std::string canon;
                valid = detail::tryParseIPMask(value, canon);
                if (valid) value = std::move(canon);
            } else if (cidrKeys_.find(key) != cidrKeys_.end()) {
                std::string canon;
                valid = detail::tryParseCIDR(value, canon);
                if (valid) value = std::move(canon);
            } else if (ipNetKeys_.find(key) != ipNetKeys_.end()) {
                std::string canon;
                valid = detail::tryParseCIDR(value, canon);
                if (valid) value = std::move(canon);
            } else if (urlKeys_.find(key) != urlKeys_.end()) {
                std::string canon;
                valid = detail::tryParseURL(value, canon);
                if (valid) value = std::move(canon);
            } else if (ipSliceKeys_.find(key) != ipSliceKeys_.end()) {
                // pflag IPSlice reads CSV: validate/canonicalize each element.
                const auto parts = detail::splitCsvParts(value, ',');
                std::string rejoined;
                for (const auto& partRaw : parts) {
                    std::string canon;
                    if (!detail::tryParseIP(partRaw, canon)) {
                        valid = false;
                        break;
                    }
                    if (!rejoined.empty()) rejoined.push_back(',');
                    rejoined += canon;
                }
                if (valid) value = std::move(rejoined);
            } else if (bytesBase64Keys_.find(key) != bytesBase64Keys_.end()) {
                // pflag bytesBase64: decode strictly, keep the canonical re-encoding.
                std::vector<unsigned char> decoded;
                valid = detail::tryDecodeBase64(value, decoded);
                if (valid) value = detail::encodeBase64(decoded);
            } else {
                valid = true;
            }
            break;
        case Kind::Bool: {
            bool parsed{};
            valid = detail::tryParseBool(value, parsed);
            if (valid) value = parsed ? "true" : "false";
            break;
        }
        case Kind::Int: {
            // Narrow pflag widths validate against their own range; plain int falls through.
            if (int8Keys_.find(key) != int8Keys_.end()) {
                std::int8_t parsed{};
                valid = detail::tryParseSignedInt<std::int8_t>(value, parsed);
            } else if (int16Keys_.find(key) != int16Keys_.end()) {
                std::int16_t parsed{};
                valid = detail::tryParseSignedInt<std::int16_t>(value, parsed);
            } else if (int32Keys_.find(key) != int32Keys_.end()) {
                std::int32_t parsed{};
                valid = detail::tryParseSignedInt<std::int32_t>(value, parsed);
            } else if (uint8Keys_.find(key) != uint8Keys_.end()) {
                std::uint8_t parsed{};
                valid = detail::tryParseUnsignedInt<std::uint8_t>(value, parsed);
            } else if (uint16Keys_.find(key) != uint16Keys_.end()) {
                std::uint16_t parsed{};
                valid = detail::tryParseUnsignedInt<std::uint16_t>(value, parsed);
            } else {
                int parsed{};
                valid = detail::tryParseSignedInt<int>(value, parsed);
            }
            break;
        }
        case Kind::Int64: {
            std::int64_t parsed{};
            valid = detail::tryParseSignedInt<std::int64_t>(value, parsed);
            break;
        }
        case Kind::Uint32: {
            std::uint32_t parsed{};
            valid = detail::tryParseUnsignedInt<std::uint32_t>(value, parsed);
            break;
        }
        case Kind::Uint64: {
            std::uint64_t parsed{};
            if (bytesKeys_.find(key) != bytesKeys_.end()) {
                valid = detail::tryParseBytes(value, parsed);
                if (valid) value = std::to_string(parsed);
            } else if (uintKeys_.find(key) != uintKeys_.end()) {
                // Go `uint` is platform-word-sized; validate against size_t width.
                std::size_t wordParsed{};
                valid = detail::tryParseUnsignedInt<std::size_t>(value, wordParsed);
            } else {
                valid = detail::tryParseUnsignedInt<std::uint64_t>(value, parsed);
            }
            break;
        }
        case Kind::Float: {
            float parsed{};
            valid = detail::tryParseFloat<float>(value, parsed);
            break;
        }
        case Kind::Double: {
            double parsed{};
            valid = detail::tryParseFloat<double>(value, parsed);
            break;
        }
        case Kind::Duration: {
            std::chrono::milliseconds parsed{};
            valid = detail::tryParseDuration(value, parsed);
            break;
        }
        default: // LCOV_EXCL_LINE (Kind::Unknown is never produced by kindFromDefault)
            valid = true; // LCOV_EXCL_LINE
            break; // LCOV_EXCL_LINE
        }

        if (!valid) {
            ok_ = false;
            error_ = "invalid argument \"" + original + "\" for \"" + key + "\"";
            return false;
        }
        return true;
    }

    void registerFlag(const Flag& f) {
        // pflag semantics: flag names are normalized on definition (via the
        // same normalize function used at lookup), so a custom normalize keeps
        // defaults, type kinds and annotation key sets reachable. A throwing
        // hook falls back to the raw name so the ctor still completes.
        auto normalized = [&](const std::string& raw) -> std::string {
            try {
                return normalizeKey(raw);
            } catch (...) {
                return raw;
            }
        };
        std::string name;
        if (!f.longName().empty()) {
            name = normalized(f.longName());
            aliases_[name] = name;
            kinds_[name] = detail::kindFromDefault(f.defaultValue());
            defaults_[name] = detail::toString(f.defaultValue());
            if (f.noOptDefaultValue().has_value()) noOptDefaults_[name] = *f.noOptDefaultValue();
            const auto it = f.annotations().find("count");
            if (it != f.annotations().end() && (it->second == "true" || it->second == "1" || it->second == "yes")) {
                countKeys_.insert(name);
            }
            const auto bytesIt = f.annotations().find("bytes");
            if (bytesIt != f.annotations().end() && detail::isTruthyAnnotation(bytesIt->second)) {
                bytesKeys_.insert(name);
            }
            const auto ipIt = f.annotations().find("ip");
            if (ipIt != f.annotations().end() && detail::isTruthyAnnotation(ipIt->second)) {
                ipKeys_.insert(name);
            }
            const auto ipMaskIt = f.annotations().find("ipmask");
            if (ipMaskIt != f.annotations().end() && detail::isTruthyAnnotation(ipMaskIt->second)) {
                ipMaskKeys_.insert(name);
            }
            const auto cidrIt = f.annotations().find("cidr");
            if (cidrIt != f.annotations().end() && detail::isTruthyAnnotation(cidrIt->second)) {
                cidrKeys_.insert(name);
            }
            const auto ipNetIt = f.annotations().find("ipnet");
            if (ipNetIt != f.annotations().end() && detail::isTruthyAnnotation(ipNetIt->second)) {
                ipNetKeys_.insert(name);
            }
            const auto urlIt = f.annotations().find("url");
            if (urlIt != f.annotations().end() && detail::isTruthyAnnotation(urlIt->second)) {
                urlKeys_.insert(name);
            }
            const auto int8It = f.annotations().find("int8");
            if (int8It != f.annotations().end() && detail::isTruthyAnnotation(int8It->second)) {
                int8Keys_.insert(name);
            }
            const auto int16It = f.annotations().find("int16");
            if (int16It != f.annotations().end() && detail::isTruthyAnnotation(int16It->second)) {
                int16Keys_.insert(name);
            }
            const auto int32It = f.annotations().find("int32");
            if (int32It != f.annotations().end() && detail::isTruthyAnnotation(int32It->second)) {
                int32Keys_.insert(name);
            }
            const auto uint8It = f.annotations().find("uint8");
            if (uint8It != f.annotations().end() && detail::isTruthyAnnotation(uint8It->second)) {
                uint8Keys_.insert(name);
            }
            const auto uint16It = f.annotations().find("uint16");
            if (uint16It != f.annotations().end() && detail::isTruthyAnnotation(uint16It->second)) {
                uint16Keys_.insert(name);
            }
            const auto uintIt = f.annotations().find("uint");
            if (uintIt != f.annotations().end() && detail::isTruthyAnnotation(uintIt->second)) {
                uintKeys_.insert(name);
            }
            const auto ipSliceIt = f.annotations().find("ipslice");
            if (ipSliceIt != f.annotations().end() && detail::isTruthyAnnotation(ipSliceIt->second)) {
                ipSliceKeys_.insert(name);
            }
            const auto bytesBase64It = f.annotations().find("bytesbase64");
            if (bytesBase64It != f.annotations().end() && detail::isTruthyAnnotation(bytesBase64It->second)) {
                bytesBase64Keys_.insert(name);
                // A bytesBase64 default must be valid base64: pflag's default is
                // already-decoded bytes, so an invalid string is a declaration
                // error. Reject it loudly instead of letting getBytesBase64
                // silently yield empty/partial bytes at getter time.
                std::vector<unsigned char> probe;
                if (!detail::tryDecodeBase64(defaults_[name], probe)) {
                    ok_ = false;
                    error_ = "invalid argument \"" + defaults_[name] + "\" for \"" + name + "\"";
                }
            }
            knownKeys_.push_back(name);
        }
        if (!f.shortName().empty() && !f.longName().empty()) {
            const std::string shortName = normalized(f.shortName());
            aliases_[shortName] = name;
            knownKeys_.push_back(shortName);
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
            if (kindIt == kinds_.end()) {
                if (!options_.allowUnknownFlags) {
                    failUnknownFlag(key);
                    return false;
                }
                // Cobra-like (UnknownFlags whitelist): ignore unknown short flags inside a group, but only the last
                // unknown flag may consume an optional value (if the next token isn't another flag).
                if (pos + 1 == group.size()) {
                    if (i + 1 < argc && !isFlagToken(std::string(argv[i + 1]))) ++i; // LCOV_EXCL_LINE (both sides of the comparison are exercised; the remaining edges are cleanup paths of the inlined std::string temporary)
                    return true;
                }
                continue;
            }

            if (kindIt != kinds_.end() && kindIt->second == Kind::Bool) { // LCOV_EXCL_LINE (unknown shorts already returned or continued above, so the flag is always known here)
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
                        if (j == 0 && ch == '+') continue;
                        if (ch < '0' || ch > '9') {
                            numeric = false;
                            break;
                        }
                    }
                    if (!remainder.empty() && numeric) { // LCOV_EXCL_LINE (guarded by pos + 1 < group.size(), so remainder is never empty)
                        auto v = remainder;
                        if (kindIt != kinds_.end() && !normalizeValue(canonical, v, kindIt->second)) return false; // LCOV_EXCL_LINE (unknown shorts already returned above, so the kind lookup always succeeds here)
                        recordFlagValue(canonical, std::move(v));
                        return true;
                    }
                    // pflag rejects a negative count increment; do not fall back to a single occurrence.
                    if (remainder.front() == '-') {
                        ok_ = false;
                        error_ = "invalid argument \"" + remainder + "\" for \"" + key + "\"";
                        return false;
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
                // pflag: a NoOptDefVal flag never consumes the next token as
                // its value (mirrors the long-form rule above).
                const auto noOptIt = noOptDefaults_.find(canonical);
                if (noOptIt != noOptDefaults_.end()) {
                    value = noOptIt->second;
                } else if (i + 1 >= argc) {
                    ok_ = false;
                    error_ = "flag needs an argument: " + key;
                    return false;
                } else {
                    value = argv[++i];
                }
            }
            if (kindIt != kinds_.end() && !normalizeValue(canonical, value, kindIt->second)) return false; // LCOV_EXCL_LINE (unknown shorts already returned above, so the kind lookup always succeeds here)
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
    std::unordered_set<std::string> ipKeys_;
    std::unordered_set<std::string> ipMaskKeys_;
    std::unordered_set<std::string> cidrKeys_;
    std::unordered_set<std::string> ipNetKeys_;
    std::unordered_set<std::string> urlKeys_;
    std::unordered_set<std::string> int8Keys_;
    std::unordered_set<std::string> int16Keys_;
    std::unordered_set<std::string> int32Keys_;
    std::unordered_set<std::string> uint8Keys_;
    std::unordered_set<std::string> uint16Keys_;
    std::unordered_set<std::string> uintKeys_;
    std::unordered_set<std::string> ipSliceKeys_;
    std::unordered_set<std::string> bytesBase64Keys_;
    std::vector<std::string> knownKeys_;
    std::vector<std::string> positionals_;
    bool ok_{true};
    std::string error_;
    Options options_;
};

} // namespace clasp

#endif // PARSER_HPP
