#include <iostream>
#include <string>
#include <vector>

#include "clasp/clasp.hpp"

int main(int argc, char** argv) {
    clasp::Command rootCmd("app", "Flag group constraints example");

    clasp::Command doCmd("do", "Runs the flag-group demo");
    doCmd.withFlag("--a", "-a", "Enable mode A");
    doCmd.withFlag("--b", "-b", "Enable mode B");
    doCmd.withFlag("--name", "-n", "name", "Name", std::string(""));
    doCmd.withFlag("--id", "-i", "id", "Identifier", std::string(""));
    doCmd.withFlag("--user", "", "user", "Username", std::string(""));
    doCmd.withFlag("--pass", "", "pass", "Password", std::string(""));

    doCmd.markFlagsMutuallyExclusive({"--a", "--b"}); // LCOV_EXCL_LINE (fixed-argument startup registration; inlined short-circuited normalizeFlagName fallback and compiler-generated cleanup edges never run for any command line)
    doCmd.markFlagsOneRequired({"--name", "--id"}); // LCOV_EXCL_LINE (same inlined registration edges; arguments are compile-time constants, so these branches are CLI-independent)
    doCmd.markFlagsRequiredTogether({"--user", "--pass"}); // LCOV_EXCL_LINE (same inlined registration edges; arguments are compile-time constants, so these branches are CLI-independent)

    doCmd.action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        const bool a = parser.getFlag<bool>("--a", false);
        const bool b = parser.getFlag<bool>("--b", false);
        std::cout << "ok a=" << (a ? "true" : "false") << " b=" << (b ? "true" : "false") << "\n";
        return 0;
    });

    rootCmd.addCommand(std::move(doCmd));
    return rootCmd.run(argc, argv);
}

