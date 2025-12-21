#include <iostream>
#include <string>
#include <vector>

#include "clasp/clasp.hpp"

int main(int argc, char** argv) {
    clasp::Command rootCmd("app", "A brief description of your application");
    rootCmd.version("0.1.0");
    rootCmd.examples("app print --message \"Hello\"\napp help print\napp --version");
    rootCmd.withPersistentFlag("--verbose", "-v", "Enable verbose output");
    rootCmd.withPersistentFlag("--quiet", "-q", "Disable output");

    clasp::Command printCmd("print", "Prints a message to the console");
    printCmd
        .withFlag("--message", "-m", "message", "Message to print", std::string("Hello, World!"))
        .addAlias("p")
        .action([](clasp::Command& /*cmd*/, const clasp::Parser& parser, const std::vector<std::string>& /*args*/) {
            const auto message = parser.getFlag<std::string>("--message", "Hello, World!");
            std::cout << message << "\n";
            return 0;
        });

    clasp::Command rawCmd("raw", "Prints raw args without flag parsing");
    rawCmd.disableFlagParsing().action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>& args) {
        for (std::size_t i = 0; i < args.size(); ++i) {
            if (i) std::cout << " ";
            std::cout << args[i];
        }
        std::cout << "\n";
        return 0;
    });

    rootCmd.addCommand(std::move(printCmd));
    rootCmd.addCommand(std::move(rawCmd));
    return rootCmd.run(argc, argv);
}
