#include <string>

#include "clasp/clasp.hpp"

int main(int argc, char** argv) {
    clasp::Command rootCmd("app", "Custom template example");
    rootCmd.suggestions(false);

    rootCmd.setUsageTemplate("USAGE>>{{.UsageLine}}");
    rootCmd.setHelpTemplate("HELP>>{{.UsageLine}}{{.CommandsSection}}{{.FlagsSection}}{{.GlobalFlagsSection}}");

    rootCmd.withPersistentFlag("--alpha", "", "Alpha flag");

    clasp::Command subCmd("sub", "Sub cmd");
    rootCmd.addCommand(std::move(subCmd));

    return rootCmd.run(argc, argv);
}

