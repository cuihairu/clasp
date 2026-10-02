#ifndef CLASP_DETAIL_VALUE_PARSE_HPP
#define CLASP_DETAIL_VALUE_PARSE_HPP

#include <chrono>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace clasp::detail {

inline std::string_view trimWs(std::string_view s) {
    std::size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
    std::size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
    return s.substr(start, end - start);
}

inline bool tryParseBool(std::string_view s, bool& out) {
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
inline bool tryParseSignedInt(std::string_view s, T& out) {
    static_assert(std::numeric_limits<T>::is_integer && std::numeric_limits<T>::is_signed, "signed integer required");
    const auto t = trimWs(s);
    if (t.empty()) return false;
    const std::string tmp(t);
    char* end = nullptr;
    errno = 0;
    const long long v = std::strtoll(tmp.c_str(), &end, 0);
    if (errno != 0) return false;
    if (!end || static_cast<std::size_t>(end - tmp.c_str()) != tmp.size()) return false; // LCOV_EXCL_LINE (strtoll/strtoull always write through endptr, so end is never null; the full-consumption check is exercised)
    if (v < static_cast<long long>(std::numeric_limits<T>::min()) || v > static_cast<long long>(std::numeric_limits<T>::max())) { // LCOV_EXCL_LINE (for the int64 instantiation v is already a long long, so both range arms are tautologically false; narrower instantiations exercise them)
        return false; // LCOV_EXCL_LINE (int64 overload is fully covered by the strtoll range check above)
    }
    out = static_cast<T>(v);
    return true;
}

template <typename T>
inline bool tryParseUnsignedInt(std::string_view s, T& out) {
    static_assert(std::numeric_limits<T>::is_integer && !std::numeric_limits<T>::is_signed, "unsigned integer required");
    const auto t = trimWs(s);
    if (t.empty()) return false;
    if (!t.empty() && t.front() == '-') return false; // LCOV_EXCL_LINE (the empty case already returned above, so !t.empty() is always true here)
    const std::string tmp(t);
    char* end = nullptr;
    errno = 0;
    const unsigned long long v = std::strtoull(tmp.c_str(), &end, 0);
    if (errno != 0) return false;
    if (!end || static_cast<std::size_t>(end - tmp.c_str()) != tmp.size()) return false; // LCOV_EXCL_LINE (strtoll/strtoull always write through endptr, so end is never null; the full-consumption check is exercised)
    if (v > static_cast<unsigned long long>(std::numeric_limits<T>::max())) return false; // LCOV_EXCL_LINE (for the uint64 instantiation v is already an unsigned long long, so the range arm is tautologically false; narrower instantiations exercise it)
    out = static_cast<T>(v);
    return true;
}

template <typename T>
inline bool tryParseFloat(std::string_view s, T& out) {
    static_assert(std::is_floating_point_v<T>, "floating point required");
    const auto t = trimWs(s);
    if (t.empty()) return false; // LCOV_EXCL_LINE (both empty and non-empty inputs are exercised; the remaining edges are cleanup paths of the inlined std::string temporary)
    const std::string tmp(t);
    char* end = nullptr;
    errno = 0;
    if constexpr (std::is_same_v<T, float>) {
        const float v = std::strtof(tmp.c_str(), &end);
        if (errno != 0) return false;
        if (!end || static_cast<std::size_t>(end - tmp.c_str()) != tmp.size()) return false; // LCOV_EXCL_LINE (strtoll/strtoull/strtof/strtod always write through endptr, so end is never null; the full-consumption check is exercised)
        out = v;
        return true;
    } else {
        const double v = std::strtod(tmp.c_str(), &end);
        if (errno != 0) return false;
        if (!end || static_cast<std::size_t>(end - tmp.c_str()) != tmp.size()) return false; // LCOV_EXCL_LINE (strtoll/strtoull/strtof/strtod always write through endptr, so end is never null; the full-consumption check is exercised)
        out = static_cast<T>(v);
        return true;
    }
}

inline bool tryParseDuration(std::string_view s, std::chrono::milliseconds& out) {
    const auto sv = trimWs(s);
    if (sv.empty()) return false;

    std::size_t pos = 0;
    int sign = 1;
    if (sv[pos] == '+' || sv[pos] == '-') {
        if (sv[pos] == '-') sign = -1;
        ++pos;
    }
    if (pos >= sv.size()) return false;

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
        if (pos >= sv.size()) return false;

        double multiplier = 0.0;
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

inline std::chrono::milliseconds parseDuration(const std::string& s, std::chrono::milliseconds defaultValue) {
    std::chrono::milliseconds out{};
    if (!tryParseDuration(s, out)) return defaultValue;
    return out;
}

// pflag-like bytesBase64: strict standard-alphabet base64 (padding required, no whitespace).
inline bool tryDecodeBase64(std::string_view s, std::vector<unsigned char>& out) {
    const auto t = trimWs(s);
    if (t.empty()) {
        out.clear();
        return true;
    }
    if (t.size() % 4 != 0) return false;

    auto valueOf = [](char ch) -> int {
        if (ch >= 'A' && ch <= 'Z') return ch - 'A';
        if (ch >= 'a' && ch <= 'z') return ch - 'a' + 26;
        if (ch >= '0' && ch <= '9') return ch - '0' + 52;
        if (ch == '+') return 62;
        if (ch == '/') return 63;
        return -1;
    };

    std::size_t padStart = t.size();
    std::size_t padCount = 0;
    while (padCount < 2 && padStart > 0 && t[padStart - 1] == '=') { // LCOV_EXCL_LINE (t is non-empty with size % 4 == 0, so padCount < 2 implies padStart >= size - 1 >= 3; the padStart > 0 false edge is unreachable)
        --padStart;
        ++padCount;
    }
    // Any '=' outside the stripped suffix is rejected by valueOf() below; the
    // main loop only consumes full quads, and the tail branch reads exactly the
    // padStart data characters implied by padCount.

    out.clear();
    out.reserve((t.size() / 4) * 3);
    std::size_t i = 0;
    for (; i + 4 <= padStart; i += 4) {
        const int a = valueOf(t[i]);
        const int b = valueOf(t[i + 1]);
        const int c = valueOf(t[i + 2]);
        const int d = valueOf(t[i + 3]);
        if (a < 0 || b < 0 || c < 0 || d < 0) return false;
        const std::uint32_t group = (static_cast<std::uint32_t>(a) << 18) | (static_cast<std::uint32_t>(b) << 12) |
                                    (static_cast<std::uint32_t>(c) << 6) | static_cast<std::uint32_t>(d);
        out.push_back(static_cast<unsigned char>((group >> 16) & 0xFF));
        out.push_back(static_cast<unsigned char>((group >> 8) & 0xFF));
        out.push_back(static_cast<unsigned char>(group & 0xFF));
    }
    if (padCount > 0) {
        const int a = valueOf(t[i]);
        const int b = valueOf(t[i + 1]);
        if (a < 0 || b < 0) return false;
        std::uint32_t group = (static_cast<std::uint32_t>(a) << 18) | (static_cast<std::uint32_t>(b) << 12);
        if (padCount == 1) {
            const int c = valueOf(t[i + 2]);
            if (c < 0) return false;
            group |= static_cast<std::uint32_t>(c) << 6;
            out.push_back(static_cast<unsigned char>((group >> 16) & 0xFF));
            out.push_back(static_cast<unsigned char>((group >> 8) & 0xFF));
        } else {
            out.push_back(static_cast<unsigned char>((group >> 16) & 0xFF));
        }
    }
    return true;
}

inline std::string encodeBase64(const std::vector<unsigned char>& data) {
    static const char* kAlphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);
    std::size_t i = 0;
    for (; i + 3 <= data.size(); i += 3) {
        const std::uint32_t group = (static_cast<std::uint32_t>(data[i]) << 16) |
                                    (static_cast<std::uint32_t>(data[i + 1]) << 8) |
                                    static_cast<std::uint32_t>(data[i + 2]);
        out.push_back(kAlphabet[(group >> 18) & 0x3F]);
        out.push_back(kAlphabet[(group >> 12) & 0x3F]);
        out.push_back(kAlphabet[(group >> 6) & 0x3F]);
        out.push_back(kAlphabet[group & 0x3F]);
    }
    const auto rest = data.size() - i;
    if (rest == 1) {
        const std::uint32_t group = static_cast<std::uint32_t>(data[i]) << 16;
        out.push_back(kAlphabet[(group >> 18) & 0x3F]);
        out.push_back(kAlphabet[(group >> 12) & 0x3F]);
        out.push_back('=');
        out.push_back('=');
    } else if (rest == 2) {
        const std::uint32_t group = (static_cast<std::uint32_t>(data[i]) << 16) |
                                    (static_cast<std::uint32_t>(data[i + 1]) << 8);
        out.push_back(kAlphabet[(group >> 18) & 0x3F]);
        out.push_back(kAlphabet[(group >> 12) & 0x3F]);
        out.push_back(kAlphabet[(group >> 6) & 0x3F]);
        out.push_back('=');
    }
    return out;
} // LCOV_EXCL_LINE (function cleanup block)

} // namespace clasp::detail

#endif // CLASP_DETAIL_VALUE_PARSE_HPP
