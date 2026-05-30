#include <iostream>
#include <string>
#include <vector>

#include "clasp/clasp.hpp"

namespace {

void testActionE() {
    // Test actionE (Action with error return)
    {
        clasp::Command root("app", "Test actionE success");
        root.withFlag("--name", "-n", "name", "Name", std::string("test"));

        bool actionCalled = false;
        root.actionE([&](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) -> std::optional<std::string> {
            actionCalled = true;
            return std::nullopt;  // Success
        });

        const char* argv[] = {"app", "--name", "value"};
        auto result = root.run(3, const_cast<char**>(argv));
        std::cout << "actionE success: " << (result == 0 && actionCalled ? "pass" : "fail") << std::endl;
    }

    // Test actionE with error
    {
        clasp::Command root("app2", "Test actionE error");
        root.withFlag("--name", "-n", "name", "Name", std::string(""));

        bool actionCalled = false;
        root.actionE([&](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) -> std::optional<std::string> {
            actionCalled = true;
            return "Error from actionE";
        });

        const char* argv[] = {"app2", "--name", "value"};
        auto result = root.run(3, const_cast<char**>(argv));
        std::cout << "actionE error: " << (result != 0 && actionCalled ? "pass" : "fail") << std::endl;
    }
}

void testHooks() {
    // Test preRun hook
    {
        clasp::Command root("app", "Test preRun");
        bool preRunCalled = false;
        bool actionCalled = false;

        root.preRun([&](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
            preRunCalled = true;
        });

        root.action([&](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app"};
        root.run(1, const_cast<char**>(argv));
        std::cout << "preRun hook: " << (preRunCalled && actionCalled ? "pass" : "fail") << std::endl;
    }

    // Test postRun hook
    {
        clasp::Command root("app2", "Test postRun");
        bool postRunCalled = false;
        bool actionCalled = false;

        root.postRun([&](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
            postRunCalled = true;
        });

        root.action([&](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app2"};
        root.run(1, const_cast<char**>(argv));
        std::cout << "postRun hook: " << (postRunCalled && actionCalled ? "pass" : "fail") << std::endl;
    }

    // Test persistentPreRun hook (inherited by subcommands)
    {
        clasp::Command root("app3", "Test persistentPreRun");
        bool persistentPreRunCalled = false;

        root.persistentPreRun([&](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
            persistentPreRunCalled = true;
        });

        clasp::Command sub("sub", "Sub command");
        sub.action([&](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
            return 0;
        });

        root.addCommand(std::move(sub));

        const char* argv[] = {"app3", "sub"};
        root.run(2, const_cast<char**>(argv));
        std::cout << "persistentPreRun hook: " << (persistentPreRunCalled ? "pass" : "fail") << std::endl;
    }
}

void testHookE() {
    // Test preRunE hook (with error return)
    {
        clasp::Command root("app", "Test preRunE");
        bool preRunECalled = false;

        root.preRunE([&](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) -> std::optional<std::string> {
            preRunECalled = true;
            return std::nullopt;  // Success
        });

        root.action([&](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
            return 0;
        });

        const char* argv[] = {"app"};
        root.run(1, const_cast<char**>(argv));
        std::cout << "preRunE success: " << (preRunECalled ? "pass" : "fail") << std::endl;
    }

    // Test postRunE hook
    {
        clasp::Command root("app2", "Test postRunE");
        bool postRunECalled = false;

        root.postRunE([&](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) -> std::optional<std::string> {
            postRunECalled = true;
            return std::nullopt;
        });

        root.action([&](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
            return 0;
        });

        const char* argv[] = {"app2"};
        root.run(1, const_cast<char**>(argv));
        std::cout << "postRunE: " << (postRunECalled ? "pass" : "fail") << std::endl;
    }
}

void testVersion() {
    // Test version flag
    {
        clasp::Command root("app", "Test version");
        root.version("1.0.0");

        const char* argv[] = {"app", "--version"};
        auto result = root.run(2, const_cast<char**>(argv));
        std::cout << "version flag: " << (result == 0 ? "pass" : "fail") << std::endl;
    }

    // Test custom version template
    {
        clasp::Command root("app2", "Test version template");
        root.version("2.0.0");
        root.setVersionTemplate("Version: {{.Version}} - {{.CommandPath}}");

        const char* argv[] = {"app2", "--version"};
        auto result = root.run(2, const_cast<char**>(argv));
        std::cout << "version template: " << (result == 0 ? "pass" : "fail") << std::endl;
    }
}

void testDeprecated() {
    // Test deprecated command
    {
        clasp::Command root("app", "Test deprecated");
        root.deprecated("Use newapp instead");

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app"};
        auto result = root.run(1, const_cast<char**>(argv));
        std::cout << "deprecated command: " << (result == 0 && actionCalled ? "pass" : "fail") << std::endl;
    }
}

void testHidden() {
    // Test hidden command
    {
        clasp::Command root("app", "Test hidden parent");
        root.withFlag("--flag", "-f", "flag", "Flag", false);

        clasp::Command sub("hidden_cmd", "Hidden command");
        sub.hidden(true);
        sub.action([&](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
            return 0;
        });

        root.addCommand(std::move(sub));

        const char* argv[] = {"app", "hidden_cmd"};
        auto result = root.run(2, const_cast<char**>(argv));
        std::cout << "hidden command: " << (result == 0 ? "pass" : "fail") << std::endl;
    }
}

void testValidArgs() {
    // Test validArgs (argument completion candidates)
    {
        clasp::Command root("app", "Test validArgs");
        root.validArgs({"arg1", "arg2", "arg3"});
        root.args(clasp::ExactArgs(1));

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser&, const std::vector<std::string>& args) {
            std::cout << "valid args match: " << (args[0] == "arg1" ? "pass" : "fail") << std::endl;
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app", "arg1"};
        root.run(2, const_cast<char**>(argv));
    }

    // Test validArgsFunction (dynamic completion)
    {
        clasp::Command root("app2", "Test validArgsFunction");
        root.validArgsFunction([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&, std::string_view) {
            return std::vector<std::string>{"dynamic1", "dynamic2"};
        });

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app2"};
        root.run(1, const_cast<char**>(argv));
        std::cout << "validArgsFunction: " << (actionCalled ? "pass" : "fail") << std::endl;
    }
}

void testFlagCompletion() {
    // Test registerFlagCompletion
    {
        clasp::Command root("app", "Test flag completion");
        root.withFlag("--log-level", "-l", "level", "Log Level", std::string("info"));
        root.registerFlagCompletion("--log-level", [](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&, std::string_view) {
            return std::vector<std::string>{"debug", "info", "warn", "error"};
        });

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app", "--log-level", "debug"};
        root.run(3, const_cast<char**>(argv));
        std::cout << "flag completion: " << (actionCalled ? "pass" : "fail") << std::endl;
    }
}

void testMarkFlagMethods() {
    // Test markFlagRequired
    {
        clasp::Command root("app", "Test markFlagRequired");
        root.withFlag("--required", "-r", "required", "Required", std::string(""));
        root.markFlagRequired("--required");

        // This should fail because required flag is missing
        const char* argv[] = {"app"};
        auto result = root.run(1, const_cast<char**>(argv));
        std::cout << "markFlagRequired missing: " << (result != 0 ? "pass" : "fail") << std::endl;
    }

    // Test with required flag present
    {
        clasp::Command root("app2", "Test markFlagRequired present");
        root.withFlag("--required", "-r", "required", "Required", std::string(""));
        root.markFlagRequired("--required");

        const char* argv[] = {"app2", "--required", "value"};
        auto result = root.run(3, const_cast<char**>(argv));
        std::cout << "markFlagRequired present: " << (result == 0 ? "pass" : "fail") << std::endl;
    }

    // Test markFlagHidden
    {
        clasp::Command root("app3", "Test markFlagHidden");
        root.withFlag("--secret", "-s", "secret", "Secret", std::string(""));
        root.markFlagHidden("--secret");

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto secret = parser.getFlag<std::string>("--secret", "");
            std::cout << "markFlagHidden: " << (secret == "value" ? "pass" : "fail") << std::endl;
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app3", "--secret", "value"};
        root.run(3, const_cast<char**>(argv));
    }
}

void testMarkFlagAnnotation() {
    // Test markFlagAnnotation
    {
        clasp::Command root("app", "Test markFlagAnnotation");
        root.withFlag("--custom", "-c", "custom", "Custom", std::string(""));
        root.markFlagAnnotation("--custom", "key", "value");

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app", "--custom", "test"};
        root.run(3, const_cast<char**>(argv));
        std::cout << "markFlagAnnotation: " << (actionCalled ? "pass" : "fail") << std::endl;
    }
}

void testMarkFlagFilename() {
    // Test markFlagFilename
    {
        clasp::Command root("app", "Test markFlagFilename");
        root.withFlag("--config", "-c", "config", "Config", std::string(""));
        root.markFlagFilename("--config", {"yaml", "yml"});

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app", "--config", "test.yaml"};
        root.run(3, const_cast<char**>(argv));
        std::cout << "markFlagFilename: " << (actionCalled ? "pass" : "fail") << std::endl;
    }

    // Test markFlagDirname
    {
        clasp::Command root("app2", "Test markFlagDirname");
        root.withFlag("--dir", "-d", "dir", "Dir", std::string(""));
        root.markFlagDirname("--dir");

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app2", "--dir", "/path"};
        root.run(3, const_cast<char**>(argv));
        std::cout << "markFlagDirname: " << (actionCalled ? "pass" : "fail") << std::endl;
    }
}

void testMarkFlagNoOptDefaultValue() {
    // Test markFlagNoOptDefaultValue
    {
        clasp::Command root("app", "Test noOptDefault");
        root.withFlag("--optional", "-o", "optional", "Optional", std::string("default"));
        root.markFlagNoOptDefaultValue("--optional", "noopt-value");

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto val = parser.getFlag<std::string>("--optional", "");
            std::cout << "noOptDefault present: " << (val == "noopt-value" ? "pass" : "fail") << std::endl;
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app", "--optional"};
        root.run(2, const_cast<char**>(argv));
    }
}

void testFlagGroupConstraints() {
    // Test markFlagsMutuallyExclusive (pass case - none set)
    {
        clasp::Command root("app", "Test mutually exclusive none");
        root.withFlag("--opt1", "-a", "opt1", "Option 1", false);
        root.withFlag("--opt2", "-b", "opt2", "Option 2", false);
        root.markFlagsMutuallyExclusive({"--opt1", "--opt2"});

        const char* argv[] = {"app"};
        auto result = root.run(1, const_cast<char**>(argv));
        std::cout << "mutually exclusive none: " << (result == 0 ? "pass" : "fail") << std::endl;
    }

    // Test markFlagsMutuallyExclusive (pass case - one set)
    {
        clasp::Command root("app2", "Test mutually exclusive one");
        root.withFlag("--opt1", "-a", "opt1", "Option 1", false);
        root.withFlag("--opt2", "-b", "opt2", "Option 2", false);
        root.markFlagsMutuallyExclusive({"--opt1", "--opt2"});

        const char* argv[] = {"app2", "--opt1"};
        auto result = root.run(2, const_cast<char**>(argv));
        std::cout << "mutually exclusive one: " << (result == 0 ? "pass" : "fail") << std::endl;
    }

    // Test markFlagsOneRequired (pass case - one set)
    {
        clasp::Command root("app3", "Test one required");
        root.withFlag("--opt1", "-a", "opt1", "Option 1", false);
        root.withFlag("--opt2", "-b", "opt2", "Option 2", false);
        root.markFlagsOneRequired({"--opt1", "--opt2"});

        const char* argv[] = {"app3", "--opt1"};
        auto result = root.run(2, const_cast<char**>(argv));
        std::cout << "one required: " << (result == 0 ? "pass" : "fail") << std::endl;
    }

    // Test markFlagsRequiredTogether (pass case - none set)
    {
        clasp::Command root("app4", "Test required together none");
        root.withFlag("--opt1", "-a", "opt1", "Option 1", false);
        root.withFlag("--opt2", "-b", "opt2", "Option 2", false);
        root.markFlagsRequiredTogether({"--opt1", "--opt2"});

        const char* argv[] = {"app4"};
        auto result = root.run(1, const_cast<char**>(argv));
        std::cout << "required together none: " << (result == 0 ? "pass" : "fail") << std::endl;
    }

    // Test markFlagsRequiredTogether (pass case - all set)
    {
        clasp::Command root("app5", "Test required together all");
        root.withFlag("--opt1", "-a", "opt1", "Option 1", false);
        root.withFlag("--opt2", "-b", "opt2", "Option 2", false);
        root.markFlagsRequiredTogether({"--opt1", "--opt2"});

        const char* argv[] = {"app5", "--opt1", "--opt2"};
        auto result = root.run(3, const_cast<char**>(argv));
        std::cout << "required together all: " << (result == 0 ? "pass" : "fail") << std::endl;
    }
}

void testExample() {
    // Test example method
    {
        clasp::Command root("app", "Test example");
        root.example("Example line 1\nExample line 2");

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app"};
        root.run(1, const_cast<char**>(argv));
        std::cout << "example: " << (actionCalled ? "pass" : "fail") << std::endl;
    }

    // Test examples alias
    {
        clasp::Command root("app2", "Test examples");
        root.examples("Examples:\n  app2 --help");

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app2"};
        root.run(1, const_cast<char**>(argv));
        std::cout << "examples: " << (actionCalled ? "pass" : "fail") << std::endl;
    }
}

void testSuggestions() {
    // Test suggestionsMinimumDistance
    {
        clasp::Command root("app", "Test suggestions distance");
        root.withFlag("--verbose", "-v", "verbose", "Verbose", false);
        root.suggestionsMinimumDistance(1);

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
            actionCalled = true;
            return 0;
        });

        // Typo: --verbos instead of --verbose
        const char* argv[] = {"app", "--verbos"};
        auto result = root.run(2, const_cast<char**>(argv));
        std::cout << "suggestions distance: " << (result != 0 ? "pass" : "fail") << std::endl;
    }

    // Test suggestions disable
    {
        clasp::Command root("app2", "Test suggestions disable");
        root.withFlag("--verbose", "-v", "verbose", "Verbose", false);
        root.suggestions(false);

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app2", "--verbos"};
        auto result = root.run(2, const_cast<char**>(argv));
        std::cout << "suggestions disable: " << (result != 0 ? "pass" : "fail") << std::endl;
    }
}

void testAddGroup() {
    // Test addGroup
    {
        clasp::Command root("app", "Test addGroup");
        root.addGroup("group1", "Group 1");
        root.addGroup("group2", "Group 2");

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app"};
        root.run(1, const_cast<char**>(argv));
        std::cout << "addGroup: " << (actionCalled ? "pass" : "fail") << std::endl;
    }
}

void testGroupId() {
    // Test groupId
    {
        clasp::Command root("app", "Test groupId");
        root.groupId("my-group");

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app"};
        root.run(1, const_cast<char**>(argv));
        std::cout << "groupId: " << (actionCalled ? "pass" : "fail") << std::endl;
    }
}

} // namespace

int main() {
    std::cout << "=== Testing actionE ===" << std::endl;
    testActionE();

    std::cout << "\n=== Testing Hooks ===" << std::endl;
    testHooks();

    std::cout << "\n=== Testing HookE ===" << std::endl;
    testHookE();

    std::cout << "\n=== Testing Version ===" << std::endl;
    testVersion();

    std::cout << "\n=== Testing Deprecated ===" << std::endl;
    testDeprecated();

    std::cout << "\n=== Testing Hidden ===" << std::endl;
    testHidden();

    std::cout << "\n=== Testing ValidArgs ===" << std::endl;
    testValidArgs();

    std::cout << "\n=== Testing FlagCompletion ===" << std::endl;
    testFlagCompletion();

    std::cout << "\n=== Testing MarkFlagMethods ===" << std::endl;
    testMarkFlagMethods();

    std::cout << "\n=== Testing MarkFlagAnnotation ===" << std::endl;
    testMarkFlagAnnotation();

    std::cout << "\n=== Testing MarkFlagFilename ===" << std::endl;
    testMarkFlagFilename();

    std::cout << "\n=== Testing MarkFlagNoOptDefaultValue ===" << std::endl;
    testMarkFlagNoOptDefaultValue();

    std::cout << "\n=== Testing FlagGroupConstraints ===" << std::endl;
    testFlagGroupConstraints();

    std::cout << "\n=== Testing Example ===" << std::endl;
    testExample();

    std::cout << "\n=== Testing Suggestions ===" << std::endl;
    testSuggestions();

    std::cout << "\n=== Testing AddGroup ===" << std::endl;
    testAddGroup();

    std::cout << "\n=== Testing GroupId ===" << std::endl;
    testGroupId();

    std::cout << "\nok\n";
    return 0;
}
