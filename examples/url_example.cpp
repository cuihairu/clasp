#include <iostream>
#include <string>
#include <vector>

#include "clasp/clasp.hpp"

int main(int argc, char** argv) {
    clasp::Command rootCmd("app", "URL flag type example");

    clasp::Command show("show", "Print parsed URL value");
    show.withURLFlag("--url", "", "url", "URL (validated/canonicalized best-effort)", "http://example.com");
    show.action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        std::cout << "url=" << parser.getFlag<std::string>("--url", "") << "\n";
        return 0;
    });

    rootCmd.addCommand(std::move(show));
    return rootCmd.run(argc, argv);
}

