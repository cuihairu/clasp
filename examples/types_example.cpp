#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "clasp/clasp.hpp"

int main(int argc, char** argv) {
    clasp::Command rootCmd("app", "More flag types example");

    rootCmd.withPersistentFlag("--i64", "", "i64", "Signed 64-bit", std::int64_t(-1));
    rootCmd.withPersistentFlag("--u64", "", "u64", "Unsigned 64-bit", std::uint64_t(0));
    rootCmd.withPersistentFlag("--pi", "", "pi", "Double", 3.14);
    rootCmd.withPersistentFlag("--timeout", "", "timeout", "Duration", std::chrono::milliseconds(1500));

    rootCmd.action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        std::cout << "i64=" << parser.getFlag<std::int64_t>("--i64", 0) << "\n";
        std::cout << "u64=" << parser.getFlag<std::uint64_t>("--u64", 0) << "\n";
        std::cout << "pi=" << parser.getFlag<double>("--pi", 0.0) << "\n";
        const auto t = parser.getFlag<std::chrono::milliseconds>("--timeout", std::chrono::milliseconds(0));
        std::cout << "timeout_ms=" << t.count() << "\n";
        return 0;
    });

    return rootCmd.run(argc, argv);
}

