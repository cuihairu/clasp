#include <iostream>
#include <string>
#include <vector>
#include <chrono>

#include "clasp/clasp.hpp"

namespace {

void testGetStringSlice() {
    clasp::Command root("app", "Test app");
    root.withFlag("--tags", "-t", "tags", "Tags", std::string(""))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto slice = parser.getStringSlice("--tags");
            std::cout << "slice:";
            for (const auto& s : slice) std::cout << s << ",";
            std::cout << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--tags", "a,b,c"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testGetStringToInt() {
    clasp::Command root("app", "Test app");
    root.withFlag("--labels", "-l", "labels", "Labels", std::string(""))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto map = parser.getStringToInt("--labels");
            std::cout << "map:";
            for (const auto& [k, v] : map) std::cout << k << "=" << v << ",";
            std::cout << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--labels", "a=1,b=2"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testGetExplicitFlagValues() {
    clasp::Command root("app", "Test app");
    root.withFlag("--name", "-n", "name", "Name", std::string("default"))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto vals = parser.getExplicitFlagValues("--name");
            std::cout << "explicit:";
            for (const auto& v : vals) std::cout << v << ",";
            std::cout << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--name", "test"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testHasExplicitValue() {
    clasp::Command root("app", "Test app");
    root.withFlag("--name", "-n", "name", "Name", std::string("default"))
        .withFlag("--count", "-c", "count", "Count", 0)
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            std::cout << "name:" << (parser.hasExplicitValue("--name") ? "yes" : "no") << "\n";
            std::cout << "count:" << (parser.hasExplicitValue("--count") ? "yes" : "no") << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--name", "test"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testOccurrences() {
    clasp::Command root("app", "Test app");
    root.withCountFlag("--verbose", "-v", "verbose", "Verbose", 0)
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            std::cout << "occ:" << parser.occurrences("--verbose") << "\n";
            return 0;
        });

    const char* argv[] = {"app", "-vvv"};
    int argc = 2;
    root.run(argc, const_cast<char**>(argv));
}

void testGetSlice() {
    clasp::Command root("app", "Test app");
    root.withFlag("--names", "-n", "names", "Names", std::string(""))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto slice = parser.getSlice<std::string>("--names");
            std::cout << "slice:" << slice.size() << "\n";
            return 0;
        });

    // getSlice splits by comma, so "a,b,c" gives 3 elements
    const char* argv[] = {"app", "--names", "a,b,c"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testGetBoolSlice() {
    clasp::Command root("app", "Test app");
    root.withFlag("--gates", "-g", "gates", "Gates", std::string(""))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto slice = parser.getBoolSlice("--gates");
            std::cout << "bools:" << slice.size() << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--gates", "true,false,yes"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testGetIntSlice() {
    clasp::Command root("app", "Test app");
    root.withFlag("--ports", "-p", "ports", "Ports", std::string(""))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto slice = parser.getIntSlice("--ports");
            std::cout << "ints:" << slice.size() << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--ports", "1,2,3"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testGetUint32Slice() {
    clasp::Command root("app", "Test app");
    root.withFlag("--ids", "-i", "ids", "IDs", std::string(""))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto slice = parser.getUint32Slice("--ids");
            std::cout << "u32s:" << slice.size() << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--ids", "1,2,3"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testGetFloatSlice() {
    clasp::Command root("app", "Test app");
    root.withFlag("--ratios", "-r", "ratios", "Ratios", std::string(""))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto slice = parser.getFloatSlice("--ratios");
            std::cout << "floats:" << slice.size() << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--ratios", "1.5,2.5"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testGetDoubleSlice() {
    clasp::Command root("app", "Test app");
    root.withFlag("--values", "-v", "values", "Values", std::string(""))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto slice = parser.getDoubleSlice("--values");
            std::cout << "doubles:" << slice.size() << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--values", "1.5,2.5"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testGetDurationSlice() {
    clasp::Command root("app", "Test app");
    root.withFlag("--times", "-t", "times", "Times", std::string(""))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto slice = parser.getDurationSlice("--times");
            std::cout << "durations:" << slice.size() << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--times", "1s,2s"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testGetBoolArray() {
    clasp::Command root("app", "Test app");
    root.withFlag("--gates", "-g", "gates", "Gates", std::string(""))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto arr = parser.getBoolArray("--gates");
            std::cout << "bools:" << arr.size() << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--gates", "true", "--gates", "false"};
    int argc = 5;
    root.run(argc, const_cast<char**>(argv));
}

void testGetUint32Array() {
    clasp::Command root("app", "Test app");
    root.withFlag("--ids", "-i", "ids", "IDs", std::string(""))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto arr = parser.getUint32Array("--ids");
            std::cout << "u32s:" << arr.size() << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--ids", "1", "--ids", "2"};
    int argc = 5;
    root.run(argc, const_cast<char**>(argv));
}

void testGetUint64Array() {
    clasp::Command root("app", "Test app");
    root.withFlag("--ids", "-i", "ids", "IDs", std::string(""))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto arr = parser.getUint64Array("--ids");
            std::cout << "u64s:" << arr.size() << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--ids", "1", "--ids", "2"};
    int argc = 5;
    root.run(argc, const_cast<char**>(argv));
}

void testGetFloatArray() {
    clasp::Command root("app", "Test app");
    root.withFlag("--ratios", "-r", "ratios", "Ratios", std::string(""))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto arr = parser.getFloatArray("--ratios");
            std::cout << "floats:" << arr.size() << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--ratios", "1.5", "--ratios", "2.5"};
    int argc = 5;
    root.run(argc, const_cast<char**>(argv));
}

void testGetDoubleArray() {
    clasp::Command root("app", "Test app");
    root.withFlag("--values", "-v", "values", "Values", std::string(""))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto arr = parser.getDoubleArray("--values");
            std::cout << "doubles:" << arr.size() << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--values", "1.5", "--values", "2.5"};
    int argc = 5;
    root.run(argc, const_cast<char**>(argv));
}

void testGetDurationArray() {
    clasp::Command root("app", "Test app");
    root.withFlag("--times", "-t", "times", "Times", std::string(""))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto arr = parser.getDurationArray("--times");
            std::cout << "durations:" << arr.size() << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--times", "1s", "--times", "2s"};
    int argc = 5;
    root.run(argc, const_cast<char**>(argv));
}

void testGetStringToString() {
    clasp::Command root("app", "Test app");
    root.withFlag("--labels", "-l", "labels", "Labels", std::string(""))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto map = parser.getStringToString("--labels");
            std::cout << "map:";
            for (const auto& [k, v] : map) std::cout << k << "=" << v << ",";
            std::cout << "\n";
            return 0;
        });

    // unordered_map iteration order varies across platforms
    const char* argv[] = {"app", "--labels", "a=1,b=2"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testGetStringToUint64() {
    clasp::Command root("app", "Test app");
    root.withFlag("--counts", "-c", "counts", "Counts", std::string(""))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto map = parser.getStringToUint64("--counts");
            std::cout << "map:";
            for (const auto& [k, v] : map) std::cout << k << "=" << v << ",";
            std::cout << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--counts", "a=1,b=2"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testGetStringToDouble() {
    clasp::Command root("app", "Test app");
    root.withFlag("--ratios", "-r", "ratios", "Ratios", std::string(""))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto map = parser.getStringToDouble("--ratios");
            std::cout << "map:";
            for (const auto& [k, v] : map) std::cout << k << "=" << v << ",";
            std::cout << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--ratios", "a=1.5,b=2.5"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testGetStringToDuration() {
    clasp::Command root("app", "Test app");
    root.withFlag("--times", "-t", "times", "Times", std::string(""))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto map = parser.getStringToDuration("--times");
            std::cout << "map:";
            for (const auto& [k, v] : map) std::cout << k << "=" << v.count() << ",";
            std::cout << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--times", "a=1s,b=2s"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testGetStringTo() {
    clasp::Command root("app", "Test app");
    root.withFlag("--mode", "-m", "mode", "Mode", std::string(""))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto map = parser.getStringTo<std::string>("--mode");
            std::cout << "map:";
            for (const auto& [k, v] : map) std::cout << k << "=" << v << ",";
            std::cout << "\n";
            return 0;
        });

    // getStringTo needs key=value pairs separated by comma
    const char* argv[] = {"app", "--mode", "a=x,b=y"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testGetFlagValuesAs() {
    clasp::Command root("app", "Test app");
    root.withFlag("--count", "-c", "count", "Count", 0)
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto vals = parser.getFlagValuesAs<int>("--count");
            std::cout << "vals:" << vals.size() << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--count", "1", "--count", "2"};
    int argc = 5;
    root.run(argc, const_cast<char**>(argv));
}

void testGetFlagMap() {
    clasp::Command root("app", "Test app");
    root.withFlag("--labels", "-l", "labels", "Labels", std::string(""))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto map = parser.getFlagMap("--labels");
            std::cout << "map:";
            for (const auto& [k, v] : map) std::cout << k << "=" << v << ",";
            std::cout << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--labels", "a=1,b=2"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testGetFlagValuesSplit() {
    clasp::Command root("app", "Test app");
    root.withFlag("--tags", "-t", "tags", "Tags", std::string(""))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto vals = parser.getFlagValuesSplit("--tags");
            std::cout << "vals:";
            for (const auto& v : vals) std::cout << v << ",";
            std::cout << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--tags", "a,b,c"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testGetFlagValues() {
    clasp::Command root("app", "Test app");
    root.withFlag("--name", "-n", "name", "Name", std::string(""))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto vals = parser.getFlagValues("--name");
            std::cout << "vals:" << vals.size() << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--name", "test"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testPositionals() {
    clasp::Command root("app", "Test app");
    root.action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        auto pos = parser.positionals();
        std::cout << "pos:";
        for (const auto& p : pos) std::cout << p << ",";
        std::cout << "\n";
        return 0;
    });

    const char* argv[] = {"app", "arg1", "arg2"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testHasValue() {
    clasp::Command root("app", "Test app");
    root.withFlag("--name", "-n", "name", "Name", std::string("default"))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            std::cout << "name:" << (parser.hasValue("--name") ? "yes" : "no") << "\n";
            std::cout << "missing:" << (parser.hasValue("--missing") ? "yes" : "no") << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--name", "test"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testDurationFlag() {
    clasp::Command root("app", "Test app");
    root.withFlag("--timeout", "-t", "timeout", "Timeout", std::chrono::milliseconds(0))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto d = parser.getFlag<std::chrono::milliseconds>("--timeout");
            std::cout << "timeout:" << d.count() << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--timeout", "1s500ms"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testUint64Flag() {
    clasp::Command root("app", "Test app");
    root.withFlag("--size", "-s", "size", "Size", std::uint64_t(0))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto v = parser.getFlag<std::uint64_t>("--size");
            std::cout << "size:" << v << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--size", "100"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testInt64Flag() {
    clasp::Command root("app", "Test app");
    root.withFlag("--value", "-v", "value", "Value", std::int64_t(0))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto v = parser.getFlag<std::int64_t>("--value");
            std::cout << "value:" << v << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--value", "-100"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testUint32Flag() {
    clasp::Command root("app", "Test app");
    root.withFlag("--port", "-p", "port", "Port", std::uint32_t(0))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto v = parser.getFlag<std::uint32_t>("--port");
            std::cout << "port:" << v << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--port", "8080"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testDoubleFlag() {
    clasp::Command root("app", "Test app");
    root.withFlag("--ratio", "-r", "ratio", "Ratio", 0.0)
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto v = parser.getFlag<double>("--ratio");
            std::cout << "ratio:" << v << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--ratio", "3.14"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testFloatFlag() {
    clasp::Command root("app", "Test app");
    root.withFlag("--ratio", "-r", "ratio", "Ratio", 0.0f)
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto v = parser.getFlag<float>("--ratio");
            std::cout << "ratio:" << v << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--ratio", "1.5"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testCIDRFlag() {
    clasp::Command root("app", "Test app");
    root.withCIDRFlag("--cidr", "-c", "cidr", "CIDR", std::string(""))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto v = parser.getFlag<std::string>("--cidr");
            std::cout << "cidr:" << v << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--cidr", "10.0.0.0/8"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testIPMaskFlag() {
    clasp::Command root("app", "Test app");
    root.withIPMaskFlag("--mask", "-m", "mask", "Mask", std::string(""))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto v = parser.getFlag<std::string>("--mask");
            std::cout << "mask:" << v << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--mask", "255.255.255.0"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testIPNetFlag() {
    clasp::Command root("app", "Test app");
    root.withIPNetFlag("--net", "-n", "net", "Net", std::string(""))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto v = parser.getFlag<std::string>("--net");
            std::cout << "net:" << v << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--net", "10.0.0.0/8"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testBytesFlag() {
    clasp::Command root("app", "Test app");
    root.withBytesFlag("--size", "-s", "size", "Size", 0)
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto v = parser.getFlag<std::uint64_t>("--size");
            std::cout << "size:" << v << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--size", "1KiB"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testCountFlag() {
    clasp::Command root("app", "Test app");
    root.withCountFlag("--verbose", "-v", "verbose", "Verbose", 0)
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto v = parser.getFlag<int>("--verbose");
            std::cout << "count:" << v << "\n";
            return 0;
        });

    const char* argv[] = {"app", "-vvv"};
    int argc = 2;
    root.run(argc, const_cast<char**>(argv));
}

void testIPv6Address() {
    clasp::Command root("app", "Test app");
    root.withIPFlag("--host", "", "host", "Host", std::string(""))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto v = parser.getFlag<std::string>("--host");
            std::cout << "host:" << v << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--host", "2001:db8::1"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testIPv6CIDR() {
    clasp::Command root("app", "Test app");
    root.withCIDRFlag("--cidr", "-c", "cidr", "CIDR", std::string(""))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto v = parser.getFlag<std::string>("--cidr");
            std::cout << "cidr:" << v << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--cidr", "2001:db8::1/32"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testURLFlag() {
    clasp::Command root("app", "Test app");
    root.withURLFlag("--url", "-u", "url", "URL", std::string(""))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto v = parser.getFlag<std::string>("--url");
            std::cout << "url:" << v << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--url", "HTTP://Example.COM/path"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testURLRelative() {
    clasp::Command root("app", "Test app");
    root.withURLFlag("--url", "-u", "url", "URL", std::string(""))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto v = parser.getFlag<std::string>("--url");
            std::cout << "url:" << v << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--url", "foo/bar"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testIPv6URL() {
    clasp::Command root("app", "Test app");
    root.withURLFlag("--url", "-u", "url", "URL", std::string(""))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto v = parser.getFlag<std::string>("--url");
            std::cout << "url:" << v << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--url", "http://[::1]:8080/path"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testNoOptFlag() {
    clasp::Command root("app", "Test app");
    root.withFlag("--mode", "-m", "mode", "Mode", std::string(""))
        .markFlagNoOptDefaultValue("--mode", "auto")
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto v = parser.getFlag<std::string>("--mode");
            std::cout << "mode:" << v << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--mode"};
    int argc = 2;
    root.run(argc, const_cast<char**>(argv));
}

void testNoOptFlagWithNextFlag() {
    clasp::Command root("app", "Test app");
    root.withFlag("--mode", "-m", "mode", "Mode", std::string(""))
        .withFlag("--other", "-o", "other", "Other", false)
        .markFlagNoOptDefaultValue("--mode", "auto")
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto v = parser.getFlag<std::string>("--mode");
            std::cout << "mode:" << v << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--mode", "--other"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testNoOptFlagWithNextArg() {
    clasp::Command root("app", "Test app");
    root.withFlag("--mode", "-m", "mode", "Mode", std::string(""))
        .markFlagNoOptDefaultValue("--mode", "auto")
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto v = parser.getFlag<std::string>("--mode");
            std::cout << "mode:" << v << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--mode", "manual"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testHexUint64() {
    clasp::Command root("app", "Test app");
    root.withFlag("--size", "-s", "size", "Size", std::uint64_t(0))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto v = parser.getFlag<std::uint64_t>("--size");
            std::cout << "size:" << v << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--size", "0xFF"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testHexUint32() {
    clasp::Command root("app", "Test app");
    root.withFlag("--port", "-p", "port", "Port", std::uint32_t(0))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto v = parser.getFlag<std::uint32_t>("--port");
            std::cout << "port:" << v << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--port", "0xFF"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testHexInt64() {
    clasp::Command root("app", "Test app");
    root.withFlag("--value", "-v", "value", "Value", std::int64_t(0))
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto v = parser.getFlag<std::int64_t>("--value");
            std::cout << "value:" << v << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--value", "0xFF"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testBoolTrueValues() {
    clasp::Command root("app", "Test app");
    root.withFlag("--debug", "-d", "debug", "Debug", false)
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto v = parser.getFlag<bool>("--debug");
            std::cout << "debug:" << (v ? "true" : "false") << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--debug", "on"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testBytesLargeUnits() {
    clasp::Command root("app", "Test app");
    root.withBytesFlag("--size", "-s", "size", "Size", 0)
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto v = parser.getFlag<std::uint64_t>("--size");
            std::cout << "size:" << v << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--size", "1TiB"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

void testBytesEB() {
    clasp::Command root("app", "Test app");
    root.withBytesFlag("--size", "-s", "size", "Size", 0)
        .action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto v = parser.getFlag<std::uint64_t>("--size");
            std::cout << "size:" << v << "\n";
            return 0;
        });

    const char* argv[] = {"app", "--size", "1EiB"};
    int argc = 3;
    root.run(argc, const_cast<char**>(argv));
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "ok\n";
        return 0;
    }

    std::string test = argv[1];

    if (test == "string-slice") testGetStringSlice();
    else if (test == "string-to-int") testGetStringToInt();
    else if (test == "explicit-values") testGetExplicitFlagValues();
    else if (test == "has-explicit") testHasExplicitValue();
    else if (test == "occurrences") testOccurrences();
    else if (test == "get-slice") testGetSlice();
    else if (test == "bool-slice") testGetBoolSlice();
    else if (test == "int-slice") testGetIntSlice();
    else if (test == "uint32-slice") testGetUint32Slice();
    else if (test == "float-slice") testGetFloatSlice();
    else if (test == "double-slice") testGetDoubleSlice();
    else if (test == "duration-slice") testGetDurationSlice();
    else if (test == "bool-array") testGetBoolArray();
    else if (test == "uint32-array") testGetUint32Array();
    else if (test == "uint64-array") testGetUint64Array();
    else if (test == "float-array") testGetFloatArray();
    else if (test == "double-array") testGetDoubleArray();
    else if (test == "duration-array") testGetDurationArray();
    else if (test == "string-to-string") testGetStringToString();
    else if (test == "string-to-uint64") testGetStringToUint64();
    else if (test == "string-to-double") testGetStringToDouble();
    else if (test == "string-to-duration") testGetStringToDuration();
    else if (test == "string-to") testGetStringTo();
    else if (test == "flag-values-as") testGetFlagValuesAs();
    else if (test == "flag-map") testGetFlagMap();
    else if (test == "flag-values-split") testGetFlagValuesSplit();
    else if (test == "flag-values") testGetFlagValues();
    else if (test == "positionals") testPositionals();
    else if (test == "has-value") testHasValue();
    else if (test == "duration-flag") testDurationFlag();
    else if (test == "uint64-flag") testUint64Flag();
    else if (test == "int64-flag") testInt64Flag();
    else if (test == "uint32-flag") testUint32Flag();
    else if (test == "double-flag") testDoubleFlag();
    else if (test == "float-flag") testFloatFlag();
    else if (test == "cidr-flag") testCIDRFlag();
    else if (test == "ipmask-flag") testIPMaskFlag();
    else if (test == "ipnet-flag") testIPNetFlag();
    else if (test == "bytes-flag") testBytesFlag();
    else if (test == "count-flag") testCountFlag();
    else if (test == "ipv6") testIPv6Address();
    else if (test == "ipv6-cidr") testIPv6CIDR();
    else if (test == "url-flag") testURLFlag();
    else if (test == "url-relative") testURLRelative();
    else if (test == "url-ipv6") testIPv6URL();
    else if (test == "noopt") testNoOptFlag();
    else if (test == "noopt-next-flag") testNoOptFlagWithNextFlag();
    else if (test == "noopt-next-arg") testNoOptFlagWithNextArg();
    else if (test == "hex-uint64") testHexUint64();
    else if (test == "hex-uint32") testHexUint32();
    else if (test == "hex-int64") testHexInt64();
    else if (test == "bool-true") testBoolTrueValues();
    else if (test == "bytes-tib") testBytesLargeUnits();
    else if (test == "bytes-eib") testBytesEB();
    else {
        std::cout << "unknown test: " << test << "\n";
        return 1;
    }

    return 0;
}
