#include <iostream>
#include <string>

#include "clasp/clasp.hpp"

namespace {

void testNoArgsValidator() {
    clasp::Command root("app", "Test NoArgs");
    root.withFlag("--flag", "-f", "flag", "Flag", false);

    // Test NoArgs validator
    root.args(clasp::NoArgs());

    root.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
        return 0;
    });

    const char* argv[] = {"app"};
    root.run(1, const_cast<char**>(argv));
}

void testExactArgsValidator() {
    clasp::Command root("app", "Test ExactArgs");
    root.withFlag("--flag", "-f", "flag", "Flag", false);

    // Test ExactArgs validator (requires exactly 2 arguments)
    root.args(clasp::ExactArgs(2));

    root.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>& args) {
        (void)args;
        return 0;
    });

    const char* argv[] = {"app", "arg1", "arg2"};
    root.run(3, const_cast<char**>(argv));
}

void testMinimumNArgsValidator() {
    clasp::Command root("app", "Test MinimumNArgs");
    root.withFlag("--flag", "-f", "flag", "Flag", false);

    // Test MinimumNArgs validator (requires at least 1 argument)
    root.args(clasp::MinimumNArgs(1));

    root.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>& args) {
        (void)args;
        return 0;
    });

    const char* argv[] = {"app", "arg1"};
    root.run(2, const_cast<char**>(argv));
}

void testMaximumNArgsValidator() {
    clasp::Command root("app", "Test MaximumNArgs");
    root.withFlag("--flag", "-f", "flag", "Flag", false);

    // Test MaximumNArgs validator (requires at most 2 arguments)
    root.args(clasp::MaximumNArgs(2));

    root.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>& args) {
        (void)args;
        return 0;
    });

    const char* argv[] = {"app", "arg1"};
    root.run(2, const_cast<char**>(argv));
}

void testRangeArgsValidator() {
    clasp::Command root("app", "Test RangeArgs");
    root.withFlag("--flag", "-f", "flag", "Flag", false);

    // Test RangeArgs validator (requires 1-3 arguments)
    root.args(clasp::RangeArgs(1, 3));

    root.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>& args) {
        (void)args;
        return 0;
    });

    const char* argv[] = {"app", "arg1", "arg2"};
    root.run(3, const_cast<char**>(argv));
}

void testFlagVariantsWithoutShort() {
    clasp::Command root("app", "Test flags without short names");

    // Test withBytesFlag with empty short name
    root.withBytesFlag("--size", "", "size", "Size", std::uint64_t(0));

    // Test withCountFlag with empty short name
    root.withCountFlag("--verbose", "", "verbose", "Verbose");

    // Test withIPFlag with empty short name
    root.withIPFlag("--host", "", "host", "Host", std::string(""));

    // Test withIPMaskFlag with empty short name
    root.withIPMaskFlag("--mask", "", "mask", "Mask", std::string(""));

    // Test withCIDRFlag with empty short name
    root.withCIDRFlag("--network", "", "network", "Network", std::string(""));

    // Test withIPNetFlag with empty short name
    root.withIPNetFlag("--ipnet", "", "ipnet", "IPNet", std::string(""));

    // Test withURLFlag with empty short name
    root.withURLFlag("--url", "", "url", "URL", std::string(""));

    root.action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        auto size = parser.getFlag<std::uint64_t>("--size", 0);
        (void)size;
        return 0;
    });

    const char* argv[] = {"app", "--size", "1MB"};
    root.run(3, const_cast<char**>(argv));
}

void testPersistentFlagVariantsWithoutShort() {
    clasp::Command root("app", "Test persistent flags without short names");

    clasp::Command sub("sub", "Sub command");
    sub.withFlag("--sub-flag", "", "subflag", "Sub Flag", std::string(""));
    sub.action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        auto val = parser.getFlag<std::string>("--sub-flag", "");
        (void)val;
        return 0;
    });

    // Test persistent bytes flag
    root.withPersistentBytesFlag("--quota", "", "quota", "Quota", std::uint64_t(0));

    // Test persistent count flag
    root.withPersistentCountFlag("--verbose", "", "verbose", "Verbose");

    // Test persistent IP flag
    root.withPersistentIPFlag("--host", "", "host", "Host", std::string(""));

    // Test persistent IPMask flag
    root.withPersistentIPMaskFlag("--mask", "", "mask", "Mask", std::string(""));

    // Test persistent CIDR flag
    root.withPersistentCIDRFlag("--network", "", "network", "Network", std::string(""));

    // Test persistent IPNet flag
    root.withPersistentIPNetFlag("--ipnet", "", "ipnet", "IPNet", std::string(""));

    // Test persistent URL flag
    root.withPersistentURLFlag("--url", "", "url", "URL", std::string(""));

    root.action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        auto quota = parser.getFlag<std::uint64_t>("--quota", 0);
        (void)quota;
        return 0;
    });

    root.addCommand(std::move(sub));

    const char* argv[] = {"app", "--quota", "100MB"};
    root.run(3, const_cast<char**>(argv));
}

void testEnableCompletion() {
    clasp::Command root("app", "Test enableCompletion");

    // Test enableCompletion with default config
    root.enableCompletion();

    root.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
        return 0;
    });

    const char* argv[] = {"app"};
    root.run(1, const_cast<char**>(argv));
}

void testDisableSortMethods() {
    clasp::Command root("app", "Test disable sort");

    root.disableSortCommands(true);
    root.disableSortFlags(true);
    root.disableFlagsInUseLine(true);

    root.withFlag("--flag", "-f", "flag", "Flag", std::string(""));

    root.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
        return 0;
    });

    const char* argv[] = {"app", "--flag", "value"};
    root.run(3, const_cast<char**>(argv));
}

void testSilenceMethods() {
    clasp::Command root("app", "Test silence");

    root.silenceUsage(true);
    root.silenceErrors(true);

    root.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
        return 0;
    });

    const char* argv[] = {"app"};
    root.run(1, const_cast<char**>(argv));
}

} // namespace

int main() {
    testNoArgsValidator();
    testExactArgsValidator();
    testMinimumNArgsValidator();
    testMaximumNArgsValidator();
    testRangeArgsValidator();
    testFlagVariantsWithoutShort();
    testPersistentFlagVariantsWithoutShort();
    testEnableCompletion();
    testDisableSortMethods();
    testSilenceMethods();

    std::cout << "ok\n";
    return 0;
}
