#include "clasp/flag.hpp"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

static void usage() {
    std::cerr << "Usage: flag_convert_example <bool|int|int64|uint32|uint64|float|double|duration|string> [value]\n";
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
        if (type == "int64") {
            const auto v = std::get<std::int64_t>(clasp::Flag::convertToFlagValue<std::int64_t>(value));
            std::cout << "int64=" << v << "\n";
            return 0;
        }
        if (type == "uint32") {
            const auto v = std::get<std::uint32_t>(clasp::Flag::convertToFlagValue<std::uint32_t>(value));
            std::cout << "uint32=" << v << "\n";
            return 0;
        }
        if (type == "uint64") {
            const auto v = std::get<std::uint64_t>(clasp::Flag::convertToFlagValue<std::uint64_t>(value));
            std::cout << "uint64=" << v << "\n";
            return 0;
        }
        if (type == "float") {
            const auto v = std::get<float>(clasp::Flag::convertToFlagValue<float>(value));
            std::cout << std::fixed << std::setprecision(6) << "float=" << v << "\n";
            return 0;
        }
        if (type == "double") {
            const auto v = std::get<double>(clasp::Flag::convertToFlagValue<double>(value));
            std::cout << std::fixed << std::setprecision(6) << "double=" << v << "\n";
            return 0;
        }
        if (type == "duration") {
            const auto v = std::get<std::chrono::milliseconds>(clasp::Flag::convertToFlagValue<std::chrono::milliseconds>(value));
            std::cout << "duration_ms=" << v.count() << "\n";
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
