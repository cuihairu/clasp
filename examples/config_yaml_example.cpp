#include <iostream>
#include <string>
#include <vector>

#include "clasp/clasp.hpp"

int main(int argc, char** argv) {
    clasp::Command rootCmd("app", "YAML config example");
    rootCmd.suggestions(false);

    clasp::Command doCmd("do", "Runs the config demo");
    doCmd.withFlag("--config", "-c", "config", "Config file path", std::string(""));
    doCmd.withFlag("--message", "-m", "message", "Message", std::string("default"));
    doCmd.withFlag("--nested-mode", "", "nestedMode", "Nested mode", std::string("none"));
    doCmd.configFileFlag("--config");
    doCmd.action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        std::cout << "message=" << parser.getFlag<std::string>("--message", "") << " nested_mode="
                  << parser.getFlag<std::string>("--nested-mode", "") << "\n";
        return 0;
    });

    rootCmd.addCommand(std::move(doCmd));
    return rootCmd.run(argc, argv);
}
