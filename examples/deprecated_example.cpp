#include <iostream>
#include <string>
#include <vector>

#include "clasp/clasp.hpp"

int main(int argc, char** argv) {
    clasp::Command rootCmd("app", "Deprecated example");
    rootCmd.version("0.1.0");

    clasp::Command oldCmd("old", "Old command");
    oldCmd.deprecated("use `app new` instead");
    oldCmd.withFlag("--old-flag", "", "An old flag");
    oldCmd.markFlagDeprecated("--old-flag", "use `--new-flag` instead");
    oldCmd.action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        std::cout << "ran old\n";
        std::cout << "old_flag=" << (parser.hasFlag("--old-flag") ? "true" : "false") << "\n";
        return 0;
    });

    rootCmd.addCommand(std::move(oldCmd));
    return rootCmd.run(argc, argv);
}

