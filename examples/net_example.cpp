#include <iostream>
#include <string>
#include <vector>

#include "clasp/clasp.hpp"

int main(int argc, char** argv) {
    clasp::Command rootCmd("app", "IP/CIDR flag types example");

    clasp::Command show("show", "Print parsed IP/CIDR values");
    show.withIPFlag("--ip", "", "ip", "IP address (IPv4 or IPv6)", "127.0.0.1");
    show.withCIDRFlag("--cidr", "", "cidr", "CIDR block (e.g. 10.0.0.0/8, 2001:db8::/32)", "10.0.0.0/8");
    show.action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        std::cout << "ip=" << parser.getFlag<std::string>("--ip", "") << "\n";
        std::cout << "cidr=" << parser.getFlag<std::string>("--cidr", "") << "\n";
        return 0;
    });

    rootCmd.addCommand(std::move(show));
    return rootCmd.run(argc, argv);
}

