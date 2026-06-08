#include <iostream>
#include <string>
#include <vector>
#include <sstream>

#include "clasp/clasp.hpp"

namespace {

// Test run() with help command
void testRunHelp() {
    clasp::Command root("app", "Test app");
    root.withFlag("--name", "-n", "name", "Name", std::string("default"));

    clasp::Command sub("sub", "Sub command");
    sub.withFlag("--verbose", "-v", "verbose", "Verbose", false);
    root.addCommand(std::move(sub));

    // Test help command
    const char* argv[] = {"app", "help"};
    int argc = 2;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with help subcommand
void testRunHelpSub() {
    clasp::Command root("app", "Test app");
    root.withFlag("--name", "-n", "name", "Name", std::string("default"));

    clasp::Command sub("sub", "Sub command");
    sub.withFlag("--verbose", "-v", "verbose", "Verbose", false);
    root.addCommand(std::move(sub));

    // Test help subcommand
    const char* argv[] = {"app", "help", "sub"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with version flag
void testRunVersion() {
    clasp::Command root("app", "Test app");
    root.version("1.0.0");

    // Test version flag
    const char* argv[] = {"app", "--version"};
    int argc = 2;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with version command
void testRunVersionCmd() {
    clasp::Command root("app", "Test app");
    root.version("1.0.0");

    // Test version command
    const char* argv[] = {"app", "version"};
    int argc = 2;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with unknown command
void testRunUnknownCmd() {
    clasp::Command root("app", "Test app");
    root.suggestions(false);

    // Test unknown command
    const char* argv[] = {"app", "unknown"};
    int argc = 2;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with unknown flag
void testRunUnknownFlag() {
    clasp::Command root("app", "Test app");
    root.withFlag("--name", "-n", "name", "Name", std::string("default"));

    // Test unknown flag
    const char* argv[] = {"app", "--unknown"};
    int argc = 2;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with missing required flag
void testRunMissingRequired() {
    clasp::Command root("app", "Test app");
    root.withFlag("--name", "-n", "name", "Name", std::string(""))
        .markFlagRequired("--name");

    // Test missing required flag
    const char* argv[] = {"app"};
    int argc = 1;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with action
void testRunAction() {
    clasp::Command root("app", "Test app");
    root.withFlag("--name", "-n", "name", "Name", std::string("default"))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            std::cout << "action:" << parser.getFlag<std::string>("--name") << "\n";
            return 0;
        });

    // Test action
    const char* argv[] = {"app", "--name", "test"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with subcommand action
void testRunSubAction() {
    clasp::Command root("app", "Test app");
    root.withFlag("--name", "-n", "name", "Name", std::string("default"));

    clasp::Command sub("sub", "Sub command");
    sub.withFlag("--verbose", "-v", "verbose", "Verbose", false)
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            std::cout << "sub action\n";
            return 0;
        });
    root.addCommand(std::move(sub));

    // Test subcommand action
    const char* argv[] = {"app", "sub"};
    int argc = 2;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with hooks
void testRunHooks() {
    clasp::Command root("app", "Test app");
    root.withFlag("--name", "-n", "name", "Name", std::string("default"))
        .persistentPreRun([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
            std::cout << "ppre\n";
        })
        .preRun([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
            std::cout << "pre\n";
        })
        .action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
            std::cout << "action\n";
            return 0;
        })
        .postRun([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
            std::cout << "post\n";
        })
        .persistentPostRun([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
            std::cout << "ppost\n";
        });

    // Test hooks
    const char* argv[] = {"app"};
    int argc = 1;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with hook error
void testRunHookError() {
    clasp::Command root("app", "Test app");
    root.withFlag("--name", "-n", "name", "Name", std::string("default"))
        .persistentPreRunE([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) -> std::optional<std::string> {
            return "hook error";
        })
        .action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
            std::cout << "action\n";
            return 0;
        });

    // Test hook error
    const char* argv[] = {"app"};
    int argc = 1;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with custom help function
void testRunCustomHelp() {
    clasp::Command root("app", "Test app");
    root.setHelpFunc([](const clasp::Command& cmd, std::ostream& out) {
        out << "Custom help for " << cmd.name() << "\n";
    });

    // Test custom help
    const char* argv[] = {"app", "--help"};
    int argc = 2;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with custom usage function
void testRunCustomUsage() {
    clasp::Command root("app", "Test app");
    root.setUsageFunc([](const clasp::Command& cmd, std::ostream& out) {
        out << "Custom usage for " << cmd.name() << "\n";
    });

    // Test custom usage
    const char* argv[] = {"app", "--unknown"};
    int argc = 2;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with custom flag error function
void testRunCustomFlagError() {
    clasp::Command root("app", "Test app");
    root.setFlagErrorFunc([](const clasp::Command& cmd, const std::string& err) -> std::string {
        return "Custom error: " + err;
    });

    // Test custom flag error
    const char* argv[] = {"app", "--unknown"};
    int argc = 2;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with version template
void testRunVersionTemplate() {
    clasp::Command root("app", "Test app");
    root.version("1.0.0")
        .setVersionTemplate("{{.Name}} v{{.Version}}");

    // Test version template
    const char* argv[] = {"app", "--version"};
    int argc = 2;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with help template
void testRunHelpTemplate() {
    clasp::Command root("app", "Test app");
    root.setHelpTemplate("CUSTOM_HELP: {{.UsageLine}}");

    // Test help template
    const char* argv[] = {"app", "--help"};
    int argc = 2;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with usage template
void testRunUsageTemplate() {
    clasp::Command root("app", "Test app");
    root.setUsageTemplate("CUSTOM_USAGE: {{.UsageLine}}");

    // Test usage template
    const char* argv[] = {"app", "--unknown"};
    int argc = 2;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with examples
void testRunExamples() {
    clasp::Command root("app", "Test app");
    root.example("app --name test  # Run with name\n"
                 "app help         # Show help");

    // Test examples
    const char* argv[] = {"app", "--help"};
    int argc = 2;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with groups
void testRunGroups() {
    clasp::Command root("app", "Test app");
    root.addGroup("core", "Core Commands")
        .addGroup("extra", "Extra Commands");

    clasp::Command sub1("sub1", "Sub command 1");
    sub1.groupId("core");
    root.addCommand(std::move(sub1));

    clasp::Command sub2("sub2", "Sub command 2");
    sub2.groupId("extra");
    root.addCommand(std::move(sub2));

    // Test groups
    const char* argv[] = {"app", "--help"};
    int argc = 2;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with aliases
void testRunAliases() {
    clasp::Command root("app", "Test app");
    root.withFlag("--name", "-n", "name", "Name", std::string("default"))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            std::cout << "action:" << parser.getFlag<std::string>("--name") << "\n";
            return 0;
        });

    // Test aliases
    const char* argv[] = {"app", "-n", "test"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with end of flags
void testRunEndOfFlags() {
    clasp::Command root("app", "Test app");
    root.withFlag("--name", "-n", "name", "Name", std::string("default"))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>& args) {
            std::cout << "action:" << parser.getFlag<std::string>("--name") << "\n";
            for (const auto& arg : args) {
                std::cout << "arg:" << arg << "\n";
            }
            return 0;
        });

    // Test end of flags
    const char* argv[] = {"app", "--", "--name", "test"};
    int argc = 4;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with bool negation
void testRunBoolNegation() {
    clasp::Command root("app", "Test app");
    root.withFlag("--verbose", "-v", "verbose", "Verbose", false)
        .boolNegation(true)
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            std::cout << "verbose:" << (parser.getFlag<bool>("--verbose") ? "true" : "false") << "\n";
            return 0;
        });

    // Test bool negation
    const char* argv[] = {"app", "--no-verbose"};
    int argc = 2;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with short flag grouping
void testRunShortFlagGrouping() {
    clasp::Command root("app", "Test app");
    root.withFlag("--verbose", "-v", "verbose", "Verbose", false)
        .withFlag("--quiet", "-q", "quiet", "Quiet", false)
        .shortFlagGrouping(true)
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            std::cout << "verbose:" << (parser.getFlag<bool>("--verbose") ? "true" : "false") << "\n";
            std::cout << "quiet:" << (parser.getFlag<bool>("--quiet") ? "true" : "false") << "\n";
            return 0;
        });

    // Test short flag grouping
    const char* argv[] = {"app", "-vq"};
    int argc = 2;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with normalize flag keys
void testRunNormalizeFlagKeys() {
    clasp::Command root("app", "Test app");
    root.withFlag("--name", "-n", "name", "Name", std::string("default"))
        .normalizeFlagKeys([](std::string key) {
            // Convert underscores to hyphens
            for (auto& c : key) {
                if (c == '_') c = '-';
            }
            return key;
        })
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            std::cout << "action:" << parser.getFlag<std::string>("--name") << "\n";
            return 0;
        });

    // Test normalize flag keys
    const char* argv[] = {"app", "--name", "test"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with config file
void testRunConfigFile() {
    clasp::Command root("app", "Test app");
    root.withFlag("--name", "-n", "name", "Name", std::string("default"))
        .withFlag("--config", "-c", "config", "Config file", std::string(""))
        .configFileFlag("config")
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            std::cout << "action:" << parser.getFlag<std::string>("--name") << "\n";
            return 0;
        });

    // Test config file (will fail if file doesn't exist, but covers the code path)
    const char* argv[] = {"app", "--config", "nonexistent.env"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with env binding
void testRunEnvBinding() {
    clasp::Command root("app", "Test app");
    root.withFlag("--name", "-n", "name", "Name", std::string("default"))
        .bindEnv("--name", "APP_NAME")
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            std::cout << "action:" << parser.getFlag<std::string>("--name") << "\n";
            return 0;
        });

    // Test env binding
    const char* argv[] = {"app"};
    int argc = 1;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with suggestions
void testRunSuggestions() {
    clasp::Command root("app", "Test app");
    root.suggestions(true);

    clasp::Command help("help", "Show help");
    root.addCommand(std::move(help));

    // Test suggestions
    const char* argv[] = {"app", "hlep"};
    int argc = 2;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with no suggest
void testRunNoSuggest() {
    clasp::Command root("app", "Test app");
    root.suggestions(false);

    // Test no suggest
    const char* argv[] = {"app", "hlep"};
    int argc = 2;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with disable flag parsing
void testRunDisableFlagParsing() {
    clasp::Command root("app", "Test app");
    root.disableFlagParsing(true)
        .action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>& args) {
            std::cout << "args:";
            for (const auto& arg : args) {
                std::cout << arg << ",";
            }
            std::cout << "\n";
            return 0;
        });

    // Test disable flag parsing
    const char* argv[] = {"app", "--name", "test", "-v"};
    int argc = 4;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with allow unknown flags
void testRunAllowUnknownFlags() {
    clasp::Command root("app", "Test app");
    root.allowUnknownFlags(true)
        .withFlag("--name", "-n", "name", "Name", std::string("default"))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            std::cout << "action:" << parser.getFlag<std::string>("--name") << "\n";
            return 0;
        });

    // Test allow unknown flags
    const char* argv[] = {"app", "--name", "test", "--unknown", "value"};
    int argc = 5;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with traverse children
void testRunTraverseChildren() {
    clasp::Command root("app", "Test app");
    root.traverseChildren(true)
        .withPersistentFlag("--name", "-n", "name", "Name", std::string("default"));

    clasp::Command sub("sub", "Sub command");
    sub.action([](clasp::Command& cmd, const clasp::Parser& parser, const std::vector<std::string>&) {
        std::cout << "sub:" << parser.getFlag<std::string>("--name") << "\n";
        return 0;
    });
    root.addCommand(std::move(sub));

    // Test traverse children
    const char* argv[] = {"app", "--name", "test", "sub"};
    int argc = 4;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with silence usage
void testRunSilenceUsage() {
    clasp::Command root("app", "Test app");
    root.silenceUsage(true)
        .withFlag("--name", "-n", "name", "Name", std::string("default"));

    // Test silence usage
    const char* argv[] = {"app", "--unknown"};
    int argc = 2;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with silence errors
void testRunSilenceErrors() {
    clasp::Command root("app", "Test app");
    root.silenceErrors(true)
        .withFlag("--name", "-n", "name", "Name", std::string("default"));

    // Test silence errors
    const char* argv[] = {"app", "--unknown"};
    int argc = 2;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with deprecated command
void testRunDeprecatedCmd() {
    clasp::Command root("app", "Test app");
    root.withFlag("--name", "-n", "name", "Name", std::string("default"));

    clasp::Command old("old", "Old command");
    old.deprecated("Use 'new' instead")
        .action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
            std::cout << "old action\n";
            return 0;
        });
    root.addCommand(std::move(old));

    // Test deprecated command
    const char* argv[] = {"app", "old"};
    int argc = 2;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with deprecated flag
void testRunDeprecatedFlag() {
    clasp::Command root("app", "Test app");
    root.withFlag("--name", "-n", "name", "Name", std::string("default"))
        .withFlag("--old", "-o", "old", "Old flag", std::string(""))
        .markFlagDeprecated("--old", "Use --name instead")
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            std::cout << "action:" << parser.getFlag<std::string>("--name") << "\n";
            return 0;
        });

    // Test deprecated flag
    const char* argv[] = {"app", "--old", "test"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with flag groups
void testRunFlagGroups() {
    clasp::Command root("app", "Test app");
    root.withFlag("--a", "", "a", "Flag A", std::string(""))
        .withFlag("--b", "", "b", "Flag B", std::string(""))
        .markFlagsMutuallyExclusive({"--a", "--b"})
        .action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
            std::cout << "action\n";
            return 0;
        });

    // Test flag groups
    const char* argv[] = {"app", "--a", "test", "--b", "test"};
    int argc = 5;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with required together flags
void testRunRequiredTogether() {
    clasp::Command root("app", "Test app");
    root.withFlag("--user", "-u", "user", "User", std::string(""))
        .withFlag("--pass", "-p", "pass", "Pass", std::string(""))
        .markFlagsRequiredTogether({"--user", "--pass"})
        .action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
            std::cout << "action\n";
            return 0;
        });

    // Test required together
    const char* argv[] = {"app", "--user", "test"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with one required flags
void testRunOneRequired() {
    clasp::Command root("app", "Test app");
    root.withFlag("--a", "", "a", "Flag A", std::string(""))
        .withFlag("--b", "", "b", "Flag B", std::string(""))
        .markFlagsOneRequired({"--a", "--b"})
        .action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
            std::cout << "action\n";
            return 0;
        });

    // Test one required
    const char* argv[] = {"app"};
    int argc = 1;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with args validation
void testRunArgsValidation() {
    clasp::Command root("app", "Test app");
    root.args([](const std::vector<std::string>& args) -> std::optional<std::string> {
        if (args.size() != 2) return "accepts 2 arg(s)";
        return std::nullopt;
    })
        .action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
            std::cout << "action\n";
            return 0;
        });

    // Test args validation
    const char* argv[] = {"app", "arg1"};
    int argc = 2;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with args range
void testRunArgsRange() {
    clasp::Command root("app", "Test app");
    root.args([](const std::vector<std::string>& args) -> std::optional<std::string> {
        if (args.size() < 1 || args.size() > 3) return "accepts between 1 and 3 arg(s)";
        return std::nullopt;
    })
        .action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
            std::cout << "action\n";
            return 0;
        });

    // Test args range
    const char* argv[] = {"app", "arg1", "arg2", "arg3", "arg4"};
    int argc = 5;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with valid args
void testRunValidArgs() {
    clasp::Command root("app", "Test app");
    root.validArgs({"apple", "banana", "cherry"})
        .action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
            std::cout << "action\n";
            return 0;
        });

    // Test valid args
    const char* argv[] = {"apple"};
    int argc = 1;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with completion
void testRunCompletion() {
    clasp::Command root("app", "Test app");
    root.enableCompletion()
        .withFlag("--name", "-n", "name", "Name", std::string("default"));

    // Test completion
    const char* argv[] = {"app", "__completeNoDesc", "--"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with count flag
void testRunCountFlag() {
    clasp::Command root("app", "Test app");
    root.withCountFlag("--verbose", "-v", "verbose", "Verbose", 0)
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            std::cout << "verbose:" << parser.getCount("--verbose", 0) << "\n";
            return 0;
        });

    // Test count flag
    const char* argv[] = {"app", "-v", "-v", "-v"};
    int argc = 4;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with bytes flag
void testRunBytesFlag() {
    clasp::Command root("app", "Test app");
    root.withBytesFlag("--size", "-s", "size", "Size", 0)
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            std::cout << "size:" << parser.getFlag<std::uint64_t>("--size") << "\n";
            return 0;
        });

    // Test bytes flag
    const char* argv[] = {"app", "--size", "1KB"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with IP flag
void testRunIPFlag() {
    clasp::Command root("app", "Test app");
    root.withIPFlag("--host", "-H", "host", "Host", std::string("127.0.0.1"))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            std::cout << "host:" << parser.getFlag<std::string>("--host") << "\n";
            return 0;
        });

    // Test IP flag
    const char* argv[] = {"app", "--host", "192.168.1.1"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with URL flag
void testRunURLFlag() {
    clasp::Command root("app", "Test app");
    root.withURLFlag("--url", "-u", "url", "URL", std::string(""))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            std::cout << "url:" << parser.getFlag<std::string>("--url") << "\n";
            return 0;
        });

    // Test URL flag
    const char* argv[] = {"app", "--url", "http://example.com"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with duration flag
void testRunDurationFlag() {
    clasp::Command root("app", "Test app");
    root.withFlag("--timeout", "-t", "timeout", "Timeout", std::chrono::milliseconds(0))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            std::cout << "timeout:" << parser.getFlag<std::chrono::milliseconds>("--timeout").count() << "\n";
            return 0;
        });

    // Test duration flag
    const char* argv[] = {"app", "--timeout", "2s"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with float flag
void testRunFloatFlag() {
    clasp::Command root("app", "Test app");
    root.withFlag("--ratio", "-r", "ratio", "Ratio", 0.0f)
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            std::cout << "ratio:" << parser.getFlag<float>("--ratio") << "\n";
            return 0;
        });

    // Test float flag
    const char* argv[] = {"app", "--ratio", "1.5"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with string slice
void testRunStringSlice() {
    clasp::Command root("app", "Test app");
    root.withFlag("--tags", "-t", "tags", "Tags", std::string(""))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto tags = parser.getFlagValuesSplit("--tags");
            std::cout << "tags:";
            for (const auto& tag : tags) {
                std::cout << tag << ",";
            }
            std::cout << "\n";
            return 0;
        });

    // Test string slice
    const char* argv[] = {"app", "--tags", "a,b,c"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with map flag
void testRunMapFlag() {
    clasp::Command root("app", "Test app");
    root.withFlag("--labels", "-l", "labels", "Labels", std::string(""))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto labels = parser.getFlagMap("--labels");
            std::cout << "labels:";
            for (const auto& [k, v] : labels) {
                std::cout << k << "=" << v << ",";
            }
            std::cout << "\n";
            return 0;
        });

    // Test map flag
    const char* argv[] = {"app", "--labels", "a=1,b=2"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with repeat flag
void testRunRepeatFlag() {
    clasp::Command root("app", "Test app");
    root.withFlag("--tag", "-t", "tag", "Tag", std::string(""))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            std::cout << "tag:" << parser.getFlag<std::string>("--tag") << "\n";
            return 0;
        });

    // Test repeat flag
    const char* argv[] = {"app", "--tag", "a", "--tag", "b"};
    int argc = 5;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with no args
void testRunNoArgs() {
    clasp::Command root("app", "Test app");
    root.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
        std::cout << "action\n";
        return 0;
    });

    // Test no args
    const char* argv[] = {"app"};
    int argc = 1;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with positionals
void testRunPositionals() {
    clasp::Command root("app", "Test app");
    root.action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        auto pos = parser.positionals();
        std::cout << "pos:";
        for (const auto& p : pos) {
            std::cout << p << ",";
        }
        std::cout << "\n";
        return 0;
    });

    // Test positionals
    const char* argv[] = {"app", "arg1", "arg2"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with color forced to Always (covers styled paths)
void testRunColorHelp() {
    clasp::Command root("app", "Test app");
    root.enableColor(clasp::ColorMode::Always, clasp::ColorThemeName::Vscode, false)
        .version("1.0.0")
        .withFlag("--name", "-n", "name", "Name", std::string("default"));

    // Test help with color forced to Always
    const char* argv[] = {"app", "--help"};
    int argc = 2;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with color forced to Always and version
void testRunColorVersion() {
    clasp::Command root("app", "Test app");
    root.enableColor(clasp::ColorMode::Always, clasp::ColorThemeName::Vscode, false)
        .version("1.0.0");

    // Test version with color forced to Always
    const char* argv[] = {"app", "--version"};
    int argc = 2;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with color forced to Always and subcommand help
void testRunColorHelpSub() {
    clasp::Command root("app", "Test app");
    root.enableColor(clasp::ColorMode::Always, clasp::ColorThemeName::Vscode, false)
        .version("1.0.0");

    clasp::Command sub("sub", "Sub command");
    sub.withFlag("--verbose", "-v", "verbose", "Verbose", false);
    root.addCommand(std::move(sub));

    // Test help sub with color forced to Always
    const char* argv[] = {"app", "help", "sub"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

// Test run() with color flag from CLI
void testRunColorFlag() {
    clasp::Command root("app", "Test app");
    root.enableColor(clasp::ColorMode::Auto, clasp::ColorThemeName::Vscode, true)
        .version("1.0.0");

    // Test with --color=always flag
    const char* argv[] = {"app", "--color=always", "--help"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "ok\n";
        return 0;
    }

    std::string test = argv[1];

    if (test == "help") testRunHelp();
    else if (test == "help-sub") testRunHelpSub();
    else if (test == "version") testRunVersion();
    else if (test == "version-cmd") testRunVersionCmd();
    else if (test == "unknown-cmd") testRunUnknownCmd();
    else if (test == "unknown-flag") testRunUnknownFlag();
    else if (test == "missing-required") testRunMissingRequired();
    else if (test == "action") testRunAction();
    else if (test == "sub-action") testRunSubAction();
    else if (test == "hooks") testRunHooks();
    else if (test == "hook-error") testRunHookError();
    else if (test == "custom-help") testRunCustomHelp();
    else if (test == "custom-usage") testRunCustomUsage();
    else if (test == "custom-flag-error") testRunCustomFlagError();
    else if (test == "version-template") testRunVersionTemplate();
    else if (test == "help-template") testRunHelpTemplate();
    else if (test == "usage-template") testRunUsageTemplate();
    else if (test == "examples") testRunExamples();
    else if (test == "groups") testRunGroups();
    else if (test == "aliases") testRunAliases();
    else if (test == "end-of-flags") testRunEndOfFlags();
    else if (test == "bool-negation") testRunBoolNegation();
    else if (test == "short-flag-grouping") testRunShortFlagGrouping();
    else if (test == "normalize-flag-keys") testRunNormalizeFlagKeys();
    else if (test == "config-file") testRunConfigFile();
    else if (test == "env-binding") testRunEnvBinding();
    else if (test == "suggestions") testRunSuggestions();
    else if (test == "no-suggest") testRunNoSuggest();
    else if (test == "disable-flag-parsing") testRunDisableFlagParsing();
    else if (test == "allow-unknown-flags") testRunAllowUnknownFlags();
    else if (test == "traverse-children") testRunTraverseChildren();
    else if (test == "silence-usage") testRunSilenceUsage();
    else if (test == "silence-errors") testRunSilenceErrors();
    else if (test == "deprecated-cmd") testRunDeprecatedCmd();
    else if (test == "deprecated-flag") testRunDeprecatedFlag();
    else if (test == "flag-groups") testRunFlagGroups();
    else if (test == "required-together") testRunRequiredTogether();
    else if (test == "one-required") testRunOneRequired();
    else if (test == "args-validation") testRunArgsValidation();
    else if (test == "args-range") testRunArgsRange();
    else if (test == "valid-args") testRunValidArgs();
    else if (test == "completion") testRunCompletion();
    else if (test == "count-flag") testRunCountFlag();
    else if (test == "bytes-flag") testRunBytesFlag();
    else if (test == "ip-flag") testRunIPFlag();
    else if (test == "url-flag") testRunURLFlag();
    else if (test == "duration-flag") testRunDurationFlag();
    else if (test == "float-flag") testRunFloatFlag();
    else if (test == "string-slice") testRunStringSlice();
    else if (test == "map-flag") testRunMapFlag();
    else if (test == "repeat-flag") testRunRepeatFlag();
    else if (test == "no-args") testRunNoArgs();
    else if (test == "positionals") testRunPositionals();
    else if (test == "color-help") testRunColorHelp();
    else if (test == "color-version") testRunColorVersion();
    else if (test == "color-help-sub") testRunColorHelpSub();
    else if (test == "color-flag") testRunColorFlag();
    else {
        std::cout << "unknown test: " << test << "\n";
        return 1;
    }

    return 0;
}
