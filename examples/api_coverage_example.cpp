#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "clasp/clasp.hpp"

namespace {

struct SimpleValue final : clasp::Value {
    std::string value{"info"};

    std::string type() const override { return "level"; }
    std::string string() const override { return value; }
    std::optional<std::string> set(std::string_view s) override {
        value = std::string(s);
        return std::nullopt;
    }
};

} // namespace

int main() {
    clasp::Command root("app", "API coverage");

    root.enableColor();

    root.disableFlagParsing(false)
        .allowUnknownFlags(false)
        .shortFlagGrouping(true)
        .boolNegation(true)
        .normalizeFlagKeys([](std::string k) { return k; })
        .completionDirective(4)
        .traverseChildren(false)
        .bindEnv("--name", "APP_NAME")
        .configFile("config.yml")
        .configFileFlag("config")
        .aliases({"a1", "a2"})
        .annotation("k", "v")
        .addAlias("a3")
        .addGroup("grp", "Group")
        .groupId("grp")
        .withPersistentFlag("--verbose", "-v", "Verbose");

    root.withPersistentFlag("--count", "", "count", "Count", 0)
        .withPersistentFlag("--limit", "", "limit", "Bytes", std::uint64_t(0))
        .withPersistentFlag("--ip", "", "ip", "IP", std::string(""))
        .withPersistentFlag("--mask", "", "mask", "Mask", std::string(""))
        .withPersistentFlag("--cidr", "", "cidr", "CIDR", std::string(""))
        .withPersistentFlag("--ipnet", "", "ipnet", "IPNet", std::string(""))
        .withPersistentFlag("--url", "", "url", "URL", std::string(""));

    root.markPersistentFlagHidden("--verbose");
    root.markPersistentFlagRequired("--verbose");
    root.markPersistentFlagDeprecated("--verbose", "deprecated");

    clasp::Command sub("sub", "Sub");
    sub.withFlag("--name", "-n", "name", "Name", std::string("x"));
    sub.markFlagRequired("--name");
    sub.markFlagHidden("--name");
    sub.markFlagDeprecated("--name", "deprecated");
    sub.markFlagAnnotation("--name", "anno", "1");
    sub.markFlagFilename("--name", {"txt"});
    sub.markFlagDirname("--name");

    sub.markFlagsOneRequired({"--name"});
    sub.markFlagsRequiredTogether({"--name"});

    sub.validArgs({"a", "b"});
    sub.validArgsFunction([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&, std::string_view) {
        return std::vector<std::string>{"a", "b"};
    });
    sub.registerFlagCompletion("--name",
                               [](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&, std::string_view) {
                                   return std::vector<std::string>{"x", "y"};
                               });

    SimpleValue v;
    sub.bindFlagValue("--name", v);
    sub.withValueFlag("--level", "-l", "Level", v);

    root.addCommand(std::move(sub));

    std::cout << "ok\n";
    return 0;
}
