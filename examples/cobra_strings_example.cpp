#include <iostream>
#include <string>
#include <vector>

#include "clasp/clasp.hpp"

// Locks the canonical Cobra-like error/usage strings byte-for-byte. The
// matching CTest assertions in CMakeLists.txt pin the exact wording, ordering,
// and whitespace of each message so regressions are catchable.
int main(int argc, char** argv) {
    clasp::Command rootCmd("app", "Canonical Cobra-like strings example");

    clasp::Command printCmd("print", "Prints a message");
    printCmd.withFlag("--message", "-m", "message", "Message to print", std::string("Hello, World!"));
    printCmd.withFlag("--count", "-c", "count", "Repeat count", 1);
    printCmd.action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        std::cout << parser.getFlag<std::string>("--message", "Hello, World!") << "\n";
        return 0;
    });

    clasp::Command lockedCmd("locked", "Requires --name");
    lockedCmd.withFlag("--name", "-n", "name", "Required name", std::string(""));
    lockedCmd.markFlagRequired("--name");
    lockedCmd.action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        std::cout << "name=" << parser.getFlag<std::string>("--name", "") << "\n";
        return 0;
    });

    rootCmd.addCommand(std::move(printCmd));
    rootCmd.addCommand(std::move(lockedCmd));
    return rootCmd.run(argc, argv);
}
