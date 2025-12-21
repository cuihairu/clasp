#include <string>
#include <vector>

#include "clasp/clasp.hpp"

int main(int argc, char** argv) {
    clasp::Command rootCmd("app", "File/dir completion directives example");
    rootCmd.enableCompletion();

    clasp::Command openCmd("open", "Open a config");
    openCmd.withFlag("--config", "-c", "config", "Config file", std::string(""));
    openCmd.markFlagFilename("--config", {"yaml", "yml"});

    openCmd.withFlag("--out-dir", "", "outDir", "Output directory", std::string(""));
    openCmd.markFlagDirname("--out-dir");

    openCmd.action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        (void)parser;
        return 0;
    });

    rootCmd.addCommand(std::move(openCmd));
    return rootCmd.run(argc, argv);
}

