#include <string>
#include <vector>

#include "clasp/clasp.hpp"

int main(int argc, char** argv) {
    clasp::Command rootCmd("app", "Completion naming example");

    clasp::Command paint("paint", "Paint cmd");
    paint.validArgs(std::vector<std::string>{"red", "green", "blue"}); // LCOV_EXCL_LINE (compiler-generated exception cleanup edges of the inlined fixed-literal initializer_list vector; bad_alloc is not triggerable via argv)
    rootCmd.addCommand(std::move(paint));

    clasp::Command::CompletionConfig cfg;
    cfg.completionCommandName = "comp";
    cfg.completeCommandName = "__c";
    cfg.completeNoDescCommandName = "__cnd";
    rootCmd.enableCompletion(cfg);

    return rootCmd.run(argc, argv);
}

