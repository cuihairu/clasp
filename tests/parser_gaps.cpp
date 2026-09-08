#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "clasp/clasp.hpp"

namespace {

int g_failures = 0;

void expect(bool cond, const char* label) {
    std::cout << label << ": " << (cond ? "pass" : "fail") << std::endl;
    if (!cond) ++g_failures;
}

clasp::Parser makeParser(const std::vector<std::string>& args, const std::vector<clasp::Flag>& flags) {
    std::vector<char*> ptrs;
    ptrs.push_back(const_cast<char*>("app"));
    for (const auto& a : args) ptrs.push_back(const_cast<char*>(a.c_str()));
    return clasp::Parser(static_cast<int>(ptrs.size()), ptrs.data(), flags);
}

clasp::Flag annotatedFlag(const char* lng, const char* key, clasp::FlagValue def) {
    clasp::Flag f(lng, "", "D", "var", std::move(def));
    f.setAnnotation(key, "true");
    return f;
}

// getFlag<T> resolution order: CLI values, external multi values, external
// single values, declared defaults, caller-provided default.
void testGetFlagSources() {
    std::vector<clasp::Flag> flags;
    flags.push_back(clasp::Flag("--def", "", "D", "var", std::string("defaultval")));

    {
        auto p = makeParser({}, flags);
        expect(p.getFlag<std::string>("--def", "sentinel") == "defaultval", "getFlag uses declared default");
    }
    {
        auto p = makeParser({}, flags);
        expect(p.getFlag<std::string>("--absent", "sentinel") == "sentinel", "getFlag falls back to caller default");
    }
    {
        auto p = makeParser({}, flags);
        p.setExternalValues({{"--ext", "extval"}});
        expect(p.getFlag<std::string>("--ext", "sentinel") == "extval", "getFlag uses external single value");
    }
    {
        auto p = makeParser({}, flags);
        p.setExternalValuesMulti({{"--multi", {"first", "second"}}});
        expect(p.getFlag<std::string>("--multi", "sentinel") == "second", "getFlag uses last external multi value");
    }
    {
        auto p = makeParser({"--def", "cli"}, flags);
        p.setExternalValues({{"--def", "extval"}});
        expect(p.getFlag<std::string>("--def", "sentinel") == "cli", "getFlag prefers CLI over external");
    }
}

void testGetFlagValuesFallback() {
    std::vector<clasp::Flag> flags;
    auto p = makeParser({}, flags);
    expect(p.getFlagValues("--absent").empty(), "getFlagValues empty for unknown flag");
}

void testCheckedExternalSources() {
    std::vector<clasp::Flag> flags;
    flags.push_back(clasp::Flag("--num", "", "N", "var", 0));

    {
        auto p = makeParser({}, flags);
        auto err = p.setExternalValuesChecked({{"--num", "notanumber"}});
        expect(err.has_value(), "setExternalValuesChecked rejects invalid value");
    }
    {
        auto p = makeParser({}, flags);
        auto err = p.setExternalValuesMultiChecked({{"--num", {"7", "bad"}}});
        expect(err.has_value(), "setExternalValuesMultiChecked rejects invalid value");
    }
    {
        auto p = makeParser({}, flags);
        auto err = p.setExternalValuesChecked({{"--num", "7"}});
        expect(!err.has_value(), "setExternalValuesChecked accepts valid value");
    }
    {
        auto p = makeParser({}, flags);
        auto err = p.setExternalValuesMultiChecked({{"--num", {"7"}}});
        expect(!err.has_value(), "setExternalValuesMultiChecked accepts valid value");
    }
}

void testMapHelpers() {
    std::vector<clasp::Flag> flags;
    flags.push_back(clasp::Flag("--list", "", "L", "var", std::string("")));

    {
        auto p = makeParser({"--list", "k=v,bad,=nokey,x"}, flags);
        const auto m = p.getFlagMap("--list");
        expect(m.count("k") == 1 && m.at("k") == "v", "getFlagMap parses k=v");
        expect(m.count("bad") == 0, "getFlagMap skips entry without =");
        expect(m.count("") == 0, "getFlagMap skips empty key");
        expect(m.count("x") == 0, "getFlagMap skips bare entry");
    }
    {
        auto p = makeParser({"--list", "a=1,b=2"}, flags);
        expect(p.getStringTo<int>("--list").at("a") == 1, "getStringTo<int>");
        expect(p.getStringToInt("--list").at("b") == 2, "getStringToInt");
        expect(p.getStringToInt32("--list").at("a") == 1, "getStringToInt32");
        expect(p.getStringToInt64("--list").at("a") == 1, "getStringToInt64");
        expect(p.getStringToUint32("--list").at("a") == 1, "getStringToUint32");
        expect(p.getStringToUint64("--list").at("a") == 1, "getStringToUint64");
        expect(p.getStringToDouble("--list").at("a") == 1.0, "getStringToDouble");
        expect(p.getStringToDuration("--list").at("a").count() == 1000, "getStringToDuration");
    }
    {
        auto p = makeParser({"--list", "a=1s"}, flags);
        expect(p.getStringToDuration("--list").at("a").count() == 1000, "getStringToDuration parses seconds");
    }
    {
        auto p = makeParser({"--list", "on=yes,off=no"}, flags);
        const auto m = p.getStringToBool("--list");
        expect(m.at("on") && !m.at("off"), "getStringToBool");
    }
}

void testArrayHelpers() {
    std::vector<clasp::Flag> flags;
    flags.push_back(clasp::Flag("--list", "", "L", "var", std::string("")));
    flags.push_back(clasp::Flag("--dflt", "", "D", "var", std::string("one")));
    flags.push_back(clasp::Flag("--nums", "", "N", "var", 0));
    flags.push_back(clasp::Flag("--durs", "", "D", "var", std::chrono::milliseconds{0}));
    flags.push_back(clasp::Flag("--gates", "", "G", "var", false));
    flags.push_back(clasp::Flag("--ratio", "", "R", "var", 0.0f));

    {
        auto p = makeParser({"--list", "x", "--list", "y"}, flags);
        const auto arr = p.getStringArray("--list");
        expect(arr.size() == 2 && arr[0] == "x" && arr[1] == "y", "getStringArray keeps CLI occurrences");
    }
    {
        auto p = makeParser({}, flags);
        p.setExternalValuesMulti({{"--list", {"e1", "e2"}}});
        expect(p.getStringArray("--list").size() == 2, "getStringArray uses external multi values");
    }
    {
        auto p = makeParser({}, flags);
        p.setExternalValues({{"--list", "single"}});
        const auto arr = p.getStringArray("--list");
        expect(arr.size() == 1 && arr[0] == "single", "getStringArray uses external single value");
    }
    {
        auto p = makeParser({}, flags);
        expect(p.getStringArray("--list").empty(), "empty default treated as empty array");
    }
    {
        auto p = makeParser({}, flags);
        const auto arr = p.getStringArray("--dflt");
        expect(arr.size() == 1 && arr[0] == "one", "non-empty default becomes single element");
    }
    {
        auto p = makeParser({}, flags);
        expect(p.getStringArray("--absent").empty(), "getStringArray empty for unknown flag");
    }
    {
        auto p = makeParser({"--nums", "1", "--nums", "2"}, flags);
        expect(p.getIntArray("--nums").size() == 2, "getIntArray");
        expect(p.getInt32Array("--nums").size() == 2, "getInt32Array");
        expect(p.getInt64Array("--nums").size() == 2, "getInt64Array");
        expect(p.getUint32Array("--nums").size() == 2, "getUint32Array");
        expect(p.getUint64Array("--nums").size() == 2, "getUint64Array");
        expect(p.getFloatArray("--nums").size() == 2, "getFloatArray");
        expect(p.getDoubleArray("--nums").size() == 2, "getDoubleArray");
        expect(p.getArray<int>("--nums").size() == 2, "getArray<int>");
        expect(p.getArray<std::string>("--list").empty(), "getArray<string>");
    }
    {
        auto p = makeParser({"--gates", "true", "--gates", "false"}, flags);
        const auto arr = p.getBoolArray("--gates");
        expect(arr.size() == 2 && arr[0] && !arr[1], "getBoolArray");
    }
    {
        auto p = makeParser({"--durs", "1s", "--durs", "2s"}, flags);
        const auto arr = p.getDurationArray("--durs");
        expect(arr.size() == 2 && arr[1].count() == 2000, "getDurationArray");
        expect(p.getArray<std::chrono::milliseconds>("--durs").size() == 2, "getArray<milliseconds>");
    }
}

void testSliceHelpers() {
    std::vector<clasp::Flag> flags;
    flags.push_back(clasp::Flag("--nums", "", "N", "var", 0));
    flags.push_back(clasp::Flag("--list", "", "L", "var", std::string("")));
    flags.push_back(clasp::Flag("--durs", "", "D", "var", std::chrono::milliseconds{0}));
    flags.push_back(clasp::Flag("--gates", "", "G", "var", false));
    flags.push_back(clasp::Flag("--ratio", "", "R", "var", 0.0f));
    flags.push_back(clasp::Flag("--big", "", "B", "var", std::int64_t{0}));

    {
        auto p = makeParser({"--nums", "1,2", "--nums", "3"}, flags);
        expect(p.getSlice<int>("--nums").size() == 3, "getSlice<int>");
        expect(p.getIntSlice("--nums").size() == 3, "getIntSlice");
        expect(p.getInt32Slice("--nums").size() == 3, "getInt32Slice");
        expect(p.getInt64Slice("--nums").size() == 3, "getInt64Slice");
        expect(p.getUint32Slice("--nums").size() == 3, "getUint32Slice");
        expect(p.getUint64Slice("--nums").size() == 3, "getUint64Slice");
        expect(p.getFloatSlice("--nums").size() == 3, "getFloatSlice");
        expect(p.getDoubleSlice("--nums").size() == 3, "getDoubleSlice");
        expect(p.getFlagValuesAs<int>("--nums").size() == 2, "getFlagValuesAs");
    }
    {
        auto p = makeParser({"--list", "a,b"}, flags);
        expect(p.getSlice<std::string>("--list").size() == 2, "getSlice<string>");
        expect(p.getStringSlice("--list").size() == 2, "getStringSlice");
    }
    {
        auto p = makeParser({"--durs", "1s,2s"}, flags);
        expect(p.getDurationSlice("--durs").size() == 2, "getDurationSlice");
    }
    {
        auto p = makeParser({"--gates", "true,false"}, flags);
        expect(p.getBoolSlice("--gates").size() == 2, "getBoolSlice");
    }
}

// "5B" uses the bare "B" unit (multiplier 1).
void testBytesBareUnit() {
    std::vector<clasp::Flag> flags;
    flags.push_back(annotatedFlag("--limit", "bytes", std::uint64_t{0}));
    auto p = makeParser({"--limit", "5B"}, flags);
    expect(p.ok(), "bytes value with bare B unit parses");
    expect(p.getFlag<std::uint64_t>("--limit", 0) == 5, "bare B unit multiplier is 1");
}

// Uppercase hex digits must be accepted in IPv6 literals.
void testIPv6UppercaseHex() {
    std::vector<clasp::Flag> flags;
    flags.push_back(annotatedFlag("--ip", "ip", std::string("")));
    auto p = makeParser({"--ip", "2001:DB8::AA"}, flags);
    expect(p.ok(), "uppercase hex IPv6 parses");
    expect(p.getFlag<std::string>("--ip", "") == "2001:db8::aa", "canonical form is lowercase");
}

// Non-numeric ports and multi-colon hosts.
void testUrlEdgeCases() {
    std::vector<clasp::Flag> flags;
    flags.push_back(annotatedFlag("--url", "url", std::string("")));

    {
        auto p = makeParser({"--url", "HTTP://Host:notaport/x"}, flags);
        expect(p.ok(), "url with non-numeric port parses");
        expect(p.getFlag<std::string>("--url", "") == "http://host:notaport/x", "url lowercased with suffix port");
    }
    {
        auto p = makeParser({"--url", "http://a::b/p"}, flags);
        expect(p.ok(), "url with multi-colon host parses");
        expect(p.getFlag<std::string>("--url", "") == "http://a::b/p", "multi-colon host lowercased");
    }
}

// Count flags accept an explicit increment in short groups ("-v3").
void testShortGroupCountExplicit() {
    std::vector<clasp::Flag> flags;
    flags.push_back(annotatedFlag("--verbose", "count", 0));
    auto p = makeParser({"-v3"}, flags);
    expect(p.ok(), "short group with numeric remainder parses");
    expect(p.getCount("--verbose") == 3, "count flag numeric remainder");
}

// Value flags at the end of a short group: noOpt defaults, next-token flag,
// next-token value, and the missing-argument error.
void testShortGroupTailValues() {
    std::vector<clasp::Flag> withNoOpt;
    withNoOpt.push_back(clasp::Flag("--xflag", "-x", "X", "x", false));
    {
        clasp::Flag m("--mflag", "-m", "M", "m", std::string(""));
        m.setNoOptDefaultValue("auto");
        withNoOpt.push_back(m);
    }

    {
        auto p = makeParser({"-xm"}, withNoOpt);
        expect(p.ok(), "group tail with no next token uses noOpt default");
        expect(p.getFlag<std::string>("--mflag", "") == "auto", "noOpt default applied at group tail");
    }
    {
        auto p = makeParser({"-xm", "--other"}, withNoOpt);
        expect(p.getFlag<std::string>("--mflag", "") == "auto", "noOpt default when next token is a flag");
    }
    {
        auto p = makeParser({"-xm", "hi"}, withNoOpt);
        expect(p.ok(), "group tail consumes next token as value");
        expect(p.getFlag<std::string>("--mflag", "") == "hi", "next token used as value at group tail");
    }

    std::vector<clasp::Flag> withoutNoOpt;
    withoutNoOpt.push_back(clasp::Flag("--xflag", "-x", "X", "x", false));
    withoutNoOpt.push_back(clasp::Flag("--mflag", "-m", "M", "m", std::string("")));
    {
        auto p = makeParser({"-xm"}, withoutNoOpt);
        expect(!p.ok(), "missing value at group tail fails");
        expect(p.error().find("flag needs an argument") != std::string::npos, "missing value error message");
    }
}

} // namespace

int main() {
    std::cout << "=== Testing getFlag source precedence ===" << std::endl;
    testGetFlagSources();

    std::cout << "\n=== Testing getFlagValues fallback ===" << std::endl;
    testGetFlagValuesFallback();

    std::cout << "\n=== Testing checked external sources ===" << std::endl;
    testCheckedExternalSources();

    std::cout << "\n=== Testing map helpers ===" << std::endl;
    testMapHelpers();

    std::cout << "\n=== Testing array helpers ===" << std::endl;
    testArrayHelpers();

    std::cout << "\n=== Testing slice helpers ===" << std::endl;
    testSliceHelpers();

    std::cout << "\n=== Testing bytes bare unit ===" << std::endl;
    testBytesBareUnit();

    std::cout << "\n=== Testing IPv6 uppercase hex ===" << std::endl;
    testIPv6UppercaseHex();

    std::cout << "\n=== Testing URL edge cases ===" << std::endl;
    testUrlEdgeCases();

    std::cout << "\n=== Testing short group count explicit ===" << std::endl;
    testShortGroupCountExplicit();

    std::cout << "\n=== Testing short group tail values ===" << std::endl;
    testShortGroupTailValues();

    if (g_failures == 0) {
        std::cout << "\nok\n";
    }
    return g_failures == 0 ? 0 : 1;
}
