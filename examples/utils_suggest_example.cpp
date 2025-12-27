#include <iostream>
#include <string>
#include <vector>

#include "clasp/utils.hpp"

static void usage() {
    std::cerr << "Usage: utils_suggest_example <input> <candidate...>\n";
}

int main(int argc, char** argv) {
    if (argc < 3) {
        usage();
        return 2;
    }

    const std::string input = argv[1];
    std::vector<std::string> candidates;
    candidates.reserve(static_cast<std::size_t>(argc - 2));
    for (int i = 2; i < argc; ++i) candidates.emplace_back(argv[i]);

    const auto suggestions = clasp::utils::suggest(input, candidates, /*maxResults=*/3, /*maxDistance=*/2);
    for (const auto& s : suggestions) std::cout << s << "\n";
    return 0;
}

