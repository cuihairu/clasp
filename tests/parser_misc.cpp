#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "clasp/clasp.hpp"

namespace {

void testHasValue() {
    clasp::Command root("app", "Test hasValue");
    root.withFlag("--name", "-n", "name", "Name", std::string(""));
    root.withFlag("--count", "-c", "count", "Count", 0);

    bool actionCalled = false;
    root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        auto hasName = parser.hasValue("--name");
        auto hasCount = parser.hasValue("--count");
        auto hasMissing = parser.hasValue("--missing");

        std::cout << "hasName: " << hasName << " hasCount: " << hasCount << " hasMissing: " << hasMissing << std::endl;
        actionCalled = true;
        return 0;
    });

    const char* argv[] = {"app", "--name", "test"};
    root.run(3, const_cast<char**>(argv));
}

void testHasExplicitValue() {
    clasp::Command root("app", "Test hasExplicitValue");
    root.withFlag("--opt", "-o", "opt", "Opt", std::string("default"));

    bool actionCalled = false;
    root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        auto hasOpt = parser.hasExplicitValue("--opt");
        std::cout << "hasOpt: " << hasOpt << std::endl;
        actionCalled = true;
        return 0;
    });

    const char* argv[] = {"app", "--opt", "value"};
    root.run(3, const_cast<char**>(argv));
}

void testParserOk() {
    clasp::Command root("app", "Test parser ok");
    root.withFlag("--flag", "-f", "flag", "Flag", false);

    bool actionCalled = false;
    root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        auto parserOk = parser.ok();
        std::cout << "parserOk: " << parserOk << std::endl;
        actionCalled = true;
        return 0;
    });

    const char* argv[] = {"app", "--flag"};
    root.run(2, const_cast<char**>(argv));
}

void testGetFlagInt() {
    clasp::Command root("app", "Test getFlag int");
    root.withFlag("--int", "-i", "int", "Int", 42);

    bool actionCalled = false;
    root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        auto intVal = parser.getFlag<int>("--int", 0);
        std::cout << "int: " << intVal << std::endl;
        actionCalled = true;
        return 0;
    });

    const char* argv[] = {"app", "--int", "100"};
    root.run(3, const_cast<char**>(argv));
}

void testGetFlagString() {
    clasp::Command root("app", "Test getFlag string");
    root.withFlag("--string", "-s", "string", "String", std::string("default"));

    bool actionCalled = false;
    root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        auto stringVal = parser.getFlag<std::string>("--string", "");
        std::cout << "string: " << stringVal << std::endl;
        actionCalled = true;
        return 0;
    });

    const char* argv[] = {"app", "--string", "hello"};
    root.run(3, const_cast<char**>(argv));
}

void testGetFlagBool() {
    clasp::Command root("app", "Test getFlag bool");
    root.withFlag("--bool", "-b", "bool", "Bool", false);

    bool actionCalled = false;
    root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        auto boolVal = parser.getFlag<bool>("--bool", false);
        std::cout << "bool: " << boolVal << std::endl;
        actionCalled = true;
        return 0;
    });

    const char* argv[] = {"app", "--bool"};
    root.run(2, const_cast<char**>(argv));
}

void testArrayMethods() {
    clasp::Command root("app", "Test array methods");
    root.withFlag("--items", "-I", "items", "Items", std::string(""));

    bool actionCalled = false;
    root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        auto strArray = parser.getStringArray("--items");
        std::cout << "strArray size: " << strArray.size() << std::endl;
        actionCalled = true;
        return 0;
    });

    const char* argv[] = {"app", "--items", "a", "--items", "b"};
    root.run(5, const_cast<char**>(argv));
}

} // namespace

int main() {
    std::cout << "Test 1: testHasValue" << std::endl;
    testHasValue();

    std::cout << "Test 2: testHasExplicitValue" << std::endl;
    testHasExplicitValue();

    std::cout << "Test 3: testParserOk" << std::endl;
    testParserOk();

    std::cout << "Test 4: testGetFlagInt" << std::endl;
    testGetFlagInt();

    std::cout << "Test 5: testGetFlagString" << std::endl;
    testGetFlagString();

    std::cout << "Test 6: testGetFlagBool" << std::endl;
    testGetFlagBool();

    std::cout << "Test 7: testArrayMethods" << std::endl;
    testArrayMethods();

    std::cout << "ok\n";
    return 0;
}
