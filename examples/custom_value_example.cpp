#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "clasp/clasp.hpp"

class LevelValue final : public clasp::Value {
public:
    explicit LevelValue(std::string v) : value_(std::move(v)) {}

    std::string type() const override { return "level"; }
    std::string string() const override { return value_; }

    std::optional<std::string> set(std::string_view v) override {
        if (v == "debug" || v == "info" || v == "warn" || v == "error") {
            value_ = std::string(v);
            return std::nullopt;
        }
        return std::string("invalid level: ") + std::string(v);
    }

private:
    std::string value_;
};

int main(int argc, char** argv) {
    clasp::Command root("app", "Custom Value example");
    LevelValue level("info");

    // External sources: allow `--config <file>` and env `APP_LEVEL`.
    root.withPersistentFlag("--config", "", "config", "Config file", std::string(""));
    root.configFileFlag("--config");
    root.bindEnv("--level", "APP_LEVEL");

    clasp::Command show("show", "Print effective level");
    show.withValueFlag("--level", "-l", "Log level", level);
    show.action([&](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
        std::cout << level.string() << "\n";
        return 0;
    });

    root.addCommand(std::move(show));
    return root.run(argc, argv);
}
