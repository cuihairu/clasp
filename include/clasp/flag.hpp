#ifndef CLASP_FLAG_HPP
#define CLASP_FLAG_HPP

#include <string>
#include <utility>
#include <vector>
#include <unordered_map>
#include <iostream>

namespace clasp {
    using FlagValue = std::variant<bool, int, std::string, float>;
    class Flag {
    public:
        explicit Flag(std::string longName, std::string shortName, std::string description,std::string varName, FlagValue defaultValue)
            : longName_(std::move(longName)), shortName_(std::move(shortName)),varName_(std::move(varName)), description_(std::move(description)), defaultValue_(std::move(defaultValue)) {}
        explicit Flag(std::string longName, std::string shortName, std::string description):Flag(std::move(longName), std::move(shortName),"", std::move(description), false) {}
        [[nodiscard]] const std::string& longName() const { return longName_; }
        [[nodiscard]] const std::string& shortName() const { return shortName_; }
        [[nodiscard]] const std::string& description() const { return description_; }
        [[nodiscard]] const std::string& varName() const { return varName_; }
        [[nodiscard]] FlagValue defaultValue() const { return defaultValue_; }

        template<typename T>
        static FlagValue convertToFlagValue(const std::string& str);
    private:
        std::string longName_;   // --help ,--config
        std::string shortName_;  // -h,-c
        std::string varName_;    // configName
        std::string description_;// config file
        FlagValue defaultValue_; // config/dev.yaml
    };
class FlagSet
{
public:
    void boolVar(std::string longName,bool,std::string description);
    void boolVarP(std::string longName,std::string shortName,bool,std::string description);
    void intVar();
    void stringVar();
    void floatVar();
};
} // namespace Clasp

#endif // CLASP_FLAG_HPP
