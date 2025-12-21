#include <iostream>
#include <string>
#include <vector>

#include "clasp/clasp.hpp"

int main() {
    clasp::Command rootCmd("app", "Execute example");

    clasp::Command printCmd("print", "Prints a message");
    printCmd.withFlag("--message", "-m", "message", "Message to print", std::string("hi"));
    printCmd.action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        std::cout << parser.getFlag<std::string>("--message", "hi") << "\n";
        return 0;
    });

    rootCmd.addCommand(std::move(printCmd));

    rootCmd.setArgs({"print", "--message", "from-execute"});
    return rootCmd.execute();
}

