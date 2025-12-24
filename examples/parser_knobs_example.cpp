#include <iostream>
#include <string>
#include <vector>

#include "clasp/clasp.hpp"

static void printArgs(const std::vector<std::string>& args) {
    std::cout << "args=[";
    for (std::size_t i = 0; i < args.size(); ++i) {
        if (i) std::cout << ",";
        std::cout << args[i];
    }
    std::cout << "]\n";
}

int main(int argc, char** argv) {
    clasp::Command rootCmd("app", "Parser knobs example");
    rootCmd.withPersistentFlag("--verbose", "-v", "Enable verbose output");
    rootCmd.withPersistentFlag("--quiet", "-q", "Disable output");

    clasp::Command unknownOk("unknown_ok", "Allows unknown flags");
    unknownOk.allowUnknownFlags(true).action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>& args) {
        std::cout << "verbose=" << (parser.hasFlag("--verbose") ? "true" : "false") << " ";
        printArgs(args);
        return 0;
    });

    clasp::Command noGroup("no_group", "Disables short-flag grouping");
    noGroup.shortFlagGrouping(false).action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
        std::cout << "ran\n";
        return 0;
    });

    clasp::Command noNeg("no_neg", "Disables bool negation");
    noNeg.boolNegation(false).action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
        std::cout << "ran\n";
        return 0;
    });

    rootCmd.addCommand(std::move(unknownOk));
    rootCmd.addCommand(std::move(noGroup));
    rootCmd.addCommand(std::move(noNeg));

    return rootCmd.run(argc, argv);
}
