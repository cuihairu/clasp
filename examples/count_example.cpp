#include <iostream>
#include <string>
#include <vector>

#include "clasp/clasp.hpp"

int main(int argc, char** argv) {
    clasp::Command rootCmd("app", "Count flag example");
    rootCmd.suggestions(false);

    rootCmd.withCountFlag("--verbose", "-v", "verbose", "Verbosity level (repeatable)", 0);

    rootCmd.action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>& args) {
        std::cout << "count=" << parser.getCount("--verbose", 0) << " occ=" << parser.occurrences("--verbose") << " args=";
        for (std::size_t i = 0; i < args.size(); ++i) {
            if (i) std::cout << ",";
            std::cout << args[i];
        }
        std::cout << "\n";
        return 0;
    });

    return rootCmd.run(argc, argv);
}
