#include <iostream>
#include <string>
#include <vector>

#include "clasp/clasp.hpp"

int main(int argc, char** argv) {
    clasp::Command rootCmd("app", "TraverseChildren + completion example");
    rootCmd.traverseChildren();
    rootCmd.withPersistentFlag("--verbose", "-v", "Enable verbose output");
    rootCmd.enableCompletion();

    clasp::Command printCmd("print", "Print a message");
    printCmd.withFlag("--message", "-m", "message", "Message", std::string("hi"));
    printCmd.action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        std::cout << parser.getFlag<std::string>("--message", "hi") << "\n";
        return 0;
    });

    rootCmd.addCommand(std::move(printCmd));
    return rootCmd.run(argc, argv);
}

