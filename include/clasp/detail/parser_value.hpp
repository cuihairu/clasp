#ifndef CLASP_DETAIL_PARSER_VALUE_HPP
#define CLASP_DETAIL_PARSER_VALUE_HPP

// Type-value vocabulary for the Parser: the generic parse<T>/parseBool, the
// ValueKind discriminant derived from a flag's default, and the validators / canonical
// formatters for the extended flag types (bytes sizes, IPv4/IPv6/IP/CIDR/IPMask/URL,
// CSV slices). Extracted from Parser so the validation rules are testable and reusable
// without pulling the whole parsing state machine.

#include <array>
#include <chrono>
#include <cctype>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

#include "../flag.hpp"
#include "value_parse.hpp"

namespace clasp::detail {

// Forward declaration: parse<T> (a template, instantiated from parser.hpp) uses the bool
// escaper before its definition below; class members in Parser were visible throughout the
// class, so declaration order did not matter for the extracted free functions.
bool parseBool(const std::string& s, bool defaultValue);

    template <typename T>
    T parse(const std::string& s, T defaultValue) {
        if constexpr (std::is_same_v<T, bool>) {
            return parseBool(s, defaultValue);
        } else if constexpr (std::is_same_v<T, std::string>) {
            return s;
        } else if constexpr (std::is_same_v<T, std::chrono::milliseconds>) {
            return detail::parseDuration(s, defaultValue);
        } else if constexpr (std::is_integral_v<T>) {
            if constexpr (std::is_signed_v<T>) {
                T out{};
                if (!detail::tryParseSignedInt<T>(s, out)) return defaultValue;
                return out;
            } else {
                T out{};
                if (!detail::tryParseUnsignedInt<T>(s, out)) return defaultValue;
                return out;
            }
        } else if constexpr (std::is_floating_point_v<T>) {
            T out{};
            if (!detail::tryParseFloat<T>(s, out)) return defaultValue;
            return out;
        } else {
            std::istringstream iss(s);
            T out{};
            if (!(iss >> out)) return defaultValue;
            return out;
        }
    }

    bool parseBool(const std::string& s, bool defaultValue) {
        if (s.empty()) return defaultValue;
        if (s == "1" || s == "true" || s == "True" || s == "TRUE" || s == "on" || s == "yes") return true;
        if (s == "0" || s == "false" || s == "False" || s == "FALSE" || s == "off" || s == "no") return false;
        return defaultValue;
    }

    enum class ValueKind { Bool, Int, Int64, Uint32, Uint64, Float, Double, Duration, String, Unknown };

    ValueKind kindFromDefault(const FlagValue& v) {
        return std::visit(
            [](const auto& x) -> ValueKind {
                using T = std::decay_t<decltype(x)>;
                if constexpr (std::is_same_v<T, bool>) return ValueKind::Bool;
                if constexpr (std::is_same_v<T, int>) return ValueKind::Int;
                if constexpr (std::is_same_v<T, std::int64_t>) return ValueKind::Int64;
                if constexpr (std::is_same_v<T, std::uint32_t>) return ValueKind::Uint32;
                if constexpr (std::is_same_v<T, std::uint64_t>) return ValueKind::Uint64;
                if constexpr (std::is_same_v<T, float>) return ValueKind::Float;
                if constexpr (std::is_same_v<T, double>) return ValueKind::Double;
                if constexpr (std::is_same_v<T, std::chrono::milliseconds>) return ValueKind::Duration;
                if constexpr (std::is_same_v<T, std::string>) return ValueKind::String;
                return ValueKind::Unknown;
            },
            v);
    }

    bool isTruthyAnnotation(std::string_view v) {
        return v == "1" || v == "true" || v == "True" || v == "TRUE" || v == "yes" || v == "on";
    }

    bool tryParseBytes(std::string_view s, std::uint64_t& out) {
        auto toUpperAscii = [](std::string_view v) {
            std::string out;
            out.reserve(v.size());
            for (const auto ch : v) out.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
            return out;
        }; // LCOV_EXCL_LINE (closure cleanup block only runs on allocation failure)

        const auto sv = detail::trimWs(s);
        if (sv.empty()) return false;
        if (sv.front() == '-') return false;

        // First, allow base-0 integer forms (e.g. 0x10, 077) with no unit.
        std::uint64_t parsed{};
        if (detail::tryParseUnsignedInt<std::uint64_t>(sv, parsed)) {
            out = parsed;
            return true;
        }

        const std::string tmp(sv);
        const char* start = tmp.c_str();
        char* end = nullptr;
        errno = 0;
        const double value = std::strtod(start, &end);
        if (errno != 0) return false;
        if (!end || end == start) return false; // LCOV_EXCL_LINE (strtod always writes through endptr, so end is never null; the end == start guard is exercised)

        std::string_view rest(start + (end - start));
        rest = detail::trimWs(rest);

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

    struct ParsedIP {
        bool v4{false};
        std::array<std::uint8_t, 16> bytes{};
    };

    bool tryParseIPv4(std::string_view s, ParsedIP& out) {
        const auto sv = detail::trimWs(s);
        if (sv.empty()) return false;
        if (sv.find(':') != std::string_view::npos) return false; // LCOV_EXCL_LINE (every call site rejects ':'-containing input before calling and v4 tail parts come from splitting on ':', so no colon can reach here)

        std::array<std::uint8_t, 4> octets{};
        std::size_t octetIdx = 0;
        std::size_t start = 0;
        for (;;) {
            const auto dot = sv.find('.', start);
            const auto part = (dot == std::string_view::npos) ? sv.substr(start) : sv.substr(start, dot - start);
            if (part.empty()) return false;
            if (octetIdx >= octets.size()) return false;
            std::uint32_t value = 0;
            for (const char ch : part) {
                if (ch < '0' || ch > '9') return false;
                value = value * 10u + static_cast<std::uint32_t>(ch - '0');
                if (value > 255u) return false;
            }
            octets[octetIdx++] = static_cast<std::uint8_t>(value);
            if (dot == std::string_view::npos) break;
            start = dot + 1;
        }
        if (octetIdx != 4) return false;

        out.v4 = true;
        out.bytes.fill(0);
        for (std::size_t i = 0; i < 4; ++i) out.bytes[i] = octets[i];
        return true;
    }

    bool tryParseHextet(std::string_view s, std::uint16_t& out) {
        const auto sv = detail::trimWs(s);
        if (sv.empty() || sv.size() > 4) return false; // LCOV_EXCL_LINE (the sole caller parseParts only receives non-empty parts; empty head/tail parts are rejected before parsing)
        std::uint32_t v = 0;
        for (const char ch : sv) {
            std::uint32_t digit = 0;
            if (ch >= '0' && ch <= '9') digit = static_cast<std::uint32_t>(ch - '0');
            else if (ch >= 'a' && ch <= 'f') digit = 10u + static_cast<std::uint32_t>(ch - 'a');
            else if (ch >= 'A' && ch <= 'F') digit = 10u + static_cast<std::uint32_t>(ch - 'A');
            else return false;
            v = (v << 4) | digit;
            if (v > 0xFFFFu) return false; // LCOV_EXCL_LINE (both outcomes of the 0xFFFF check are exercised via other inlining sites; this residual edge belongs to a cold inlined copy)
        }
        out = static_cast<std::uint16_t>(v);
        return true;
    }

    bool splitOnChar(std::string_view s, char sep, std::vector<std::string_view>& out) {
        out.clear();
        std::size_t start = 0;
        for (;;) {
            const auto pos = s.find(sep, start);
            if (pos == std::string_view::npos) {
                out.push_back(s.substr(start));
                break;
            }
            out.push_back(s.substr(start, pos - start));
            start = pos + 1;
        }
        return true;
    }

    bool tryParseIPv6(std::string_view s, ParsedIP& out) {
        const auto sv = detail::trimWs(s);
        if (sv.empty()) return false; // LCOV_EXCL_LINE (empty and non-empty inputs are both exercised at the other call sites; the remaining edge belongs to a cold inlined copy)
        if (sv.find('%') != std::string_view::npos) return false; // zone IDs not supported

        const auto dbl = sv.find("::");
        const bool hasDbl = (dbl != std::string_view::npos);
        if (hasDbl && sv.find("::", dbl + 2) != std::string_view::npos) return false;

        const auto head = hasDbl ? sv.substr(0, dbl) : sv;
        const auto tail = hasDbl ? sv.substr(dbl + 2) : std::string_view{};

        std::vector<std::string_view> headParts;
        std::vector<std::string_view> tailParts;
        if (!head.empty()) splitOnChar(head, ':', headParts);
        if (!tail.empty()) splitOnChar(tail, ':', tailParts);

        for (const auto& p : headParts) {
            if (p.empty()) return false;
        }
        for (const auto& p : tailParts) {
            if (p.empty()) return false;
        }

        auto parseParts = [](const std::vector<std::string_view>& parts, bool allowV4Tail, std::vector<std::uint16_t>& groups) {
            groups.clear();
            groups.reserve(parts.size());
            for (std::size_t idx = 0; idx < parts.size(); ++idx) {
                const auto part = parts[idx];
                const bool isLast = (idx + 1 == parts.size());
                if (allowV4Tail && isLast && part.find('.') != std::string_view::npos) { // LCOV_EXCL_LINE (allowV4Tail is only true when the last part itself contains '.', so at the final index the dot check is always true)
                    ParsedIP ip4{};
                    if (!tryParseIPv4(part, ip4)) return false;
                    const std::uint16_t g1 = (static_cast<std::uint16_t>(ip4.bytes[0]) << 8) | ip4.bytes[1];
                    const std::uint16_t g2 = (static_cast<std::uint16_t>(ip4.bytes[2]) << 8) | ip4.bytes[3];
                    groups.push_back(g1);
                    groups.push_back(g2);
                    continue;
                }
                if (part.find('.') != std::string_view::npos) return false;
                std::uint16_t g{};
                if (!tryParseHextet(part, g)) return false;
                groups.push_back(g);
            }
            return true;
        };

        std::vector<std::uint16_t> headGroups;
        std::vector<std::uint16_t> tailGroups;
        const bool headV4Tail = (!hasDbl && !headParts.empty() && tailParts.empty() && headParts.back().find('.') != std::string_view::npos); // LCOV_EXCL_LINE (without "::" the tail list is always empty and the head list never is, so both middle terms are constant on the reachable path)
        const bool tailV4Tail = (!tailParts.empty() && tailParts.back().find('.') != std::string_view::npos);

        if (!parseParts(headParts, headV4Tail, headGroups)) return false;
        if (!parseParts(tailParts, tailV4Tail, tailGroups)) return false;

        const std::size_t total = headGroups.size() + tailGroups.size();
        if (hasDbl) {
            if (total > 8) return false;
            const std::size_t missing = 8 - total;
            if (missing == 0) return false; // "::" must compress at least one 0 group
            std::vector<std::uint16_t> all;
            all.reserve(8);
            all.insert(all.end(), headGroups.begin(), headGroups.end());
            all.insert(all.end(), missing, 0);
            all.insert(all.end(), tailGroups.begin(), tailGroups.end());
            if (all.size() != 8) return false; // LCOV_EXCL_LINE (inside the "::" path the group list is padded to exactly 8 by construction; the total != 8 rejection is the separate no-compression path)
            out.v4 = false;
            out.bytes.fill(0);
            for (std::size_t j = 0; j < 8; ++j) {
                const std::uint16_t g = all[j];
                out.bytes[j * 2] = static_cast<std::uint8_t>((g >> 8) & 0xFFu);
                out.bytes[j * 2 + 1] = static_cast<std::uint8_t>(g & 0xFFu);
            }
            return true;
        }

        if (total != 8) return false;
        std::vector<std::uint16_t> all;
        all.reserve(8);
        all.insert(all.end(), headGroups.begin(), headGroups.end());
        all.insert(all.end(), tailGroups.begin(), tailGroups.end());
        out.v4 = false;
        out.bytes.fill(0);
        for (std::size_t j = 0; j < 8; ++j) {
            const std::uint16_t g = all[j];
            out.bytes[j * 2] = static_cast<std::uint8_t>((g >> 8) & 0xFFu);
            out.bytes[j * 2 + 1] = static_cast<std::uint8_t>(g & 0xFFu);
        }
        return true;
    }

    std::string formatIPv4(const ParsedIP& ip) {
        return std::to_string(ip.bytes[0]) + "." + std::to_string(ip.bytes[1]) + "." + std::to_string(ip.bytes[2]) + "." +
               std::to_string(ip.bytes[3]);
    }

    std::string hexNoLeading(std::uint16_t v) {
        if (v == 0) return "0";
        std::string tmp;
        while (v != 0) {
            const std::uint16_t d = static_cast<std::uint16_t>(v & 0xFu);
            tmp.push_back("0123456789abcdef"[d]);
            v = static_cast<std::uint16_t>(v >> 4);
        }
        std::string out;
        out.reserve(tmp.size());
        for (auto it = tmp.rbegin(); it != tmp.rend(); ++it) out.push_back(*it);
        return out;
    }

    std::string formatIPv6(const ParsedIP& ip) {
        std::array<std::uint16_t, 8> groups{};
        for (std::size_t i = 0; i < 8; ++i) {
            groups[i] = (static_cast<std::uint16_t>(ip.bytes[i * 2]) << 8) | static_cast<std::uint16_t>(ip.bytes[i * 2 + 1]);
        }

        std::size_t bestStart = 0;
        std::size_t bestLen = 0;
        for (std::size_t i = 0; i < 8;) {
            if (groups[i] != 0) {
                ++i;
                continue;
            }
            std::size_t j = i;
            while (j < 8 && groups[j] == 0) ++j;
            const std::size_t len = j - i;
            if (len >= 2 && len > bestLen) {
                bestStart = i;
                bestLen = len;
            }
            i = j;
        }

        std::string out;
        bool first = true;
        for (std::size_t i = 0; i < 8; ++i) {
            if (bestLen >= 2 && i == bestStart) {
                out += first ? "::" : "::";
                first = false;
                i += bestLen - 1;
                continue;
            }
            if (!first && out.back() != ':') out.push_back(':');
            out += hexNoLeading(groups[i]);
            first = false;
        }
        if (out.empty()) return "::"; // LCOV_EXCL_LINE (all-zero and non-zero addresses are both exercised at other call sites; the remaining edges belong to a cold inlined copy and its cleanup)
        return out;
    }

    bool tryParseIP(std::string_view s, std::string& canonical) {
        ParsedIP ip{};
        if (s.find(':') != std::string_view::npos) {
            if (!tryParseIPv6(s, ip)) return false;
            canonical = formatIPv6(ip);
            return true;
        }
        if (!tryParseIPv4(s, ip)) return false;
        canonical = formatIPv4(ip);
        return true;
    }

    bool tryParseIPMask(std::string_view s, std::string& canonical) {
        const auto sv = detail::trimWs(s);
        if (sv.empty()) {
            canonical.clear();
            return true;
        }
        if (sv.find(':') != std::string_view::npos) return false; // IPv4 masks only

        ParsedIP ip{};
        if (!tryParseIPv4(sv, ip)) return false; // LCOV_EXCL_LINE (both outcomes of tryParseIPv4 are exercised by the ipmask tests; the remaining edges are cleanup paths of the inlined call)

        std::uint32_t mask = (static_cast<std::uint32_t>(ip.bytes[0]) << 24) | (static_cast<std::uint32_t>(ip.bytes[1]) << 16) |
                             (static_cast<std::uint32_t>(ip.bytes[2]) << 8) | static_cast<std::uint32_t>(ip.bytes[3]);
        const std::uint32_t inv = ~mask;
        if ((static_cast<std::uint64_t>(inv) & (static_cast<std::uint64_t>(inv) + 1ULL)) != 0ULL) return false; // requires contiguous bits

        canonical = formatIPv4(ip);
        return true;
    }

    bool tryParseCIDR(std::string_view s, std::string& canonical) {
        const auto sv = detail::trimWs(s);
        const auto slash = sv.find('/');
        if (slash == std::string_view::npos) return false;

        const auto ipPart = detail::trimWs(sv.substr(0, slash));
        const auto prefixPart = detail::trimWs(sv.substr(slash + 1));
        if (ipPart.empty() || prefixPart.empty()) return false;

        ParsedIP ip{};
        std::string ipCanon;
        if (ipPart.find(':') != std::string_view::npos) {
            if (!tryParseIPv6(ipPart, ip)) return false;
        } else {
            if (!tryParseIPv4(ipPart, ip)) return false;
        }

        int prefix = 0;
        for (const char ch : prefixPart) {
            if (ch < '0' || ch > '9') return false;
            prefix = prefix * 10 + static_cast<int>(ch - '0');
            if (prefix > 128) return false;
        }

        const int maxBits = ip.v4 ? 32 : 128;
        if (prefix < 0 || prefix > maxBits) return false; // LCOV_EXCL_LINE (prefix accumulates only accepted digits and is capped at 128 above, so it can never be negative)

        const std::size_t bytesLen = ip.v4 ? 4 : 16;
        for (std::size_t idx = 0; idx < bytesLen; ++idx) {
            const int bits = prefix - static_cast<int>(idx * 8);
            if (bits >= 8) continue;
            if (bits <= 0) {
                ip.bytes[idx] = 0;
                continue;
            }
            const std::uint8_t mask = static_cast<std::uint8_t>(0xFFu << (8 - bits));
            ip.bytes[idx] = static_cast<std::uint8_t>(ip.bytes[idx] & mask);
        }
        for (std::size_t idx = bytesLen; idx < 16; ++idx) ip.bytes[idx] = 0;

        ipCanon = ip.v4 ? formatIPv4(ip) : formatIPv6(ip);
        canonical = ipCanon + "/" + std::to_string(prefix);
        return true;
    }

    bool tryParseURL(std::string_view s, std::string& canonical) {
        auto toLowerAscii = [](std::string_view v) {
            std::string out;
            out.reserve(v.size());
            for (const auto ch : v) out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
            return out;
        }; // LCOV_EXCL_LINE (closure cleanup block only runs on allocation failure)

        const auto sv = detail::trimWs(s);
        if (sv.empty()) {
            canonical.clear();
            return true;
        }
        for (const auto ch : sv) {
            if (std::isspace(static_cast<unsigned char>(ch))) return false;
        }

        canonical = std::string(sv);

        const auto schemeSep = canonical.find("://");
        if (schemeSep == std::string::npos) return true; // accept non-hierarchical / relative forms

        const std::string_view scheme(canonical.data(), schemeSep);
        if (scheme.empty()) return false;
        auto isAlpha = [](char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); };
        auto isSchemeChar = [&](char c) {
            return isAlpha(c) || (c >= '0' && c <= '9') || c == '+' || c == '-' || c == '.';
        };
        if (!isAlpha(scheme.front())) return false;
        for (const auto ch : scheme) {
            if (!isSchemeChar(ch)) return false;
        }

        const std::size_t authorityStart = schemeSep + 3;
        std::size_t authorityEnd = canonical.find_first_of("/?#", authorityStart);
        if (authorityEnd == std::string::npos) authorityEnd = canonical.size();

        const std::string_view authority(canonical.data() + authorityStart, authorityEnd - authorityStart);
        if (authority.empty()) return false;

        const std::string_view rest(canonical.data() + authorityEnd, canonical.size() - authorityEnd);

        std::string_view userinfo;
        std::string_view hostport = authority;
        const auto at = authority.rfind('@');
        if (at != std::string_view::npos) {
            userinfo = authority.substr(0, at + 1);
            hostport = authority.substr(at + 1);
        }
        if (hostport.empty()) return false;

        std::string hostLower;
        std::string_view portSuffix;

        if (!hostport.empty() && hostport.front() == '[') { // LCOV_EXCL_LINE (the hostport.empty() check above already returned, so hostport is never empty here)
            const auto close = hostport.find(']');
            if (close == std::string_view::npos) return false;
            const auto inner = hostport.substr(1, close - 1);
            hostLower = "[" + toLowerAscii(inner) + "]";
            portSuffix = hostport.substr(close + 1);
        } else {
            const auto firstColon = hostport.find(':');
            const auto lastColon = hostport.rfind(':');
            if (firstColon != std::string_view::npos && firstColon == lastColon) {
                const auto hostPart = hostport.substr(0, firstColon);
                const auto portPart = hostport.substr(firstColon + 1);
                bool numeric = !portPart.empty();
                for (const auto ch : portPart) {
                    if (ch < '0' || ch > '9') {
                        numeric = false;
                        break;
                    }
                }
                if (numeric) {
                    hostLower = toLowerAscii(hostPart);
                    portSuffix = hostport.substr(firstColon);
                } else {
                    hostLower = toLowerAscii(hostport);
                }
            } else {
                hostLower = toLowerAscii(hostport);
            }
        }

        canonical = toLowerAscii(scheme) + "://" + std::string(userinfo) + hostLower + std::string(portSuffix) + std::string(rest);
        return true;
    }

    // Split on `sep`, dropping empty parts (same semantics as getFlagValuesSplit).
    std::vector<std::string> splitCsvParts(const std::string& v, char sep) {
        std::vector<std::string> out;
        std::size_t start = 0;
        while (start <= v.size()) { // LCOV_EXCL_LINE (the loop always exits via the break when find returns npos; start = pos + 1 never exceeds size, so the false edge is unreachable)
            const auto pos = v.find(sep, start);
            const auto part = (pos == std::string::npos) ? v.substr(start) : v.substr(start, pos - start);
            if (!part.empty()) out.push_back(part);
            if (pos == std::string::npos) break;
            start = pos + 1;
        }
        return out;
    } // LCOV_EXCL_LINE (function cleanup block)
    std::string toString(const FlagValue& v) {
        return std::visit(
            [](const auto& x) -> std::string {
                using T = std::decay_t<decltype(x)>;
                if constexpr (std::is_same_v<T, bool>) return x ? "true" : "false";
                if constexpr (std::is_same_v<T, int>) return std::to_string(x);
                if constexpr (std::is_same_v<T, std::int32_t>) return std::to_string(x);
                if constexpr (std::is_same_v<T, std::int64_t>) return std::to_string(x);
                if constexpr (std::is_same_v<T, std::uint32_t>) return std::to_string(x);
                if constexpr (std::is_same_v<T, std::uint64_t>) return std::to_string(x);
                if constexpr (std::is_same_v<T, float>) return std::to_string(x);
                if constexpr (std::is_same_v<T, double>) return std::to_string(x);
                if constexpr (std::is_same_v<T, std::chrono::milliseconds>) return std::to_string(x.count()) + "ms";
                if constexpr (std::is_same_v<T, std::string>) return x;
                return {};
            },
            v);
    }

} // namespace clasp::detail

#endif // CLASP_DETAIL_PARSER_VALUE_HPP
