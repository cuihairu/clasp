#include <string>
#include <vector>

#include "clasp/clasp.hpp"

int main(int argc, char** argv) {
    clasp::Command rootCmd("app", "Version template example");
    rootCmd.version("1.2.3");
    rootCmd.setVersionTemplate("{{.CommandPath}}={{.Version}}");

    clasp::Command subCmd("sub", "Sub command");
    subCmd.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
    rootCmd.addCommand(std::move(subCmd));

    return rootCmd.run(argc, argv);
}
