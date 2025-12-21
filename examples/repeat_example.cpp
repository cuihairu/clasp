#include <iostream>
#include <string>
#include <vector>

#include "clasp/clasp.hpp"

static std::string join(const std::vector<std::string>& v, const std::string& sep) {
    std::string out;
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (i) out += sep;
        out += v[i];
    }
    return out;
}

int main(int argc, char** argv) {
    clasp::Command rootCmd("app", "Repeat flags example");
    rootCmd.withPersistentFlag("--tag", "-t", "tag", "Tag values", std::string(""));

    rootCmd.action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        std::cout << "last=" << parser.getFlag<std::string>("--tag", "") << "\n";
        std::cout << "tags=" << join(parser.getFlagValuesSplit("--tag", ','), "|") << "\n";
        return 0;
    });

    return rootCmd.run(argc, argv);
}

