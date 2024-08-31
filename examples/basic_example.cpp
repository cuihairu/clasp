#include "clasp/clasp.hpp"

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " [filename]" << std::endl;
  }
    auto rootCmd = Clasp::Command("app", "A brief description of your application");

    auto printCmd = Clasp::Command("print", "Prints a message to the console")
        .withFlag<std::string>("--message", "-m", "Message to print", "Hello, World!")
        .action([](auto& args) {
            std::string message = args.getFlag<std::string>("--message");
            std::cout << message << std::endl;
        });

    rootCmd.addCommand(printCmd);
    rootCmd.run(argc, argv);

    return 0;
}
