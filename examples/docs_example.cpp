#include <iostream>
#include <string>
#include <vector>

#include "clasp/clasp.hpp"

int main() {
    clasp::Command rootCmd("app", "Docs example");
    rootCmd.version("0.1.0");
    rootCmd.examples("app print --message \"hi\"\napp help print\napp --version");
    rootCmd.withPersistentFlag("--verbose", "-v", "Enable verbose output");

    clasp::Command printCmd("print", "Prints a message");
    printCmd.withFlag("--message", "-m", "message", "Message to print", std::string("hi"));
    rootCmd.addCommand(std::move(printCmd));

    std::cout << "MARKDOWN\n";
    rootCmd.printMarkdown(std::cout, /*recursive=*/true);
    std::cout << "\nMANPAGE\n";
    rootCmd.printManpage(std::cout);
    return 0;
}
