#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "clasp/clasp.hpp"

static std::string normalizeUnderscoreToDash(std::string key) {
    if (key.rfind("--", 0) != 0) return key;
    std::replace(key.begin(), key.end(), '_', '-');
    return key;
}

int main(int argc, char** argv) {
    clasp::Command rootCmd("app", "Normalize example");
    rootCmd.normalizeFlagKeys(normalizeUnderscoreToDash);

    rootCmd.withPersistentFlag("--message-text", "", "message", "Message", std::string("default"));
    rootCmd.withPersistentFlag("--do-thing", "", "Do thing");

    rootCmd.action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        std::cout << "message=" << parser.getFlag<std::string>("--message-text", "") << "\n";
        std::cout << "doThingSeen=" << (parser.hasFlag("--do-thing") ? "true" : "false") << "\n";
        return 0;
    });

    return rootCmd.run(argc, argv);
}

