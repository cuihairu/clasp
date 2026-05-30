#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>

#include "clasp/clasp.hpp"

namespace {

void testGetFlag() {
    clasp::Command root("app", "Test getFlag");
    root.withFlag("--name", "-n", "name", "Name", std::string("default"));
    root.withFlag("--count", "-c", "count", "Count", 42);
    root.withFlag("--debug", "-d", "debug", "Debug", false);

    bool actionCalled = false;
    root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        auto name = parser.getFlag<std::string>("--name", "default");
        auto count = parser.getFlag<int>("--count", 0);
        auto debug = parser.getFlag<bool>("--debug", false);
        auto missing = parser.getFlag<std::string>("--missing", "missing_default");

        (void)name; (void)count; (void)debug; (void)missing;
        actionCalled = true;
        return 0;
    });

    const char* argv[] = {"app", "--name", "test", "--count", "10", "--debug"};
    root.run(5, const_cast<char**>(argv));
}

void testArrayGetters() {
    clasp::Command root("app", "Test array getters");
    root.withFlag("--items", "-i", "items", "Items", std::string(""));
    root.withFlag("--counts", "-c", "counts", "Counts", std::string(""));
    root.withFlag("--int64s", "", "int64s", "Int64s", std::string(""));

    bool actionCalled = false;
    root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        auto strArray = parser.getStringArray("--items");
        auto intArray = parser.getIntArray("--counts");
        auto int32Array = parser.getInt32Array("--counts");
        auto int64Array = parser.getInt64Array("--int64s");
        auto u32Array = parser.getUint32Array("--counts");
        auto u64Array = parser.getUint64Array("--counts");

        (void)strArray; (void)intArray; (void)int32Array; (void)int64Array; (void)u32Array; (void)u64Array;
        actionCalled = true;
        return 0;
    });

    const char* argv[] = {"app", "--items", "a", "--items", "b", "--counts", "1", "--counts", "2"};
    root.run(9, const_cast<char**>(argv));
}

void testGetArray() {
    clasp::Command root("app", "Test getArray");
    root.withFlag("--strings", "-s", "strings", "Strings", std::string(""));
    root.withFlag("--counts", "-c", "counts", "Counts", 0);
    root.withFlag("--durations", "-d", "durations", "Durations", std::chrono::milliseconds{0});

    bool actionCalled = false;
    root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        auto strArray = parser.getArray<std::string>("--strings");
        auto countArray = parser.getArray<int>("--counts");
        auto durArray = parser.getArray<std::chrono::milliseconds>("--durations");

        (void)strArray; (void)countArray; (void)durArray;
        actionCalled = true;
        return 0;
    });

    const char* argv[] = {"app", "--strings", "a", "--strings", "b"};
    root.run(5, const_cast<char**>(argv));
}

void testGetInt32Slice() {
    clasp::Command root("app", "Test int32 slice");
    root.withFlag("--numbers", "-n", "numbers", "Numbers", std::string(""));

    bool actionCalled = false;
    root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        auto int32Slice = parser.getInt32Slice("--numbers", ',');
        (void)int32Slice;
        actionCalled = true;
        return 0;
    });

    const char* argv[] = {"app", "--numbers", "1", "2", "3"};
    root.run(5, const_cast<char**>(argv));
}

void testGetInt64Slice() {
    clasp::Command root("app", "Test int64 slice");
    root.withFlag("--big", "-b", "big", "Big", std::string(""));

    bool actionCalled = false;
    root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        auto int64Slice = parser.getInt64Slice("--big", ',');
        (void)int64Slice;
        actionCalled = true;
        return 0;
    });

    const char* argv[] = {"app", "--big", "1000000", "2000000"};
    root.run(4, const_cast<char**>(argv));
}

void testGetUint32Slice() {
    clasp::Command root("app", "Test uint32 slice");
    root.withFlag("--nums", "-n", "nums", "Nums", std::string(""));

    bool actionCalled = false;
    root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        auto uint32Slice = parser.getUint32Slice("--nums", ',');
        (void)uint32Slice;
        actionCalled = true;
        return 0;
    });

    const char* argv[] = {"app", "--nums", "10", "20", "30"};
    root.run(5, const_cast<char**>(argv));
}

void testGetUint64Slice() {
    clasp::Command root("app", "Test uint64 slice");
    root.withFlag("--big", "-b", "big", "Big", std::string(""));

    bool actionCalled = false;
    root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        auto uint64Slice = parser.getUint64Slice("--big", ',');
        (void)uint64Slice;
        actionCalled = true;
        return 0;
    });

    const char* argv[] = {"app", "--big", "4000000000", "5000000000"};
    root.run(4, const_cast<char**>(argv));
}

void testGetStringToInt32() {
    clasp::Command root("app", "Test string to int32");
    root.withFlag("--map", "-m", "map", "Map", std::string(""));

    bool actionCalled = false;
    root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        auto int32Map = parser.getStringToInt32("--map", ',', '=');
        (void)int32Map;
        actionCalled = true;
        return 0;
    });

    const char* argv[] = {"app", "--map", "a=10", "b=20", "c=30"};
    root.run(5, const_cast<char**>(argv));
}

void testGetStringToInt64() {
    clasp::Command root("app", "Test string to int64");
    root.withFlag("--bigmap", "-b", "bigmap", "BigMap", std::string(""));

    bool actionCalled = false;
    root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        auto int64Map = parser.getStringToInt64("--bigmap", ',', '=');
        (void)int64Map;
        actionCalled = true;
        return 0;
    });

    const char* argv[] = {"app", "--bigmap", "x=1000000", "y=2000000"};
    root.run(4, const_cast<char**>(argv));
}

void testGetStringToUint32() {
    clasp::Command root("app", "Test string to uint32");
    root.withFlag("--umap", "-u", "umap", "Umap", std::string(""));

    bool actionCalled = false;
    root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        auto uint32Map = parser.getStringToUint32("--umap", ',', '=');
        (void)uint32Map;
        actionCalled = true;
        return 0;
    });

    const char* argv[] = {"app", "--umap", "a=100", "b=200"};
    root.run(4, const_cast<char**>(argv));
}

void testGetStringToBool() {
    clasp::Command root("app", "Test string to bool");
    root.withFlag("--bools", "-b", "bools", "Bools", std::string(""));

    bool actionCalled = false;
    root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        auto boolMap = parser.getStringToBool("--bools", ',', '=');
        (void)boolMap;
        actionCalled = true;
        return 0;
    });

    const char* argv[] = {"app", "--bools", "flag1=true", "flag2=false", "flag3=yes"};
    root.run(5, const_cast<char**>(argv));
}

void testHasFlag() {
    clasp::Command root("app", "Test hasFlag");
    root.withFlag("--name", "-n", "name", "Name", std::string(""));
    root.withFlag("--debug", "-d", "debug", "Debug", false);

    bool actionCalled = false;
    root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        auto hasName = parser.hasFlag("--name");
        auto hasDebug = parser.hasFlag("--debug");
        auto hasMissing = parser.hasFlag("--missing");

        (void)hasName; (void)hasDebug; (void)hasMissing;
        actionCalled = true;
        return 0;
    });

    const char* argv[] = {"app", "--name", "test"};
    root.run(3, const_cast<char**>(argv));
}

void testGetCountWithDefault() {
    clasp::Command root("app", "Test getCount with default");
    root.withCountFlag("--verbose", "-v", "verbose", "Verbose");

    bool actionCalled = false;
    root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        auto count1 = parser.getCount("--verbose", 0);
        auto count2 = parser.getCount("--missing", 5);

        (void)count1; (void)count2;
        actionCalled = true;
        return 0;
    });

    const char* argv[] = {"app", "-v", "-v"};
    root.run(3, const_cast<char**>(argv));
}

} // namespace

int main() {
    testGetFlag();
    testArrayGetters();
    testGetArray();
    testGetInt32Slice();
    testGetInt64Slice();
    testGetUint32Slice();
    testGetUint64Slice();
    testGetStringToInt32();
    testGetStringToInt64();
    testGetStringToUint32();
    testGetStringToBool();
    testHasFlag();
    testGetCountWithDefault();

    std::cout << "ok\n";
    return 0;
}
