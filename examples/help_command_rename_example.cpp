#include <string>
#include <vector>

#include "clasp/clasp.hpp"

int main(int argc, char** argv) {
    clasp::Command rootCmd("app", "Help command rename example");
    rootCmd.suggestions(false);
    rootCmd.enableHelp({/*addHelpCommand=*/true, /*helpCommandName=*/"assist"});

    clasp::Command printCmd("print", "Prints a message");
    printCmd.withFlag("--message", "-m", "message", "Message to print", std::string("hi"));
    rootCmd.addCommand(std::move(printCmd));

    return rootCmd.run(argc, argv);
}
