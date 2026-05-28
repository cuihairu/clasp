#include <iostream>
#include <string>
#include <vector>

#include "clasp/clasp.hpp"

int main(int argc, char** argv) {
    clasp::Command root("tpl", "Template coverage");
    root.version("1.0.0");
    root.setVersionTemplate("AppVersion:{{.Version}}\n");
    root.setHelpTemplate("CUSTOM_HELP {{.CommandPath}}\n");
    root.setUsageTemplate("CUSTOM_USAGE {{.CommandPath}}\n");

    clasp::Command sub("sub", "Sub");
    sub.withFlag("--name", "-n", "name", "Name", std::string(""));
    sub.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
        std::cout << "ok\n";
        return 0;
    });

    root.addCommand(std::move(sub));
    return root.run(argc, argv);
}
