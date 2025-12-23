#include <iostream>
#include <string>
#include <vector>

#include "clasp/clasp.hpp"

static std::string join(const std::vector<std::string>& parts, const std::string& sep) {
    std::string out;
    for (std::size_t i = 0; i < parts.size(); ++i) {
        if (i) out += sep;
        out += parts[i];
    }
    return out;
}

int main(int argc, char** argv) {
    clasp::Command root("app", "Args validation example");

    clasp::Command noargs("noargs", "Accepts no args");
    noargs.args(clasp::NoArgs()).action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
        std::cout << "ok\n";
        return 0;
    });

    clasp::Command exact("exact", "Accepts exactly 2 args");
    exact.args(clasp::ExactArgs(2)).action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>& args) {
        std::cout << join(args, ",") << "\n";
        return 0;
    });

    clasp::Command range("range", "Accepts 1 or 2 args");
    range.args(clasp::RangeArgs(1, 2)).action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>& args) {
        std::cout << join(args, ",") << "\n";
        return 0;
    });

    clasp::Command silenceUsage("silence_usage", "Args error without usage");
    silenceUsage.silenceUsage().args(clasp::ExactArgs(1)).action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
        std::cout << "unreachable\n";
        return 0;
    });

    clasp::Command silenceErrors("silence_errors", "Args error without error text");
    silenceErrors.silenceErrors().args(clasp::ExactArgs(1)).action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
        std::cout << "unreachable\n";
        return 0;
    });

    root.addCommand(std::move(noargs));
    root.addCommand(std::move(exact));
    root.addCommand(std::move(range));
    root.addCommand(std::move(silenceUsage));
    root.addCommand(std::move(silenceErrors));
    return root.run(argc, argv);
}

