#include <iostream>
#include <string>
#include <vector>

#include "clasp/clasp.hpp"

int main(int argc, char** argv) {
    clasp::Command root("parser", "Parser coverage");
    root.suggestions(false);

    root.withFlag("--name", "-n", "name", "Name", std::string("default_val"));
    root.withCountFlag("--verbose", "-v", "verbose", "Verbose");
    root.withFlag("--list", "-l", "list", "List", std::string(""));
    root.withFlag("--map", "-m", "map", "Map", std::string(""));
    root.withFlag("--mode", "", "mode", "Mode", std::string(""));
    root.withFlag("--count", "-c", "count", "Count", 0);

    root.action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>& args) {
        std::cout << "occ=" << parser.occurrences("--verbose") << "\n";

        std::cout << "explicit=" << (parser.hasExplicitValue("--name") ? "yes" : "no");
        std::cout << " default=" << (parser.hasExplicitValue("--count") ? "yes" : "no") << "\n";

        auto explicitVals = parser.getExplicitFlagValues("--name");
        if (!explicitVals.empty()) std::cout << "explicit_val=" << explicitVals.back() << "\n";

        auto split = parser.getFlagValuesSplit("--list", ',');
        std::cout << "split=" << split.size() << "\n";

        auto pos = parser.positionals();
        std::cout << "pos=";
        for (size_t i = 0; i < pos.size(); ++i) {
            if (i > 0) std::cout << ",";
            std::cout << pos[i];
        }
        std::cout << "\n";

        auto flagMap = parser.getFlagMap("--map");
        for (const auto& [k, v] : flagMap) {
            std::cout << "map_" << k << "=" << v << "\n";
        }

        auto modeVal = parser.getStringTo<std::string>("--mode");
        if (!modeVal.empty()) {
            for (const auto& [k, v] : modeVal) {
                std::cout << "mode_" << k << "=" << v << "\n";
            }
        }

        std::cout << "has_name=" << (parser.hasValue("--name") ? "yes" : "no");
        std::cout << " has_missing=" << (parser.hasValue("--nonexistent") ? "yes" : "no") << "\n";

        auto nameVals = parser.getFlagValues("--name");
        if (!nameVals.empty()) std::cout << "name_vals=" << nameVals.size() << "\n";

        auto nameSlice = parser.getSlice<std::string>("--name");
        if (!nameSlice.empty()) std::cout << "slice=" << nameSlice.size() << "\n";

        return 0;
    });

    return root.run(argc, argv);
}
