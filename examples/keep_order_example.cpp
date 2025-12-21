#include <string>
#include <string_view>
#include <vector>

#include "clasp/clasp.hpp"

int main(int argc, char** argv) {
    clasp::Command root("app", "KeepOrder directive example");
    root.enableCompletion();

    clasp::Command fruit("fruit", "Fruit cmd");
    fruit.completionDirective(static_cast<std::uint32_t>(clasp::Command::ShellCompDirective::NoFileComp) |
                              static_cast<std::uint32_t>(clasp::Command::ShellCompDirective::KeepOrder));
    fruit.validArgsFunction([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&, std::string_view) {
        // Intentionally not sorted.
        return std::vector<std::string>{"zebra", "apple"};
    });

    root.addCommand(std::move(fruit));
    return root.run(argc, argv);
}

