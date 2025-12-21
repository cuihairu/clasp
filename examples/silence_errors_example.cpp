#include <string>
#include <vector>

#include "clasp/clasp.hpp"

int main(int argc, char** argv) {
    clasp::Command root("app", "SilenceErrors example");
    root.silenceErrors(true);

    clasp::Command ok("ok", "Always succeeds");
    ok.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });

    root.addCommand(std::move(ok));
    return root.run(argc, argv);
}

