#include <iostream>
#include <string>
#include <vector>

#include "clasp/clasp.hpp"

int main(int argc, char** argv) {
    clasp::Command rootCmd("app", "Required flag example");

    clasp::Command doCmd("do", "Runs the required-flag demo");
    doCmd.withFlag("--name", "-n", "name", "Name to print", std::string(""));
    doCmd.markFlagRequired("--name");
    doCmd.action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        std::cout << parser.getFlag<std::string>("--name", "") << "\n";
        return 0;
    });

    rootCmd.addCommand(std::move(doCmd));
    return rootCmd.run(argc, argv);
}

