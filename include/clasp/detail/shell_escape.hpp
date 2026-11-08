#ifndef CLASP_DETAIL_SHELL_ESCAPE_HPP
#define CLASP_DETAIL_SHELL_ESCAPE_HPP

// Shell word/description escaping and word-list joining for the generated
// bash / zsh / fish / powershell completion scripts. Kept apart from the
// Command class so the escaping rules (and their fish double-encoding
// invariant) live in one place and stay unit-testable without a Command.

#include <cctype>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace clasp::detail {

// Forward declaration: joinFishWords / joinFishConditionWords / escapeFishSingleQuoted
// use the content escaper before its definition below (they were class members in
// command.hpp where declaration order did not matter).
inline std::string escapeFishSingleQuotedContent(const std::string& s);

inline std::string escapeDoubleQuotes(const std::string& s) {
std::string out;
out.reserve(s.size());
for (const char ch : s) {
    if (ch == '"' || ch == '\\' || ch == '$' || ch == '`') out.push_back('\\');
    out.push_back(ch);
}
return out;
} // LCOV_EXCL_LINE (function cleanup block)

inline std::string escapeHelpString(const std::string& s) {
std::string out;
out.reserve(s.size());
for (const char ch : s) {
    if (ch == '"') out.push_back('\\');
    out.push_back(ch);
}
return out;
} // LCOV_EXCL_LINE (function cleanup block)

inline std::string escapeBashWord(const std::string& s) {
std::string out = "\"";
out.reserve(s.size() + 2);
for (const char ch : s) {
    if (ch == '"' || ch == '\\' || ch == '$' || ch == '`') out.push_back('\\');
    out.push_back(ch);
}
out.push_back('"');
return out;
} // LCOV_EXCL_LINE (function cleanup block)

inline std::string escapeBashDoubleQuoted(const std::string& s) {
return escapeDoubleQuotes(s);
} // LCOV_EXCL_LINE (function cleanup block)

inline std::string escapeFishWord(const std::string& s) {
std::string out = "\"";
out.reserve(s.size() + 2);
for (const char ch : s) {
    if (ch == '"' || ch == '\\' || ch == '$') out.push_back('\\');
    out.push_back(ch);
}
out.push_back('"');
return out;
} // LCOV_EXCL_LINE (function cleanup block)

inline std::string escapeFishDoubleQuoted(const std::string& s) {
std::string out;
out.reserve(s.size());
for (const char ch : s) {
    if (ch == '"' || ch == '\\' || ch == '$') out.push_back('\\');
    out.push_back(ch);
}
return out;
} // LCOV_EXCL_LINE (function cleanup block)

inline std::string joinBashWords(const std::vector<std::string>& words) {
std::ostringstream oss;
for (std::size_t i = 0; i < words.size(); ++i) {
    if (i) oss << ' ';
    oss << escapeBashDoubleQuoted(words[i]);
}
return oss.str();
}

inline std::string joinFishWords(const std::vector<std::string>& words) {
std::ostringstream oss;
for (std::size_t i = 0; i < words.size(); ++i) {
    if (i) oss << ' ';
    // fish re-evaluates the `-a` argument list as code, so each word must
    // be emitted as a fully-quoted fish literal. Single quotes never
    // expand `$`, backticks or `()` in fish, so they are the only quoting
    // form that stays literal in *every* evaluation context (unlike
    // double quotes, whose escape semantics depend on whether fish treats
    // the string as a `complete` argument or as ordinary code). A bare
    // apostrophe is escaped with `\'` inside the single quotes.
    oss << "'" << escapeFishSingleQuotedContent(words[i]) << "'";
}
return oss.str();
}

// Builds the fish `-n` condition string for `__fish_seen_subcommand_from`.
// The condition is re-evaluated as fish code at completion time, so each word
// must be emitted as a fully-quoted fish literal. Single quotes (with `'`
// escaped as `\'`) keep dollars, backticks and command substitutions literal in every
// evaluation context, so the condition does not depend on fish's `complete`
// argument handling to avoid executing `$(...)`/`` `...` ``.
inline std::string joinFishConditionWords(const std::vector<std::string>& words) {
std::ostringstream oss;
for (std::size_t i = 0; i < words.size(); ++i) {
    if (i) oss << ' ';
    oss << "'" << escapeFishSingleQuotedContent(words[i]) << "'";
}
return oss.str();
}

inline std::string escapeFishSingleQuotedContent(const std::string& s) {
std::string out;
out.reserve(s.size());
for (const char ch : s) {
    // Inside a fish single-quoted string, `'` is escaped by a preceding
    // `\`. Escape every backslash too: otherwise an odd trailing backslash
    // escapes the closing quote when fish re-evaluates the generated list.
    if (ch == '\'') out += "\\'";
    else if (ch == '\\') out += "\\\\";
    else out.push_back(ch);
}
return out;
} // LCOV_EXCL_LINE (function cleanup block)

inline std::string escapeFishSingleQuoted(const std::string& s) {
// The outer wrapper must produce the same content escaping as the inner
// helper so the `-a`/`-n` double-encoding round-trip stays consistent.
// Delegating here keeps the two escaping rules in lock-step; editing only
// one of them would silently corrupt the fish completion script.
return "'" + escapeFishSingleQuotedContent(s) + "'";
} // LCOV_EXCL_LINE (function cleanup block)

inline std::string joinQuotedPowerShell(const std::vector<std::string>& words) {
auto escapeSingleQuoted = [](const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (const char ch : s) {
        if (ch == '\'') out += "''";
        else out.push_back(ch);
    }
    return out;
}; // LCOV_EXCL_LINE (closure cleanup block)

std::ostringstream oss;
for (std::size_t i = 0; i < words.size(); ++i) {
    if (i) oss << ", ";
    oss << "'" << escapeSingleQuoted(words[i]) << "'";
}
return oss.str();
}

inline std::string escapePowerShellSingleQuoted(const std::string& s) {
std::string out = "'";
out.reserve(s.size() + 2);
for (const char ch : s) {
    if (ch == '\'') out += "''";
    else out.push_back(ch);
}
out.push_back('\'');
return out;
} // LCOV_EXCL_LINE (function cleanup block)

inline std::string sanitizeIdentifier(const std::string& s) {
std::string out;
out.reserve(s.size());
for (const unsigned char ch : s) {
    if (std::isalnum(ch) || ch == '_') out.push_back(static_cast<char>(ch));
    else out.push_back('_');
}
if (out.empty() || (!std::isalpha(static_cast<unsigned char>(out[0])) && out[0] != '_')) out.insert(out.begin(), '_');
return out;
} // LCOV_EXCL_LINE (function cleanup block)

inline std::string bashCaseLabel(const std::vector<std::string>& paths) {
std::ostringstream oss;
for (std::size_t i = 0; i < paths.size(); ++i) {
    if (i) oss << "|";
    oss << "\"" << escapeDoubleQuotes(paths[i]) << "\"";
}
return oss.str();
}

} // namespace clasp::detail

#endif // CLASP_DETAIL_SHELL_ESCAPE_HPP
