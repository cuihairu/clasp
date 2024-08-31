#include "clasp/flag.hpp"

namespace clasp
{
    template<>
    FlagValue Flag::convertToFlagValue<bool>(const std::string& str) {
        if (str.empty())
        {
            return false;
        }
        if (str == "true" || str == "1" || str == "True" || str == "TRUE" || str == "on") {
            return true;
        }
        return false;
    }

    template<>
    FlagValue Flag::convertToFlagValue<int>(const std::string& str) {
        return std::stoi(str);
    }

    template<>
    FlagValue Flag::convertToFlagValue<float>(const std::string& str) {
        return std::stof(str);
    }

    template<>
    FlagValue Flag::convertToFlagValue<std::string>(const std::string& str) {
        return str;
    }
}
