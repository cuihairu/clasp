#include <string>
#include <vector>

#include "clasp/clasp.hpp"

int main(int argc, char** argv) {
    clasp::Command rootCmd("app", "Flag error function example");
    rootCmd.suggestions(false);

    rootCmd.setFlagErrorFunc([](const clasp::Command&, const std::string& err) {
        return std::string("FLAGERR: ") + err;
    });

    rootCmd.withFlag("--name", "-n", "name", "Name", std::string("bob"));
    return rootCmd.run(argc, argv);
}
