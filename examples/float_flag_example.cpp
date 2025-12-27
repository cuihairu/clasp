#include <iostream>
#include <string>
#include <vector>

#include "clasp/clasp.hpp"

int main(int argc, char** argv) {
    clasp::Command rootCmd("app", "Float flag example");

    rootCmd.withFlag("--ratio", "", "ratio", "Ratio (float32)", float(0.5f));
    rootCmd.action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        std::cout << "ratio=" << parser.getFlag<float>("--ratio", 0.0f) << "\n";
        return 0;
    });

    return rootCmd.run(argc, argv);
}

