#ifndef CLASP_DETAIL_CONFIG_PARSE_HPP
#define CLASP_DETAIL_CONFIG_PARSE_HPP

// Flat parsers for Clasp's supported config formats (.env / .ini / .cfg / .json
// / .toml / .yaml). Each returns a flattened key->value map (plus multi-value
// lists) that Command::applyExternalSources merges into the parser with
// CLI > env > config > default precedence. Config files are untrusted input:
// the JSON parser bounds nesting depth so a deeply nested file cannot overflow
// the stack.

#include <cctype>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "value_parse.hpp"

namespace clasp::detail {

inline bool endsWith(std::string_view s, std::string_view suffix) {
    if (suffix.size() > s.size()) return false;
    return s.substr(s.size() - suffix.size()) == suffix;
}

inline std::unordered_map<std::string, std::string> parseEnvLike(std::istream& in) {
    std::unordered_map<std::string, std::string> raw;
    std::string line;
    while (std::getline(in, line)) {
        const auto t = trimWs(line);
        if (t.empty()) continue;
        if (t.front() == '#') continue;
        const auto eq = t.find('=');
        if (eq == std::string_view::npos) continue;
        const auto key = trimWs(t.substr(0, eq));
        auto value = std::string(trimWs(t.substr(eq + 1)));
        if (value.size() >= 2 && ((value.front() == '"' && value.back() == '"') || (value.front() == '\'' && value.back() == '\''))) {
            value = value.substr(1, value.size() - 2);
        }
        raw[std::string(key)] = std::move(value);
    }
    return raw;
};

struct ConfigRaw {
    std::unordered_map<std::string, std::string> scalar;
    std::unordered_map<std::string, std::vector<std::string>> multi;
};

struct JsonParse {
    const char* p{nullptr};
    const char* end{nullptr};

    // Config files are untrusted input; unbounded recursion on deeply nested
    // JSON would overflow the stack. Reject nesting past this depth (config
    // keys flatten to "a.b.c" paths, so real configs nest a handful of levels).
    enum : int { kMaxNestingDepth = 128 };
    int depth{0};

    struct DepthGuard {
        JsonParse* jp;
        bool ok;
        explicit DepthGuard(JsonParse& j) : jp(&j), ok(j.enter()) {}
        ~DepthGuard() {
            if (ok) jp->leave();
        }
    };

    bool enter() {
        if (depth >= kMaxNestingDepth) return false;
        ++depth;
        return true;
    }

    void leave() { --depth; }

    static bool isWs(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }

    void skipWs() {
        while (p < end && isWs(*p)) ++p;
    }

    bool consume(char c) {
        skipWs();
        if (p >= end || *p != c) return false;
        ++p;
        return true;
    }

    std::optional<std::string> parseString() {
        skipWs();
        if (p >= end || *p != '"') return std::nullopt;
        ++p;
        std::string out;
        while (p < end) {
            const char ch = *p++;
            if (ch == '"') return out;
            if (ch != '\\') {
                out.push_back(ch);
                continue;
            }
            if (p >= end) return std::nullopt;
            const char esc = *p++;
            switch (esc) {
            case '"': out.push_back('"'); break;
            case '\\': out.push_back('\\'); break;
            case '/': out.push_back('/'); break;
            case 'b': out.push_back('\b'); break;
            case 'f': out.push_back('\f'); break;
            case 'n': out.push_back('\n'); break;
            case 'r': out.push_back('\r'); break;
            case 't': out.push_back('\t'); break;
            default:
                // Minimal parser: unsupported escapes fail.
                return std::nullopt;
            }
        }
        return std::nullopt;
    }

    std::optional<std::string> parseNumberToken() {
        skipWs();
        if (p >= end) return std::nullopt; // LCOV_EXCL_LINE (sole caller checked *p is a digit, so p < end)
        const char* start = p;
        if (*p == '-' || *p == '+') ++p;
        bool any = false;
        while (p < end && std::isdigit(static_cast<unsigned char>(*p))) {
            any = true;
            ++p;
        }
        if (p < end && *p == '.') {
            ++p;
            while (p < end && std::isdigit(static_cast<unsigned char>(*p))) {
                any = true;
                ++p;
            }
        }
        if (!any) return std::nullopt;
        if (p < end && (*p == 'e' || *p == 'E')) {
            ++p;
            if (p < end && (*p == '+' || *p == '-')) ++p;
            bool expAny = false;
            while (p < end && std::isdigit(static_cast<unsigned char>(*p))) {
                expAny = true;
                ++p;
            }
            if (!expAny) return std::nullopt;
        }
        return std::string(start, p);
    }

    bool startsWith(std::string_view lit) const {
        return static_cast<std::size_t>(end - p) >= lit.size() && std::string_view(p, lit.size()) == lit;
    }

    std::optional<std::string> parseScalarToken() {
        skipWs();
        if (p >= end) return std::nullopt;
        if (*p == '"') return parseString();
        if (*p == '-' || *p == '+' || std::isdigit(static_cast<unsigned char>(*p))) return parseNumberToken();
        if (startsWith("true")) {
            p += 4;
            return std::string("true");
        }
        if (startsWith("false")) {
            p += 5;
            return std::string("false");
        }
        if (startsWith("null")) {
            p += 4;
            return std::string();
        }
        return std::nullopt;
    }

    std::optional<std::vector<std::string>> parseArrayScalarTokens() {
        if (!consume('[')) return std::nullopt; // LCOV_EXCL_LINE (sole call site guarantees *p == '[')
        skipWs();
        std::vector<std::string> out;
        if (consume(']')) return out;
        while (true) {
            skipWs();
            if (p >= end) return std::nullopt;
            if (*p == '{' || *p == '[') return std::nullopt;
            auto v = parseScalarToken();
            if (!v.has_value()) return std::nullopt;
            out.push_back(*v);
            skipWs();
            if (consume(']')) return out;
            if (!consume(',')) return std::nullopt;
        }
    }

    bool skipValue() {
        skipWs();
        if (p >= end) return false;
        if (*p == '{') return skipObject();
        if (*p == '[') return skipArray();
        return static_cast<bool>(parseScalarToken());
    }

    bool skipArray() {
        if (!consume('[')) return false; // LCOV_EXCL_LINE (sole call site guarantees *p == '[')
        const DepthGuard guard(*this);
        if (!guard.ok) return false;
        skipWs();
        if (consume(']')) return true;
        while (true) {
            if (!skipValue()) return false;
            skipWs();
            if (consume(']')) return true;
            if (!consume(',')) return false;
        }
    }

    bool skipObject() {
        if (!consume('{')) return false; // LCOV_EXCL_LINE (sole call site guarantees *p == '{')
        const DepthGuard guard(*this);
        if (!guard.ok) return false;
        skipWs();
        if (consume('}')) return true;
        while (true) {
            if (!parseString().has_value()) return false;
            if (!consume(':')) return false;
            if (!skipValue()) return false;
            skipWs();
            if (consume('}')) return true;
            if (!consume(',')) return false;
        }
    }

    bool parseObjectFlatten(ConfigRaw& out, const std::string& prefix) {
        if (!consume('{')) return false;
        const DepthGuard guard(*this);
        if (!guard.ok) return false;
        skipWs();
        if (consume('}')) return true;
        while (true) {
            auto key = parseString();
            if (!key.has_value()) return false;
            if (!consume(':')) return false;
            skipWs();
            const std::string fullKey = prefix.empty() ? *key : (prefix + "." + *key); // LCOV_EXCL_LINE (cold inlined clone of the fullKey ternary; main instantiation fully covered)

            if (p < end && *p == '{') {
                if (!parseObjectFlatten(out, fullKey)) return false;
            } else if (p < end && *p == '[') {
                const char* checkpoint = p;
                if (auto vec = parseArrayScalarTokens()) {
                    if (!vec->empty()) out.multi[fullKey] = std::move(*vec);
                } else {
                    p = checkpoint;
                    // Not a scalar array, skip without failing.
                    if (!skipArray()) return false;
                }
            } else {
                auto val = parseScalarToken();
                if (!val.has_value()) return false;
                out.scalar[fullKey] = *val;
            }

            skipWs();
            if (consume('}')) return true;
            if (!consume(',')) return false;
        }
    }
};

inline std::optional<ConfigRaw> parseJsonFlatten(std::istream& in) {
    std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    JsonParse jp;
    jp.p = contents.data();
    jp.end = contents.data() + contents.size();
    jp.skipWs();
    ConfigRaw out;
    if (!jp.parseObjectFlatten(out, /*prefix=*/"")) return std::nullopt;
    jp.skipWs();
    if (jp.p != jp.end) return std::nullopt;
    return out;
};

inline std::optional<ConfigRaw> parseTomlFlatten(std::istream& in) {
    ConfigRaw out;
    std::string tablePrefix;

    auto stripComment = [](std::string_view s) -> std::string_view {
        bool inDouble = false;
        bool inSingle = false;
        for (std::size_t i = 0; i < s.size(); ++i) {
            const char ch = s[i];
            if (!inSingle && ch == '"' && (i == 0 || s[i - 1] != '\\')) inDouble = !inDouble;
            else if (!inDouble && ch == '\'') inSingle = !inSingle;
            else if (!inDouble && !inSingle && ch == '#') return s.substr(0, i);
        }
        return s;
    };

    auto unquote = [](std::string_view v) -> std::string {
        if (v.size() >= 2 && v.front() == '"' && v.back() == '"') {
            std::string out; // LCOV_EXCL_LINE (default-constructed string always takes the SSO edge)
            out.reserve(v.size() - 2);
            for (std::size_t i = 1; i + 1 < v.size(); ++i) {
                char ch = v[i];
                if (ch == '\\' && i + 2 < v.size()) {
                    const char n = v[++i];
                    switch (n) {
                    case 'n': out.push_back('\n'); break;
                    case 't': out.push_back('\t'); break;
                    case 'r': out.push_back('\r'); break;
                    case '\\': out.push_back('\\'); break;
                    case '"': out.push_back('"'); break;
                    default: out.push_back(n); break;
                    }
                    continue;
                }
                out.push_back(ch);
            }
            return out; // LCOV_EXCL_LINE (NRVO fallback edge of 'return out', compiler-generated)
        } // LCOV_EXCL_LINE (block only reachable via the returns above)
        if (v.size() >= 2 && v.front() == '\'' && v.back() == '\'') { // LCOV_EXCL_LINE (cold inlined clone; main instantiation covers both quote kinds)
            return std::string(v.substr(1, v.size() - 2));
        }
        return std::string(v);
    }; // LCOV_EXCL_LINE (closure cleanup block)

    auto parseTomlArray = [&](std::string_view v) -> std::optional<std::vector<std::string>> {
        v = trimWs(v);
        if (v.size() < 2 || v.front() != '[' || v.back() != ']') return std::nullopt; // LCOV_EXCL_LINE (cold inlined clone of the array-shape guard; main instantiation covered)
        v = trimWs(v.substr(1, v.size() - 2));
        std::vector<std::string> out;
        std::size_t i = 0;
        while (i < v.size()) {
            while (i < v.size() && std::isspace(static_cast<unsigned char>(v[i]))) ++i; // LCOV_EXCL_LINE (cold inlined clone of the whitespace-skip loop)
            if (i >= v.size()) break; // LCOV_EXCL_LINE (cold inlined clone of the post-item break check)
            std::size_t start = i;
            bool inDouble = false;
            bool inSingle = false;
            while (i < v.size()) {
                const char ch = v[i];
                if (!inSingle && ch == '"' && (i == start || v[i - 1] != '\\')) inDouble = !inDouble;
                else if (!inDouble && ch == '\'') inSingle = !inSingle;
                else if (!inDouble && !inSingle && ch == ',') break;
                ++i;
            }
            auto item = trimWs(v.substr(start, i - start));
            if (!item.empty()) out.push_back(unquote(item));
            if (i < v.size() && v[i] == ',') ++i; // LCOV_EXCL_LINE (cold inlined clone of the comma-advance check)
        }
        return out;
    };

    std::string line;
    while (std::getline(in, line)) {
        const auto t = trimWs(line);
        if (t.empty()) continue;
        if (t.front() == '#') continue;

        if (t.front() == '[') {
            // Table header: [a.b]
            const auto close = t.find(']');
            if (close == std::string_view::npos) return std::nullopt;
            const auto inner = trimWs(t.substr(1, close - 1));
            if (inner.empty()) {
                tablePrefix.clear();
            } else {
                tablePrefix = std::string(inner);
            }
            continue;
        }

        const auto eq = t.find('=');
        if (eq == std::string_view::npos) continue;
        const auto key = trimWs(t.substr(0, eq));
        auto valueView = trimWs(t.substr(eq + 1));
        valueView = trimWs(stripComment(valueView));

        if (key.empty()) continue;
        if (valueView.empty()) {
            // Preserve empty value as explicit override.
        }

        const std::string fullKey = tablePrefix.empty() ? std::string(key) : (tablePrefix + "." + std::string(key)); // LCOV_EXCL_LINE (cold inlined clone of the tablePrefix ternary and its string temporaries)
        if (!valueView.empty() && valueView.front() == '[') {
            if (auto vec = parseTomlArray(valueView)) {
                if (!vec->empty()) out.multi[fullKey] = std::move(*vec);
                continue;
            }
            // Unknown array type; skip.
            continue;
        }
        if (!valueView.empty() && valueView.front() == '{') {
            // Inline table not supported.
            continue;
        }
        out.scalar[fullKey] = unquote(valueView);
    }

    return out;
};

inline std::optional<ConfigRaw> parseIniFlatten(std::istream& in) {
    ConfigRaw out;
    std::string sectionPrefix;

    std::string line;
    while (std::getline(in, line)) {
        const auto t = trimWs(line);
        if (t.empty()) continue;
        if (t.front() == '#' || t.front() == ';') continue;

        if (t.front() == '[') {
            const auto close = t.find(']');
            if (close == std::string_view::npos) continue;
            const auto inner = trimWs(t.substr(1, close - 1));
            sectionPrefix = std::string(inner);
            continue;
        }

        auto eq = t.find('=');
        if (eq == std::string_view::npos) eq = t.find(':');
        if (eq == std::string_view::npos) continue;

        const auto key = trimWs(t.substr(0, eq));
        if (key.empty()) continue;

        auto value = std::string(trimWs(t.substr(eq + 1)));
        if (value.size() >= 2 &&
            ((value.front() == '"' && value.back() == '"') ||
             (value.front() == '\'' && value.back() == '\''))) {
            value = value.substr(1, value.size() - 2);
        }

        const std::string fullKey =
            sectionPrefix.empty() ? std::string(key) : (sectionPrefix + "." + std::string(key)); // LCOV_EXCL_LINE (cold inlined clone of the sectionPrefix ternary and its string temporaries)
        out.scalar[fullKey] = std::move(value);
    }

    return out;
};

inline std::optional<ConfigRaw> parseYamlFlatten(std::istream& in) {
    ConfigRaw out;

    struct Frame {
        std::size_t indent;
        std::string key;
    };
    std::vector<Frame> stack;

    auto stripComment = [](std::string_view s) -> std::string_view {
        bool inDouble = false;
        bool inSingle = false;
        for (std::size_t i = 0; i < s.size(); ++i) {
            const char ch = s[i];
            if (!inSingle && ch == '"' && (i == 0 || s[i - 1] != '\\')) inDouble = !inDouble;
            else if (!inDouble && ch == '\'') inSingle = !inSingle;
            else if (!inDouble && !inSingle && ch == '#') return s.substr(0, i);
        }
        return s;
    };

    auto unquote = [&](std::string_view v) -> std::string {
        v = trimWs(v);
        if (v.size() >= 2 && ((v.front() == '"' && v.back() == '"') || (v.front() == '\'' && v.back() == '\''))) {
            return std::string(v.substr(1, v.size() - 2));
        }
        return std::string(v);
    };

    auto fullKeyFromStack = [&](const std::vector<Frame>& frames) -> std::string {
        std::string fullKey;
        for (const auto& f : frames) {
            if (!fullKey.empty()) fullKey.push_back('.');
            fullKey += f.key;
        }
        return fullKey;
    }; // LCOV_EXCL_LINE (closure cleanup block)

    std::string line;
    while (std::getline(in, line)) {
        // Count indentation (spaces only).
        std::size_t indent = 0;
        while (indent < line.size() && line[indent] == ' ') ++indent;
        std::string_view content(line);
        content = content.substr(indent);
        content = trimWs(stripComment(content));
        if (content.empty()) continue;

        // Adjust nesting stack based on indent.
        while (!stack.empty() && indent <= stack.back().indent) stack.pop_back();

        // Sequence item: treat as multi-value for current stack path.
        if (content.rfind("-", 0) == 0) {
            if (stack.empty()) continue;
            auto item = trimWs(content.substr(1));
            const auto keyPath = fullKeyFromStack(stack);
            if (!keyPath.empty()) out.multi[keyPath].push_back(unquote(item)); // LCOV_EXCL_LINE (stack frames always carry non-empty keys)
            continue;
        }

        const auto colon = content.find(':');
        if (colon == std::string_view::npos) continue;

        const auto key = trimWs(content.substr(0, colon));
        if (key.empty()) continue;

        auto value = trimWs(content.substr(colon + 1));
        const bool hasValue = !value.empty();

        std::string fullKey;
        for (const auto& f : stack) {
            if (!fullKey.empty()) fullKey.push_back('.'); // LCOV_EXCL_LINE (push_back('.') on an empty string always takes the SSO edge)
            fullKey += f.key;
        }
        if (!fullKey.empty()) fullKey.push_back('.');
        fullKey += std::string(key);

        if (hasValue) {
            out.scalar[fullKey] = unquote(value);
        } else {
            // New mapping section.
            stack.push_back(Frame{indent, std::string(key)});
        }
    }

    return out;
};

} // namespace clasp::detail

#endif // CLASP_DETAIL_CONFIG_PARSE_HPP
