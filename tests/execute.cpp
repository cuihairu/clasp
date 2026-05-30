#include <iostream>
#include <string>
#include <vector>

#include "clasp/clasp.hpp"

int main(int argc, char** argv) {
    clasp::Command root("exec", "Execute coverage");
    root.version("1.0.0");
    root.suggestions(true);
    root.suggestionsMinimumDistance(2);

    // sub: basic action + hooks + deprecated flag + bool negation + count + repeat
    clasp::Command sub("sub", "Sub command");
    sub.withFlag("--name", "-n", "name", "Name", std::string(""));
    sub.withFlag("--debug", "-d", "debug", "Debug", false);
    sub.withFlag("--old", "", "old", "Old flag", std::string(""));
    sub.withCountFlag("--verbose", "-v", "verbose", "Verbose");
    sub.withFlag("--tag", "-t", "tag", "Tag", std::string(""));
    sub.withFlag("--count", "", "count", "Count", 0);
    sub.markFlagRequired("--name");
    sub.markFlagDeprecated("--old", "use --name instead");
    sub.aliases({"s"});
    sub.boolNegation(true);

    sub.persistentPreRun([](clasp::Command& c, const clasp::Parser&, const std::vector<std::string>&) {
        std::cout << "ppre:" << c.name();
    });
    sub.preRun([](clasp::Command& c, const clasp::Parser&, const std::vector<std::string>&) {
        std::cout << "pre:" << c.name();
    });
    sub.postRun([](clasp::Command& c, const clasp::Parser&, const std::vector<std::string>&) {
        std::cout << "post:" << c.name();
    });
    sub.persistentPostRun([](clasp::Command& c, const clasp::Parser&, const std::vector<std::string>&) {
        std::cout << "ppost:" << c.name();
    });

    sub.action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>& args) {
        std::cout << "action:" << parser.getFlag<std::string>("--name", "");
        std::cout << " debug=" << (parser.getFlag<bool>("--debug", false) ? "1" : "0");
        std::cout << " count=" << parser.getFlag<int>("--count", 0);
        std::cout << " verbose=" << parser.getCount("--verbose");
        auto tags = parser.getFlagValues("--tag");
        if (!tags.empty()) {
            std::cout << " tags=";
            for (size_t i = 0; i < tags.size(); ++i) {
                if (i > 0) std::cout << ",";
                std::cout << tags[i];
            }
        }
        if (!args.empty()) {
            std::cout << " pos:";
            for (size_t i = 0; i < args.size(); ++i) {
                if (i > 0) std::cout << ",";
                std::cout << args[i];
            }
        }
        std::cout << "\n";
        return 0;
    });

    // sub2: flag groups (one required + required together)
    clasp::Command sub2("sub2", "Sub2 with flag groups");
    sub2.withFlag("--a", "", "a", "A", std::string(""));
    sub2.withFlag("--b", "", "b", "B", std::string(""));
    sub2.withFlag("--c", "", "c", "C", std::string(""));
    sub2.withFlag("--user", "-u", "user", "User", std::string(""));
    sub2.withFlag("--pass", "-p", "pass", "Pass", std::string(""));
    sub2.markFlagsMutuallyExclusive({"--a", "--c"});
    sub2.markFlagsOneRequired({"--a", "--b"});
    sub2.markFlagsRequiredTogether({"--user", "--pass"});
    sub2.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
        std::cout << "ok\n";
        return 0;
    });

    // err: hook error path
    clasp::Command errCmd("err", "Error hook");
    errCmd.preRunE([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&)
                       -> std::optional<std::string> {
        return "hook failed";
    });
    errCmd.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
        std::cout << "should not reach\n";
        return 0;
    });

    // quiet: silence usage and errors
    clasp::Command quiet("quiet", "Quiet cmd");
    quiet.silenceUsage(true);
    quiet.silenceErrors(true);
    quiet.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
        std::cout << "ok\n";
        return 0;
    });

    // raw: disable flag parsing
    clasp::Command raw("raw", "Raw cmd");
    raw.disableFlagParsing(true);
    raw.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>& args) {
        std::cout << "raw:";
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0) std::cout << ",";
            std::cout << args[i];
        }
        std::cout << "\n";
        return 0;
    });

    // flex: allow unknown flags
    clasp::Command flex("flex", "Flex cmd");
    flex.withFlag("--name", "-n", "name", "Name", std::string(""));
    flex.allowUnknownFlags(true);
    flex.action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        std::cout << "ok name=" << parser.getFlag<std::string>("--name", "") << "\n";
        return 0;
    });

    // trav: traverseChildren with persistent flag
    clasp::Command trav("trav", "Traverse cmd");
    trav.traverseChildren(true);
    trav.withPersistentFlag("--name", "-n", "name", "Name", std::string(""));
    clasp::Command travSub("sub", "Trav sub");
    travSub.action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        std::cout << "trav name=" << parser.getFlag<std::string>("--name", "") << "\n";
        return 0;
    });
    trav.addCommand(std::move(travSub));

    root.addCommand(std::move(sub));
    root.addCommand(std::move(sub2));
    root.addCommand(std::move(errCmd));
    root.addCommand(std::move(quiet));
    root.addCommand(std::move(raw));
    root.addCommand(std::move(flex));
    root.addCommand(std::move(trav));

    return root.run(argc, argv);
}
