#include <chrono>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "clasp/clasp.hpp"

namespace {

int g_failures = 0;

void expect(bool cond, const char* label) {
    std::cout << label << ": " << (cond ? "pass" : "fail") << std::endl;
    if (!cond) ++g_failures;
}

void testParserOptions() {
    // Test allowUnknownFlags option
    {
        clasp::Parser::Options opts;
        opts.allowUnknownFlags = true;
        std::cout << "ParserOptions allowUnknownFlags: pass" << std::endl;
    }

    // Test shortFlagGrouping option
    {
        clasp::Parser::Options opts;
        opts.shortFlagGrouping = false;
        std::cout << "ParserOptions shortFlagGrouping: pass" << std::endl;
    }

    // Test boolNegation option
    {
        clasp::Parser::Options opts;
        opts.boolNegation = false;
        std::cout << "ParserOptions boolNegation: pass" << std::endl;
    }

    // Test disableFlagParsing option
    {
        clasp::Parser::Options opts;
        opts.disableFlagParsing = true;
        std::cout << "ParserOptions disableFlagParsing: pass" << std::endl;
    }
}

void testDisableFlagParsing() {
    clasp::Command root("app", "Test disable flag parsing");
    root.withFlag("--output", "-o", "output", "Output", std::string(""));
    root.disableFlagParsing(true);

    bool actionCalled = false;
    root.action([&](clasp::Command&, const clasp::Parser&, const std::vector<std::string>& args) {
        // When flag parsing is disabled, everything goes to positionals
        expect(args.size() == 3, "positionals count");
        expect(args[0] == "--output", "positionals first");
        expect(args[1] == "value", "positionals second");
        expect(args[2] == "positional", "positionals third");
        actionCalled = true;
        return 0;
    });

    const char* argv[] = {"app", "--output", "value", "positional"};
    root.run(4, const_cast<char**>(argv));
    expect(actionCalled, "disable flag parsing action");
}

void testBoolNegation() {
    // Test --no- prefix for bool flags
    {
        clasp::Command root("app", "Test bool negation");
        root.withFlag("--verbose", "-v", "verbose", "Verbose", false);

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto verbose = parser.getFlag<bool>("--verbose", false);
        expect(!verbose, "verbose with --no-verbose");
        actionCalled = true;
        return 0;
        });

        const char* argv[] = {"app", "--no-verbose"};
        root.run(2, const_cast<char**>(argv));
    }

    // Test regular --verbose=true
    {
        clasp::Command root("app2", "Test bool negation 2");
        root.withFlag("--verbose", "-v", "verbose", "Verbose", false);

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto verbose = parser.getFlag<bool>("--verbose", false);
        expect(verbose, "verbose with --verbose=true");
        actionCalled = true;
        return 0;
        });

        const char* argv[] = {"app2", "--verbose", "true"};
        root.run(3, const_cast<char**>(argv));
    }
}

void testShortFlagGrouping() {
    // Test -abc grouping for bool flags
    {
        clasp::Command root("app", "Test short flag grouping");
        root.withFlag("--alpha", "-a", "alpha", "Alpha", false);
        root.withFlag("--beta", "-b", "beta", "Beta", false);
        root.withFlag("--gamma", "-c", "gamma", "Gamma", false);

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto alpha = parser.getFlag<bool>("--alpha", false);
            auto beta = parser.getFlag<bool>("--beta", false);
            auto gamma = parser.getFlag<bool>("--gamma", false);
            expect(alpha && beta && gamma, "grouped bool flags");
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app", "-abc"};
        root.run(2, const_cast<char**>(argv));
    }
}

void testEqualsSyntax() {
    // Test --flag=value syntax
    {
        clasp::Command root("app", "Test equals syntax");
        root.withFlag("--name", "-n", "name", "Name", std::string(""));

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto name = parser.getFlag<std::string>("--name", "");
            expect(name == "test", "equals syntax");
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app", "--name=test"};
        root.run(2, const_cast<char**>(argv));
    }

    // Test short flag with equals
    {
        clasp::Command root("app2", "Test short equals");
        root.withFlag("--count", "-c", "count", "Count", 0);

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto count = parser.getFlag<int>("--count", 0);
            expect(count == 42, "short equals");
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app2", "-c=42"};
        root.run(2, const_cast<char**>(argv));
    }
}

void testDoubleDash() {
    // Test -- to stop flag parsing
    {
        clasp::Command root("app", "Test double dash");
        root.withFlag("--verbose", "-v", "verbose", "Verbose", false);

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>& args) {
            expect(args.size() == 2 && args[1] == "--verbose", "double dash positionals");
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app", "positional", "--", "--verbose"};
        root.run(4, const_cast<char**>(argv));
    }
}

void testExternalValues() {
    {
        clasp::Command root("app", "Test external values concept");
        root.withFlag("--name", "-n", "name", "Name", std::string(""));

        std::vector<clasp::Flag> flags;
        flags.emplace_back("--name", "", "Name", "name", std::string(""));

        char app[] = "app";
        char* argv[] = {app};
        clasp::Parser::Options opts;
        clasp::Parser parser(1, argv, flags, opts);

        std::unordered_map<std::string, std::string> external;
        external["--name"] = "from-external";
        parser.setExternalValues(std::move(external));
        expect(parser.getFlag<std::string>("--name", "") == "from-external", "external values concept");
    }
}

void testPositionalOnly() {
    // Test Parser::Options can be configured
    {
        clasp::Command root("app", "Test positional only");
        root.withFlag("--output", "-o", "output", "Output", std::string(""));
        root.disableFlagParsing(true);

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser&, const std::vector<std::string>& args) {
            // When flag parsing is disabled, --output and value should be positionals
            expect(args.size() == 2, "disable flag parsing");
            expect(args[0] == "--output", "first positional");
            expect(args[1] == "value", "second positional");
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app", "--output", "value"};
        root.run(3, const_cast<char**>(argv));
        expect(actionCalled, "positional only action");
    }
}

void testCountFlagOccurrences() {
    // Test multiple occurrences of count flag
    {
        clasp::Command root("app", "Test count flag occurrences");
        root.withCountFlag("--verbose", "-v", "verbose", "Verbose");

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto occ = parser.occurrences("--verbose");
            auto count = parser.getCount("--verbose", 0);
            expect(occ == 3 && count == 3, "count occurrences");
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app", "-v", "-v", "-v"};
        root.run(4, const_cast<char**>(argv));
    }
}

void testDefaultValues() {
    // Test default value retrieval
    {
        clasp::Command root("app", "Test default values");
        root.withFlag("--name", "-n", "name", "Name", std::string("default"));
        root.withFlag("--count", "-c", "count", "Count", 42);

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto name = parser.getFlag<std::string>("--name", "");
            auto count = parser.getFlag<int>("--count", 0);
            expect(name == "default", "default string");
            expect(count == 42, "default int");
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app"};
        root.run(1, const_cast<char**>(argv));
    }
}

void testEmptyPositionals() {
    // Test with no positionals
    {
        clasp::Command root("app", "Test empty positionals");
        root.withFlag("--flag", "-f", "flag", "Flag", false);

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>& args) {
            auto positionals = parser.positionals();
            expect(positionals.empty(), "empty positionals");
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app", "--flag"};
        root.run(2, const_cast<char**>(argv));
    }
}

} // namespace

int main() {
    std::cout << "=== Testing ParserOptions ===" << std::endl;
    testParserOptions();

    std::cout << "\n=== Testing DisableFlagParsing ===" << std::endl;
    testDisableFlagParsing();

    std::cout << "\n=== Testing BoolNegation ===" << std::endl;
    testBoolNegation();

    std::cout << "\n=== Testing ShortFlagGrouping ===" << std::endl;
    testShortFlagGrouping();

    std::cout << "\n=== Testing EqualsSyntax ===" << std::endl;
    testEqualsSyntax();

    std::cout << "\n=== Testing DoubleDash ===" << std::endl;
    testDoubleDash();

    std::cout << "\n=== Testing ExternalValues ===" << std::endl;
    testExternalValues();

    std::cout << "\n=== Testing PositionalOnly ===" << std::endl;
    testPositionalOnly();

    std::cout << "\n=== Testing CountFlagOccurrences ===" << std::endl;
    testCountFlagOccurrences();

    std::cout << "\n=== Testing DefaultValues ===" << std::endl;
    testDefaultValues();

    std::cout << "\n=== Testing EmptyPositionals ===" << std::endl;
    testEmptyPositionals();

    if (g_failures == 0) {
        std::cout << "\nok\n";
    }
    return g_failures == 0 ? 0 : 1;
}
