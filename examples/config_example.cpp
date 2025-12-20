#include <iostream>
#include <string>
#include <vector>

#include "clasp/clasp.hpp"

int main(int argc, char** argv) {
    clasp::Command rootCmd("app", "Config example");

    clasp::Command doCmd("do", "Prints message with env/config precedence");
    doCmd.withFlag("--config", "-c", "config", "Config file path", std::string(""));
    doCmd.withFlag("--message", "-m", "message", "Message to print", std::string("default"));

    // Precedence: flag > env > config > default
    doCmd.configFileFlag("--config");
    doCmd.bindEnv("--message", "APP_MESSAGE");

    doCmd.action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        std::cout << parser.getFlag<std::string>("--message", "default") << "\n";
        return 0;
    });

    rootCmd.addCommand(std::move(doCmd));
    return rootCmd.run(argc, argv);
}

