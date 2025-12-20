#include <string>
#include <vector>

#include "clasp/clasp.hpp"

int main(int argc, char** argv) {
    clasp::Command rootCmd("app", "Completion example");
    rootCmd.version("0.1.0");
    rootCmd.withPersistentFlag("--verbose", "-v", "Enable verbose output");

    clasp::Command printCmd("print", "Prints a message");
    printCmd.withFlag("--message", "-m", "message", "Message to print", std::string("hi")).addAlias("p");

    rootCmd.addCommand(std::move(printCmd));
    rootCmd.enableCompletion();
    return rootCmd.run(argc, argv);
}

