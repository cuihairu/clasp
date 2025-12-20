#include <iostream>
#include <string>
#include <vector>

#include "clasp/clasp.hpp"

int main(int argc, char** argv) {
    clasp::Command rootCmd("app", "TraverseChildren example");
    rootCmd.traverseChildren();

    clasp::Command printCmd("print", "Prints a message");
    printCmd.withFlag("--message", "-m", "message", "Message to print", std::string("Hello")).action(
        [](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            std::cout << parser.getFlag<std::string>("--message", "Hello") << "\n";
            return 0;
        });

    rootCmd.addCommand(std::move(printCmd));
    return rootCmd.run(argc, argv);
}

