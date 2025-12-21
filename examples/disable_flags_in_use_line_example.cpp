#include <string>
#include <vector>

#include "clasp/clasp.hpp"

int main(int argc, char** argv) {
    clasp::Command rootCmd("app", "Disable flags-in-usage example");
    rootCmd.suggestions(false);
    rootCmd.disableFlagsInUseLine(true);

    rootCmd.withPersistentFlag("--alpha", "", "Alpha flag");

    clasp::Command subCmd("sub", "Sub command");
    subCmd.disableFlagsInUseLine(false);
    subCmd.withFlag("--beta", "", "Beta flag");
    rootCmd.addCommand(std::move(subCmd));

    return rootCmd.run(argc, argv);
}
