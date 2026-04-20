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
    if (!end || static_cast<std::size_t>(end - tmp.c_str()) != tmp.size()) return false;
    if (v < static_cast<long long>(std::numeric_limits<T>::min()) || v > static_cast<long long>(std::numeric_limits<T>::max())) {
        return false;
    }
    out = static_cast<T>(v);
    return true;
}

template <typename T>
inline bool tryParseUnsignedInt(std::string_view s, T& out) {
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
inline bool tryParseFloat(std::string_view s, T& out) {
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

} // namespace clasp::detail

#endif // CLASP_DETAIL_VALUE_PARSE_HPP
