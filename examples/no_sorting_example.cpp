#include <string>

#include "clasp/clasp.hpp"

int main(int argc, char** argv) {
    clasp::Command rootCmd("app", "No-sorting example");
    rootCmd.suggestions(false);
    rootCmd.disableSortCommands(true);
    rootCmd.disableSortFlags(true);

    rootCmd.withPersistentFlag("--zeta", "", "Z flag");
    rootCmd.withPersistentFlag("--alpha", "", "A flag");

    clasp::Command bCmd("b", "B cmd");
    clasp::Command aCmd("a", "A cmd");
    rootCmd.addCommand(std::move(bCmd));
    rootCmd.addCommand(std::move(aCmd));

    return rootCmd.run(argc, argv);
}

