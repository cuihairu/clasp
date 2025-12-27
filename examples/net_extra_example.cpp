#include <iostream>
#include <string>
#include <vector>

#include "clasp/clasp.hpp"

int main(int argc, char** argv) {
    clasp::Command rootCmd("app", "Additional net flag types example");

    clasp::Command show("show", "Print parsed net values");
    show.withIPNetFlag("--ipnet", "", "ipnet", "IPNet (CIDR, canonicalized network address + prefix)", "10.0.0.0/8");
    show.withIPMaskFlag("--mask", "", "mask", "IPv4 netmask (contiguous bits)", "255.255.255.0");
    show.action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        std::cout << "ipnet=" << parser.getFlag<std::string>("--ipnet", "") << "\n";
        std::cout << "mask=" << parser.getFlag<std::string>("--mask", "") << "\n";
        return 0;
    });

    rootCmd.addCommand(std::move(show));
    return rootCmd.run(argc, argv);
}

