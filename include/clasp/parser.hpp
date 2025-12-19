#ifndef PARSER_HPP
#define PARSER_HPP

#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <variant>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
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
    };

    Parser(int argc, char** argv, const std::vector<Flag>& flags) : Parser(argc, argv, flags, Options{}) {}

    Parser(int argc, char** argv, const std::vector<Flag>& flags, Options options) : options_(options) {
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
                        flagValues_[canonical] = "false";
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
                    if (isBool) {
                        if (i + 1 < argc && isBoolLiteral(argv[i + 1])) {
                            value = argv[++i];
                        } else {
                            value = "true";
                        }
                    } else {
                        if (i + 1 >= argc) {
                            ok_ = false;
                            error_ = "flag needs an argument: " + key;
                            continue;
                        }
                        value = argv[++i];
                    }
                }

                key = normalizeKey(std::move(key));
                if (kinds_.find(key) == kinds_.end() && !options_.allowUnknownFlags) {
                    failUnknownFlag(key);
                    continue;
                }
                flagValues_[std::move(key)] = std::move(value);
                continue;
            }

            positionals_.push_back(std::move(arg));
        }
    }

    bool hasFlag(const std::string& flag) const {
        const auto it = flagValues_.find(resolveKey(flag));
        return it != flagValues_.end();
    }

    template <typename T>
    T getFlag(const std::string& flag, T defaultValue = T()) const {
        const auto key = resolveKey(flag);
        const auto it = flagValues_.find(key);
        if (it != flagValues_.end()) return parse<T>(it->second, std::move(defaultValue));

        const auto defIt = defaults_.find(key);
        if (defIt != defaults_.end()) return parse<T>(defIt->second, std::move(defaultValue));

        return defaultValue;
    }

    const std::vector<std::string>& positionals() const { return positionals_; }

    bool ok() const { return ok_; }
    const std::string& error() const { return error_; }

private:
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

    std::string normalizeKey(std::string k) const {
        const auto it = aliases_.find(k);
        if (it != aliases_.end()) return it->second;
        return k;
    }

    std::string resolveKey(const std::string& k) const {
        const auto it = aliases_.find(k);
        if (it != aliases_.end()) return it->second;
        return k;
    }

    template <typename T>
    static T parse(const std::string& s, T defaultValue) {
        std::istringstream iss(s);
        T out{};
        if (!(iss >> out)) return defaultValue;
        return out;
    }

    static bool parseBool(const std::string& s, bool defaultValue) {
        if (s.empty()) return defaultValue;
        if (s == "1" || s == "true" || s == "True" || s == "TRUE" || s == "on" || s == "yes") return true;
        if (s == "0" || s == "false" || s == "False" || s == "FALSE" || s == "off" || s == "no") return false;
        return defaultValue;
    }

    enum class Kind { Bool, Int, String, Float, Unknown };

    static Kind kindFromDefault(const FlagValue& v) {
        switch (v.index()) {
        case 0: return Kind::Bool;
        case 1: return Kind::Int;
        case 2: return Kind::String;
        case 3: return Kind::Float;
        default: return Kind::Unknown;
        }
    }

    static std::string toString(const FlagValue& v) {
        return std::visit(
            [](const auto& x) -> std::string {
                using T = std::decay_t<decltype(x)>;
                if constexpr (std::is_same_v<T, bool>) return x ? "true" : "false";
                if constexpr (std::is_same_v<T, int>) return std::to_string(x);
                if constexpr (std::is_same_v<T, float>) return std::to_string(x);
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
            knownKeys_.push_back(f.longName());
        }
        if (!f.shortName().empty() && !f.longName().empty()) {
            aliases_[f.shortName()] = f.longName();
            knownKeys_.push_back(f.shortName());
        }
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
                flagValues_[canonical] = "true";
                continue;
            }

            // Needs a value: -ovalue OR -o value
            std::string value;
            if (pos + 1 < group.size()) {
                value = group.substr(pos + 1);
            } else {
                if (i + 1 >= argc) {
                    ok_ = false;
                    error_ = "flag needs an argument: " + key;
                    return false;
                }
                value = argv[++i];
            }
            flagValues_[canonical] = std::move(value);
            return true;
        }
        return true;
    }

    void failUnknownFlag(const std::string& key) {
        ok_ = false;
        error_ = "unknown flag: " + key;
        if (!options_.suggestFlags || knownKeys_.empty()) return;
        const auto suggestions = utils::suggest(key, knownKeys_);
        if (suggestions.empty()) return;
        error_ += "\n\nDid you mean this?\n";
        for (const auto& s : suggestions) error_ += "  " + s + "\n";
    }

    std::unordered_map<std::string, std::string> flagValues_;
    std::unordered_map<std::string, std::string> aliases_;
    std::unordered_map<std::string, Kind> kinds_;
    std::unordered_map<std::string, std::string> defaults_;
    std::vector<std::string> knownKeys_;
    std::vector<std::string> positionals_;
    bool ok_{true};
    std::string error_;
    Options options_;
};

template <>
inline bool Parser::parse<bool>(const std::string& s, bool defaultValue) {
    return parseBool(s, defaultValue);
}

template <>
inline std::string Parser::parse<std::string>(const std::string& s, std::string /*defaultValue*/) {
    return s;
}

} // namespace clasp

#endif // PARSER_HPP
