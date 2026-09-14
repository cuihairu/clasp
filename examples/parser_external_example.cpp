#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "clasp/parser.hpp"

int main() {
    std::vector<clasp::Flag> flags;
    flags.emplace_back("--name", "", "name", "Name", std::string("def"));
    flags.emplace_back("--v", "", "v", "Verbosity", 0);

    char app[] = "app";
    char* argv[] = {app, nullptr};

    clasp::Parser::Options opts;
    clasp::Parser parser(/*argc=*/1, argv, flags, opts);
    if (!parser.ok()) { // LCOV_EXCL_LINE (the Parser is built with argc=1 and only valid flag definitions, so the argument-parse loop never runs and ok() is always true)
        std::cerr << parser.error() << "\n"; // LCOV_EXCL_LINE (unreachable: ok() is always true for this fixed zero-argument construction)
        return 1; // LCOV_EXCL_LINE (unreachable: ok() is always true for this fixed zero-argument construction)
    }

    std::unordered_map<std::string, std::vector<std::string>> extMulti;
    extMulti["--name"] = {"a", "b"}; // LCOV_EXCL_BR_LINE (libstdc++ unordered_map duplicate-key/collision branches never run for these fixed keys inserted into a fresh map)
    extMulti["--v"] = {"1", "1", "1"}; // LCOV_EXCL_BR_LINE (libstdc++ unordered_map duplicate-key/collision branches never run for these fixed keys inserted into a fresh map)

    auto err = parser.setExternalValuesMultiChecked(extMulti);
    if (err) { // LCOV_EXCL_LINE (the fixed external values -- strings for --name, "1" repeats for the count flag --v -- always validate, so err is always empty)
        std::cerr << *err << "\n"; // LCOV_EXCL_LINE (unreachable: setExternalValuesMultiChecked cannot fail for this fixed map)
        return 1; // LCOV_EXCL_LINE (unreachable: setExternalValuesMultiChecked cannot fail for this fixed map)
    }

    std::cout << "name=" << parser.getFlag<std::string>("--name", "") << "\n";
    std::cout << "count=" << parser.getCount("--v", 0) << "\n";
    return 0;
}
