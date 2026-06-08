#include <iostream>
#include <string>

#include "clasp/clasp.hpp"

namespace {

int testPersistentFilename() {
    clasp::Command root("app", "persistent filename completion");
    root.enableCompletion()
        .withPersistentFlag("--config", "-c", "config", "Config", std::string(""))
        .markPersistentFlagFilename("--config", {"yaml", "yml"});

    const char* argv[] = {"app", "__completeNoDesc", "--config", ""};
    return root.run(4, const_cast<char**>(argv));
}

int testPersistentDirname() {
    clasp::Command root("app", "persistent dirname completion");
    root.enableCompletion()
        .withPersistentFlag("--out-dir", "-o", "outDir", "Output dir", std::string(""))
        .markPersistentFlagDirname("--out-dir");

    const char* argv[] = {"app", "__completeNoDesc", "--out-dir", ""};
    return root.run(4, const_cast<char**>(argv));
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "missing case\n";
        return 1;
    }

    const std::string mode = argv[1];
    if (mode == "filename") return testPersistentFilename();
    if (mode == "dirname") return testPersistentDirname();

    std::cout << "unknown case\n";
    return 1;
}
