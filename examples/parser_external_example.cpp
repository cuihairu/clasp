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
    if (!parser.ok()) {
        std::cerr << parser.error() << "\n";
        return 1;
    }

    std::unordered_map<std::string, std::vector<std::string>> extMulti;
    extMulti["--name"] = {"a", "b"};
    extMulti["--v"] = {"1", "1", "1"};

    auto err = parser.setExternalValuesMultiChecked(extMulti);
    if (err) {
        std::cerr << *err << "\n";
        return 1;
    }

    std::cout << "name=" << parser.getFlag<std::string>("--name", "") << "\n";
    std::cout << "count=" << parser.getCount("--v", 0) << "\n";
    return 0;
}
