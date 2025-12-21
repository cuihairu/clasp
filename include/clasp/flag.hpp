#ifndef CLASP_FLAG_HPP
#define CLASP_FLAG_HPP

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>

namespace clasp {
using FlagValue = std::variant<bool, int, std::int64_t, std::uint64_t, float, double, std::chrono::milliseconds, std::string>;

class Flag {
public:
    explicit Flag(std::string longName,
                  std::string shortName,
                  std::string description,
                  std::string varName,
                  FlagValue defaultValue)
        : longName_(std::move(longName)),
          shortName_(std::move(shortName)),
          varName_(std::move(varName)),
          description_(std::move(description)),
          defaultValue_(std::move(defaultValue)) {}

    explicit Flag(std::string longName, std::string shortName, std::string description)
        : Flag(std::move(longName), std::move(shortName), std::move(description), "", false) {}

    [[nodiscard]] const std::string& longName() const { return longName_; }
    [[nodiscard]] const std::string& shortName() const { return shortName_; }
    [[nodiscard]] const std::string& description() const { return description_; }
    [[nodiscard]] const std::string& varName() const { return varName_; }
    [[nodiscard]] FlagValue defaultValue() const { return defaultValue_; }

    [[nodiscard]] bool required() const { return required_; }
    [[nodiscard]] bool hidden() const { return hidden_; }
    [[nodiscard]] const std::string& deprecated() const { return deprecated_; }
    [[nodiscard]] const std::unordered_map<std::string, std::string>& annotations() const { return annotations_; }
    [[nodiscard]] const std::optional<std::string>& noOptDefaultValue() const { return noOptDefaultValue_; }

    void setRequired(bool v) { required_ = v; }
    void setHidden(bool v) { hidden_ = v; }
    void setDeprecated(std::string msg) { deprecated_ = std::move(msg); }
    void setAnnotation(std::string key, std::string value) { annotations_[std::move(key)] = std::move(value); }
    void setNoOptDefaultValue(std::string value) { noOptDefaultValue_ = std::move(value); }

    template <typename T>
    static FlagValue convertToFlagValue(const std::string& str);

private:
    std::string longName_;   // --help ,--config
    std::string shortName_;  // -h,-c
    std::string varName_;    // configName
    std::string description_;// config file
    FlagValue defaultValue_; // config/dev.yaml
    bool required_{false};
    bool hidden_{false};
    std::string deprecated_;
    std::unordered_map<std::string, std::string> annotations_;
    std::optional<std::string> noOptDefaultValue_;
};

} // namespace clasp

#endif // CLASP_FLAG_HPP
