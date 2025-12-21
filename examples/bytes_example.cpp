#include <cstdint>
#include <iostream>
#include <vector>

#include "clasp/clasp.hpp"

int main(int argc, char** argv) {
    clasp::Command rootCmd("app", "Bytes flag example");
    rootCmd.suggestions(false);

    rootCmd.withPersistentBytesFlag("--limit", "", "limit", "Byte size (e.g. 1KB, 1.5MiB, 0x10)", 0);
    rootCmd.bindEnv("--limit", "APP_LIMIT");

    rootCmd.action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        std::cout << "limit=" << parser.getFlag<std::uint64_t>("--limit", 0) << "\n";
        return 0;
    });

    return rootCmd.run(argc, argv);
}

