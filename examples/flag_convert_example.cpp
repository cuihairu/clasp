#include "clasp/flag.hpp"

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

static void usage() {
    std::cerr << "Usage: flag_convert_example <bool|int|float|string> [value]\n";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        usage();
        return 2;
    }

    const std::string type = argv[1];
    const std::string value = (argc >= 3) ? std::string(argv[2]) : std::string();

    try {
        if (type == "bool") {
            const auto v = std::get<bool>(clasp::Flag::convertToFlagValue<bool>(value));
            std::cout << "bool=" << (v ? "true" : "false") << "\n";
            return 0;
        }
        if (type == "int") {
            const auto v = std::get<int>(clasp::Flag::convertToFlagValue<int>(value));
            std::cout << "int=" << v << "\n";
            return 0;
        }
        if (type == "float") {
            const auto v = std::get<float>(clasp::Flag::convertToFlagValue<float>(value));
            std::cout << std::fixed << std::setprecision(6) << "float=" << v << "\n";
            return 0;
        }
        if (type == "string") {
            const auto v = std::get<std::string>(clasp::Flag::convertToFlagValue<std::string>(value));
            std::cout << "string=" << v << "\n";
            return 0;
        }
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }

    usage();
    return 2;
}

