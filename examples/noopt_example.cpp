#include <iostream>
#include <string>
#include <vector>

#include "clasp/clasp.hpp"

int main(int argc, char** argv) {
    clasp::Command rootCmd("app", "NoOptDefVal example");

    rootCmd.withPersistentFlag("--mode", "", "mode", "Mode", std::string("unset"));
    rootCmd.withPersistentFlag("--other", "", "Other flag");
    rootCmd.markPersistentFlagNoOptDefaultValue("--mode", "auto");

    clasp::Command showCmd("show", "Show values");
    showCmd.action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        std::cout << "mode=" << parser.getFlag<std::string>("--mode", "unset") << "\n";
        return 0;
    });

    rootCmd.action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        std::cout << "mode=" << parser.getFlag<std::string>("--mode", "unset") << "\n";
        std::cout << "other=" << (parser.hasFlag("--other") ? "true" : "false") << "\n";
        return 0;
    });

    rootCmd.addCommand(std::move(showCmd));
    return rootCmd.run(argc, argv);
}
