#include <iostream>
#include <string>
#include <vector>

#include "clasp/clasp.hpp"

namespace {

void testSimplifiedFlagMethods() {
    clasp::Command root("app", "Test simplified flag methods");

    // Test simplified withBytesFlag (4 params - non-string default, no ambiguity)
    root.withBytesFlag("--size", "-s", "Size", 1024);

    // Test simplified withCountFlag (4 params - int default, no ambiguity)
    root.withCountFlag("--verbose", "-v", "Verbose", 0);

    // Test simplified withIPFlag (5 params - all strings, use explicit varName)
    root.withIPFlag("--host", "-h", "host", "Host", std::string("127.0.0.1"));

    // Test simplified withIPMaskFlag (5 params)
    root.withIPMaskFlag("--mask", "-m", "mask", "Mask", std::string("255.255.255.0"));

    // Test simplified withCIDRFlag (5 params)
    root.withCIDRFlag("--cidr", "-c", "cidr", "CIDR", std::string("192.168.1.0/24"));

    // Test simplified withIPNetFlag (5 params)
    root.withIPNetFlag("--network", "-n", "network", "Network", std::string("192.168.1.0/24"));

    // Test simplified withURLFlag (5 params)
    root.withURLFlag("--url", "-u", "url", "URL", std::string("http://example.com"));

    // Test simplified withFlag (3 params)
    root.withFlag("--name", "-n", "Name");

    std::cout << "testSimplifiedFlagMethods: ok\n";
}

void testPersistentFlagMethods() {
    clasp::Command root("app", "Test persistent flag methods");

    // Test simplified withPersistentBytesFlag (4 params)
    root.withPersistentBytesFlag("--limit", "-l", "Limit", std::uint64_t(1024));

    // Test simplified withPersistentCountFlag (4 params)
    root.withPersistentCountFlag("--verbose", "-v", "Verbose", 0);

    // Test simplified withPersistentIPFlag (5 params)
    root.withPersistentIPFlag("--host", "-h", "host", "Host", std::string("127.0.0.1"));

    // Test simplified withPersistentIPMaskFlag (5 params)
    root.withPersistentIPMaskFlag("--mask", "-m", "mask", "Mask", std::string("255.255.255.0"));

    // Test simplified withPersistentCIDRFlag (5 params)
    root.withPersistentCIDRFlag("--cidr", "-c", "cidr", "CIDR", std::string("192.168.1.0/24"));

    // Test simplified withPersistentIPNetFlag (5 params)
    root.withPersistentIPNetFlag("--network", "-n", "network", "Network", std::string("192.168.1.0/24"));

    // Test simplified withPersistentURLFlag (5 params)
    root.withPersistentURLFlag("--url", "-u", "url", "URL", std::string("http://example.com"));

    std::cout << "testPersistentFlagMethods: ok\n";
}

void testPersistentFlagMarking() {
    clasp::Command root("app", "Test persistent flag marking");

    // Setup persistent flags
    root.withPersistentFlag("--debug", "-d", "debug", "Debug", true)
        .withPersistentFlag("--level", "-l", "level", "Level", 5)
        .withPersistentFlag("--name", "-n", "name", "Name", std::string("default"))
        .withPersistentFlag("--timeout", "-t", "timeout", "Timeout", std::uint64_t(1000))
        .withPersistentFlag("--count", "-c", "count", "Count", 0);

    // Test markPersistentFlagAnnotation
    root.markPersistentFlagAnnotation("--debug", "type", "boolean");

    // Test markPersistentFlagFilename (vector version)
    root.markPersistentFlagFilename("--name", std::vector<std::string>{"txt", "md"});

    // Test markPersistentFlagFilename (initializer_list version)
    root.markPersistentFlagFilename("--name", {"log", "csv"});

    // Test markPersistentFlagDirname
    root.markPersistentFlagDirname("--name");

    // Test markPersistentFlagNoOptDefaultValue
    root.markPersistentFlagNoOptDefaultValue("--debug", "true");

    std::cout << "testPersistentFlagMarking: ok\n";
}

void testCustomFunctions() {
    clasp::Command root("app", "Test custom functions");

    // Test setHelpFunc
    root.setHelpFunc([](const clasp::Command& cmd, std::ostream& out) {
        out << "Custom help for " << cmd.name();
    });

    // Test setUsageFunc
    root.setUsageFunc([](const clasp::Command& cmd, std::ostream& out) {
        out << "Custom usage for " << cmd.name();
    });

    // Test setFlagErrorFunc
    root.setFlagErrorFunc([](const clasp::Command& cmd, const std::string& err) -> std::string {
        return "Custom error: " + err;
    });

    std::cout << "testCustomFunctions: ok\n";
}

void testAnnotations() {
    clasp::Command root("app", "Test annotations");

    // Test annotations method
    root.withFlag("--name", "-n", "name", "Name", std::string("default"))
        .markFlagAnnotation("--name", "type", "string")
        .annotation("version", "1.0");

    // Access annotations
    const auto& ann = root.annotations();
    std::cout << "annotations count: " << ann.size() << "\n";

    std::cout << "testAnnotations: ok\n";
}

void testFlagWithShortName() {
    clasp::Command root("app", "Test flag with short name");

    // Test withFlag with short name
    root.withFlag("--message", "-m", "msg", "Message", std::string("hello"))
        .withFlag("--count", "-c", "cnt", "Count", 42)
        .withFlag("--debug", "-d", "dbg", "Debug", true);

    std::cout << "testFlagWithShortName: ok\n";
}

void testPersistentFlagWithShortName() {
    clasp::Command root("app", "Test persistent flag with short name");

    // Test withPersistentFlag with short name
    root.withPersistentFlag("--verbose", "-v", "verb", "Verbose", 0)
        .withPersistentFlag("--output", "-o", "out", "Output", std::string("stdout"));

    std::cout << "testPersistentFlagWithShortName: ok\n";
}

} // namespace

int main() {
    testSimplifiedFlagMethods();
    testPersistentFlagMethods();
    testPersistentFlagMarking();
    testCustomFunctions();
    testAnnotations();
    testFlagWithShortName();
    testPersistentFlagWithShortName();

    std::cout << "ok\n";
    return 0;
}
