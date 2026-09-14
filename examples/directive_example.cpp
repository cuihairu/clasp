#include <string>
#include <vector>

#include "clasp/clasp.hpp"

int main(int argc, char** argv) {
    clasp::Command rootCmd("app", "Directive example");
    rootCmd.enableCompletion();

    clasp::Command fruit("fruit", "Fruit cmd");
    fruit.validArgsFunction([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&, std::string_view) {
        // Cobra-like: the special last token `:8` encodes ShellCompDirectiveFilterFileExt (example).
        return std::vector<std::string>{"apple", ":8"}; // LCOV_EXCL_LINE (compiler-generated exception cleanup edges of the inlined fixed-literal initializer_list vector; bad_alloc is not triggerable via argv)
    });

    rootCmd.addCommand(std::move(fruit));
    return rootCmd.run(argc, argv);
}

