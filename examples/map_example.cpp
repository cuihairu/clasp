#include <algorithm>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "clasp/clasp.hpp"

static std::string render(const std::unordered_map<std::string, std::string>& m) {
    std::vector<std::string> keys;
    keys.reserve(m.size());
    for (const auto& [k, _] : m) keys.push_back(k);
    std::sort(keys.begin(), keys.end());

    std::string out;
    for (const auto& k : keys) {
        if (!out.empty()) out.push_back(',');
        out += k + "=" + m.at(k);
    }
    return out;
}

int main(int argc, char** argv) {
    clasp::Command rootCmd("app", "Map flags example");
    rootCmd.withPersistentFlag("--label", "", "label", "Key/value labels", std::string(""));

    rootCmd.action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        const auto labels = parser.getFlagMap("--label");
        std::cout << "labels=" << render(labels) << "\n";
        return 0;
    });

    return rootCmd.run(argc, argv);
}
