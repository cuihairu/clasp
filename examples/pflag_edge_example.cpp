#include <iostream>
#include <string>
#include <vector>

#include "clasp/clasp.hpp"

// Pins the "weird" pflag parsing interactions: repeated bools, empty `=`
// values, negative-number values, intermixed positionals, NoOptDefVal in both
// forms, and end-of-flags handling.
int main(int argc, char** argv) {
    clasp::Command rootCmd("app", "pflag edge interactions example");

    clasp::Command edgesCmd("edges", "Print parsed flag states");
    edgesCmd.withFlag("--verbose", "-v", "Verbose flag");
    edgesCmd.withFlag("--name", "-n", "name", "Name value", std::string(""));
    edgesCmd.withFlag("--num", "", "num", "Number value", 0);
    edgesCmd.withFlag("--sep", "-s", "sep", "Separator value", std::string("none"));
    edgesCmd.withFlag("--mode", "", "mode", "Mode value", std::string("unset"));
    edgesCmd.markFlagNoOptDefaultValue("--mode", "auto");

    edgesCmd.action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>& args) {
        std::cout << "verbose=" << (parser.getFlag<bool>("--verbose", false) ? "true" : "false") << "\n";
        std::cout << "name=" << parser.getFlag<std::string>("--name", "") << "\n";
        std::cout << "num=" << parser.getFlag<int>("--num", 0) << "\n";
        std::cout << "sep=" << parser.getFlag<std::string>("--sep", "none") << "\n";
        std::cout << "mode=" << parser.getFlag<std::string>("--mode", "unset") << "\n";
        std::string pos;
        for (std::size_t i = 0; i < args.size(); ++i) {
            if (i) pos += ",";
            pos += args[i];
        }
        std::cout << "pos=[" << pos << "]\n";
        return 0;
    });

    rootCmd.addCommand(std::move(edgesCmd));
    return rootCmd.run(argc, argv);
}
