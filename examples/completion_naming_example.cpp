#include <string>
#include <vector>

#include "clasp/clasp.hpp"

int main(int argc, char** argv) {
    clasp::Command rootCmd("app", "Completion naming example");

    clasp::Command paint("paint", "Paint cmd");
    paint.validArgs(std::vector<std::string>{"red", "green", "blue"});
    rootCmd.addCommand(std::move(paint));

    clasp::Command::CompletionConfig cfg;
    cfg.completionCommandName = "comp";
    cfg.completeCommandName = "__c";
    cfg.completeNoDescCommandName = "__cnd";
    rootCmd.enableCompletion(cfg);

    return rootCmd.run(argc, argv);
}

