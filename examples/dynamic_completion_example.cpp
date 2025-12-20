#include <iostream>
#include <string>
#include <vector>

#include "clasp/clasp.hpp"

int main(int argc, char** argv) {
    clasp::Command rootCmd("app", "Dynamic completion example");
    rootCmd.enableCompletion();

    clasp::Command fruitCmd("fruit", "Fruit command");
    fruitCmd.validArgs({"apple", "banana", "cherry"});
    fruitCmd.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>& args) {
        for (std::size_t i = 0; i < args.size(); ++i) {
            if (i) std::cout << " ";
            std::cout << args[i];
        }
        std::cout << "\n";
        return 0;
    });

    clasp::Command paintCmd("paint", "Paint command");
    paintCmd.withFlag("--color", "-c", "color", "Color to use", std::string(""));
    paintCmd.registerFlagCompletion("--color",
                                    [](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&, std::string_view) {
                                        return std::vector<std::string>{"red", "green", "blue"};
                                    });
    paintCmd.action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        std::cout << parser.getFlag<std::string>("--color", "") << "\n";
        return 0;
    });

    rootCmd.addCommand(std::move(fruitCmd));
    rootCmd.addCommand(std::move(paintCmd));
    return rootCmd.run(argc, argv);
}

