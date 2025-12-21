#include <iostream>
#include <string>
#include <vector>

#include "clasp/clasp.hpp"

int main(int argc, char** argv) {
    clasp::Command rootCmd("app", "Context propagation example");
    rootCmd.suggestions(false);

    // This mimics ExecuteContext-style usage: set a context on the root and access it from subcommands.
    rootCmd.setContext(std::string("rootctx"));

    clasp::Command show("show", "Print context");
    show.action([](clasp::Command& cmd, const clasp::Parser&, const std::vector<std::string>&) {
        const auto* ctx = cmd.contextAs<std::string>();
        std::cout << "ctx=" << (ctx ? *ctx : std::string("<none>")) << "\n";
        return 0;
    });

    rootCmd.addCommand(std::move(show));
    return rootCmd.run(argc, argv);
}
