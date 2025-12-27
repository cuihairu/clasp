#include <string>

#include "clasp/clasp.hpp"

int main(int argc, char** argv) {
    clasp::Command rootCmd("app", "Group example");
    rootCmd.suggestions(false);

    rootCmd.addGroup("core", "Core Commands");
    rootCmd.addGroup("extra", "Extra Commands");

    clasp::Command alpha("alpha", "Alpha cmd");
    alpha.groupId("core");
    clasp::Command beta("beta", "Beta cmd");
    beta.groupId("extra");
    clasp::Command zulu("zulu", "Zulu cmd");
    clasp::Command ghost("ghost", "Ghost cmd");
    ghost.groupId("ghost"); // Unknown group -> should be treated as ungrouped in help.

    rootCmd.addCommand(std::move(beta));
    rootCmd.addCommand(std::move(zulu));
    rootCmd.addCommand(std::move(alpha));
    rootCmd.addCommand(std::move(ghost));

    return rootCmd.run(argc, argv);
}
