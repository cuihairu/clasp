#include <iostream>
#include <string>
#include <vector>

#include "clasp/clasp.hpp"

namespace {

void testBasicFlagMethods() {
    clasp::Command root("app", "Test basic flag methods");

    // Test withFlag - generic flag method (5 params)
    root.withFlag("--message", "-m", "message", "Message", std::string("default"))
        .withFlag("--count", "-c", "count", "Count", 42)
        .withFlag("--debug", "-d", "debug", "Debug", true);

    // Test withCountFlag
    root.withCountFlag("--verbose", "-v", "verbose", "Verbose", 0);

    // Test withBytesFlag
    root.withBytesFlag("--size", "-s", "size", "Size", 1024);

    // Test withIPFlag
    root.withIPFlag("--host", "-h", "host", "Host", std::string("127.0.0.1"));

    // Test withURLFlag
    root.withURLFlag("--url", "-u", "url", "URL", std::string("http://example.com"));

    // Test marking methods
    root.markFlagRequired("--message");
    root.markFlagHidden("--debug");
    root.markFlagDeprecated("--verbose", "Use --verbosity instead");
    root.markFlagAnnotation("--count", "type", "number");
    root.markFlagFilename("--message", {"txt"});
    root.markFlagDirname("--message");
}

void testPersistentFlagMethods() {
    clasp::Command root("app", "Test persistent flags");

    // Test withPersistentFlag with different types (5 params)
    root.withPersistentFlag("--debug", "-d", "debug", "Debug", true)
        .withPersistentFlag("--level", "-l", "level", "Level", 5)
        .withPersistentFlag("--name", "-n", "name", "Name", std::string("default"))
        .withPersistentFlag("--timeout", "-t", "timeout", "Timeout", std::uint64_t(1000))
        .withPersistentFlag("--count", "-c", "count", "Count", 0);

    // Test specialized persistent flag methods
    root.withPersistentCountFlag("--verbose", "-v", "verbose", "Verbose");
    root.withPersistentBytesFlag("--limit", "-l", "limit", "Limit", 1024);
    root.withPersistentIPFlag("--host", "-h", "host", "Host", std::string("127.0.0.1"));
    root.withPersistentIPMaskFlag("--mask", "-m", "mask", "Mask", std::string("255.255.255.0"));
    root.withPersistentCIDRFlag("--cidr", "-c", "cidr", "CIDR", std::string("192.168.1.0/24"));
    root.withPersistentIPNetFlag("--network", "-n", "network", "Network", std::string("192.168.1.0/24"));
    root.withPersistentURLFlag("--url", "-u", "url", "URL", std::string("http://example.com"));

    // Test persistent flag marking methods
    root.markPersistentFlagRequired("--debug");
    root.markPersistentFlagHidden("--level");
    root.markPersistentFlagDeprecated("--timeout", "Use --time instead");
}

void testFlagGrouping() {
    clasp::Command root("app", "Test flag grouping");

    // Test flagsOneRequired
    root.withFlag("--input", "-i", "input", "Input", std::string(""))
        .withFlag("--data", "-d", "data", "Data", std::string(""))
        .markFlagsOneRequired({"--input", "--data"});

    // Test flagsRequiredTogether
    root.withFlag("--user", "-u", "user", "User", std::string(""))
        .withFlag("--pass", "-p", "pass", "Pass", std::string(""))
        .markFlagsRequiredTogether({"--user", "--pass"});
}

void testCommandConfig() {
    clasp::Command root("app", "Test command config");

    // Test various configuration methods
    root.disableFlagParsing(true)
        .allowUnknownFlags(true)
        .shortFlagGrouping(true)
        .boolNegation(true)
        .normalizeFlagKeys([](std::string k) { return k; })
        .completionDirective(4)
        .traverseChildren(false)
        .bindEnv("--name", "APP_NAME")
        .configFile("config.yml")
        .configFileFlag("config")
        .aliases({"alias1", "alias2"})
        .annotation("key", "value")
        .addAlias("alias3")
        .addGroup("group1", "Group 1")
        .groupId("group1")
        .withPersistentFlag("--verbose", "-v", "Verbose");
}

void testCompletionMethods() {
    clasp::Command root("app", "Test completion");

    // Test validArgs
    root.validArgs({"arg1", "arg2", "arg3"});

    // Test validArgsFunction
    root.validArgsFunction([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&, std::string_view) {
        return std::vector<std::string>{"comp1", "comp2"};
    });

    // Test registerFlagCompletion
    root.registerFlagCompletion("--flag",
        [](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&, std::string_view) {
            return std::vector<std::string>{"val1", "val2"};
        });
}

void testCommandWithSubcommands() {
    clasp::Command root("app", "Test subcommands");

    // Create and configure subcommands
    clasp::Command cmd1("cmd1", "Command 1");
    cmd1.withFlag("--name", "-n", "name", "Name", std::string("default"))
        .withFlag("--count", "-c", "count", "Count", 10);

    clasp::Command cmd2("cmd2", "Command 2");
    cmd2.withFlag("--debug", "-d", "debug", "Debug", true)
        .withCountFlag("--verbose", "-v", "verbose", "Verbose", 0);

    // Add subcommands to root
    root.addCommand(std::move(cmd1)).addCommand(std::move(cmd2));
}

void testHelpConfiguration() {
    clasp::Command root("app", "Test help configuration");

    // Test disableHelpCommand
    root.disableHelpCommand(true);

    // Test enableHelp with config
    clasp::Command::HelpConfig cfg;
    cfg.addHelpCommand = true;
    cfg.helpCommandName = "help2";
    root.enableHelp(cfg);

    // Test enableHelp without config (default config)
    root.enableHelp();
}

void testTraversalMethods() {
    clasp::Command root("app", "Test traversal");

    // Test traverseChildren
    root.traverseChildren(true);

    // Test completion directive
    root.completionDirective(4);

    // Create subcommand for traversal
    clasp::Command sub("sub", "Subcommand");
    sub.withFlag("--opt", "-o", "opt", "Opt", std::string("value"));
    root.addCommand(std::move(sub));
}

} // namespace

int main() {
    testBasicFlagMethods();
    testPersistentFlagMethods();
    testFlagGrouping();
    testCommandConfig();
    testCompletionMethods();
    testCommandWithSubcommands();
    testHelpConfiguration();
    testTraversalMethods();

    std::cout << "ok\n";
    return 0;
}
