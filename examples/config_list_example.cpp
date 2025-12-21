#include <iostream>
#include <string>
#include <vector>

#include "clasp/clasp.hpp"

static std::string join(const std::vector<std::string>& v, const char sep = ',') {
    std::string out;
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (i) out.push_back(sep);
        out += v[i];
    }
    return out;
}

int main(int argc, char** argv) {
    clasp::Command rootCmd("app", "Config list example");
    rootCmd.suggestions(false);

    clasp::Command doCmd("do", "Reads tag list from config");
    doCmd.withFlag("--config", "-c", "config", "Config file path", std::string(""));
    doCmd.withFlag("--tag", "-t", "tag", "Repeatable tag", std::string(""));
    doCmd.configFileFlag("--config");

    doCmd.action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        const auto tags = parser.getFlagValues("--tag");
        std::cout << "tags=" << join(tags) << "\n";
        return 0;
    });

    rootCmd.addCommand(std::move(doCmd));
    return rootCmd.run(argc, argv);
}
