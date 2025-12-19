#include <iostream>
#include <string>
#include <vector>

#include "clasp/clasp.hpp"

int main(int argc, char** argv) {
    clasp::Command rootCmd("app", "Hooks example");

    rootCmd.persistentPreRun([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
        std::cout << "root-pre ";
    });
    rootCmd.persistentPostRun([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
        std::cout << "root-post";
    });

    clasp::Command parentCmd("parent", "Parent command");
    parentCmd.persistentPreRun([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
        std::cout << "parent-pre ";
    });
    parentCmd.persistentPostRun([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
        std::cout << " parent-post ";
    });

    clasp::Command childCmd("child", "Child command");
    childCmd.preRun([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { std::cout << "child-pre "; });
    childCmd.postRun([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { std::cout << "child-post "; });
    childCmd.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
        std::cout << "run ";
        return 0;
    });

    parentCmd.addCommand(std::move(childCmd));
    rootCmd.addCommand(std::move(parentCmd));
    return rootCmd.run(argc, argv);
}

