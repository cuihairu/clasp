#include "clasp/flag.hpp"
#include "clasp/detail/value_parse.hpp"

#include <stdexcept>

namespace clasp {

template <>
FlagValue Flag::convertToFlagValue<bool>(const std::string& str) {
    if (detail::trimWs(str).empty()) return false;
    bool out = false;
    if (!detail::tryParseBool(str, out)) throw std::invalid_argument("invalid bool");
    return out;
}

template <>
FlagValue Flag::convertToFlagValue<int>(const std::string& str) {
    int out = 0;
    if (!detail::tryParseSignedInt<int>(str, out)) throw std::invalid_argument("invalid int");
    return out;
}

template <>
FlagValue Flag::convertToFlagValue<std::int64_t>(const std::string& str) {
    std::int64_t out = 0;
    if (!detail::tryParseSignedInt<std::int64_t>(str, out)) throw std::invalid_argument("invalid int64");
    return out;
}

template <>
FlagValue Flag::convertToFlagValue<std::uint32_t>(const std::string& str) {
    std::uint32_t out = 0;
    if (!detail::tryParseUnsignedInt<std::uint32_t>(str, out)) throw std::invalid_argument("invalid uint32");
    return out;
}

template <>
FlagValue Flag::convertToFlagValue<std::uint64_t>(const std::string& str) {
    std::uint64_t out = 0;
    if (!detail::tryParseUnsignedInt<std::uint64_t>(str, out)) throw std::invalid_argument("invalid uint64");
    return out;
}

template <>
FlagValue Flag::convertToFlagValue<float>(const std::string& str) {
    float out = 0.0f;
    if (!detail::tryParseFloat<float>(str, out)) throw std::invalid_argument("invalid float");
    return out;
}

template <>
FlagValue Flag::convertToFlagValue<double>(const std::string& str) {
    double out = 0.0;
    if (!detail::tryParseFloat<double>(str, out)) throw std::invalid_argument("invalid double");
    return out;
}

template <>
FlagValue Flag::convertToFlagValue<std::chrono::milliseconds>(const std::string& str) {
    std::chrono::milliseconds out{};
    if (!detail::tryParseDuration(str, out)) throw std::invalid_argument("invalid duration");
    return out;
}

template <>
FlagValue Flag::convertToFlagValue<std::string>(const std::string& str) {
    return str;
}

} // namespace clasp
