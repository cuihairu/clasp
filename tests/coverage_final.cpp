// Final coverage gap tests: exercise the remaining uncovered lines in
// parser.hpp and command.hpp (exception unwind paths, getFlag resolution
// branches, help/version templates, completion scanning and config scalars).
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "clasp/clasp.hpp"

namespace {

int g_failures = 0;

void expect(bool cond, const std::string& label) {
    std::cout << label << ": " << (cond ? "pass" : "fail") << std::endl;
    if (!cond) ++g_failures;
}

clasp::Parser makeParser(const std::vector<std::string>& args, const std::vector<clasp::Flag>& flags) {
    std::vector<char*> ptrs;
    ptrs.push_back(const_cast<char*>("app"));
    for (const auto& a : args) ptrs.push_back(const_cast<char*>(a.c_str()));
    return clasp::Parser(static_cast<int>(ptrs.size()), ptrs.data(), flags);
}

int runArgs(clasp::Command& cmd, const std::vector<std::string>& args) {
    std::vector<char*> ptrs;
    ptrs.push_back(const_cast<char*>(cmd.name().c_str()));
    for (const auto& a : args) ptrs.push_back(const_cast<char*>(a.c_str()));
    return cmd.run(static_cast<int>(ptrs.size()), ptrs.data());
}

void writeTextFile(const std::string& path, const std::string& content) {
    std::ofstream f(path, std::ios::binary);
    f << content;
}

// A normalizeKey hook that throws exercises the exception-cleanup blocks of
// every parser helper that resolves keys (constructor, slices, arrays, maps).
void testThrowingNormalizeKey() {
    std::vector<clasp::Flag> flags;
    flags.push_back(clasp::Flag("--nums", "", "N", "var", 0));

    {
        // A flag token during parsing triggers the hook inside the ctor.
        clasp::Parser::Options opts;
        opts.normalizeKey = [](std::string) -> std::string { throw std::runtime_error("boom"); };
        const char* argv[] = {"app", "--x", "v"};
        bool threw = false;
        try {
            clasp::Parser p(3, const_cast<char**>(argv), flags, opts);
            (void)p;
        } catch (const std::runtime_error&) {
            threw = true;
        }
        expect(threw, "throwing normalizeKey escapes ctor");
    }
    {
        // No flag tokens: the ctor completes, and each helper unwinds through
        // the hook exception.
        clasp::Parser::Options opts;
        opts.normalizeKey = [](std::string) -> std::string { throw std::runtime_error("boom"); };
        const char* argv[] = {"app"};
        clasp::Parser p(1, const_cast<char**>(argv), flags, opts);
        int threwCount = 0;
        auto attempt = [&](auto&& fn) {
            try {
                fn();
            } catch (const std::runtime_error&) {
                ++threwCount;
            }
        };
        attempt([&] { (void)p.getFlagValuesAs<int>("--nums", 0); });
        attempt([&] { (void)p.getFlagValuesSplit("--nums"); });
        attempt([&] { (void)p.getFlagMap("--nums"); });
        attempt([&] { (void)p.getStringTo<std::string>("--nums"); });
        attempt([&] { (void)p.getStringTo<int>("--nums"); });
        attempt([&] { (void)p.getStringTo<std::int32_t>("--nums"); });
        attempt([&] { (void)p.getStringTo<std::int64_t>("--nums"); });
        attempt([&] { (void)p.getStringTo<std::uint32_t>("--nums"); });
        attempt([&] { (void)p.getStringTo<std::uint64_t>("--nums"); });
        attempt([&] { (void)p.getStringTo<float>("--nums"); });
        attempt([&] { (void)p.getStringTo<double>("--nums"); });
        attempt([&] { (void)p.getStringTo<std::chrono::milliseconds>("--nums"); });
        attempt([&] { (void)p.getStringTo<bool>("--nums"); });
        attempt([&] { (void)p.getStringToInt("--nums"); });
        attempt([&] { (void)p.getStringToInt64("--nums"); });
        attempt([&] { (void)p.getStringToUint64("--nums"); });
        attempt([&] { (void)p.getStringToDouble("--nums"); });
        attempt([&] { (void)p.getStringToDuration("--nums"); });
        attempt([&] { (void)p.getStringToBool("--nums"); });
        attempt([&] { (void)p.getBoolArray("--nums"); });
        attempt([&] { (void)p.getIntArray("--nums"); });
        attempt([&] { (void)p.getInt64Array("--nums"); });
        attempt([&] { (void)p.getUint32Array("--nums"); });
        attempt([&] { (void)p.getUint64Array("--nums"); });
        attempt([&] { (void)p.getFloatArray("--nums"); });
        attempt([&] { (void)p.getDoubleArray("--nums"); });
        attempt([&] { (void)p.getDurationArray("--nums"); });
        attempt([&] { (void)p.getBoolSlice("--nums"); });
        attempt([&] { (void)p.getIntSlice("--nums"); });
        attempt([&] { (void)p.getInt64Slice("--nums"); });
        attempt([&] { (void)p.getUint32Slice("--nums"); });
        attempt([&] { (void)p.getUint64Slice("--nums"); });
        attempt([&] { (void)p.getFloatSlice("--nums"); });
        attempt([&] { (void)p.getDoubleSlice("--nums"); });
        attempt([&] { (void)p.getDurationSlice("--nums"); });
        attempt([&] { (void)p.getFlag<int>("--nums", 0); });
        expect(threwCount == 36, "all helpers unwind through throwing hook");
    }
}

// getFlag<T> must visit every resolution branch (CLI, external multi,
// external single, declared default, caller default) for each scalar type.
void testGetFlagBranchMatrix() {
    std::vector<clasp::Flag> flags;
    flags.push_back(clasp::Flag("--b", "", "", "var", false));
    flags.push_back(clasp::Flag("--i", "", "", "var", 3));
    flags.push_back(clasp::Flag("--l", "", "", "var", std::int64_t{4}));
    flags.push_back(clasp::Flag("--j", "", "", "var", std::uint32_t{5}));
    flags.push_back(clasp::Flag("--m", "", "", "var", std::uint64_t{6}));
    flags.push_back(clasp::Flag("--f", "", "", "var", 1.5f));
    flags.push_back(clasp::Flag("--d", "", "", "var", 2.5));
    flags.push_back(clasp::Flag("--ms", "", "", "var", std::chrono::milliseconds{250}));
    flags.push_back(clasp::Flag("--s", "", "", "var", std::string("declared")));

    auto checkBool = [&](const char* label, bool got, bool want) { expect(got == want, label); };

    // External multi fallback then caller default for every type.
    {
        auto p = makeParser({}, flags);
        p.setExternalValuesMulti({{"--b", {"true"}},
                                  {"--i", {"9"}},
                                  {"--l", {"10"}},
                                  {"--j", {"11"}},
                                  {"--m", {"12"}},
                                  {"--f", {"0.5"}},
                                  {"--d", {"0.25"}},
                                  {"--ms", {"1s"}},
                                  {"--s", {"multi"}}});
        checkBool("getFlag<bool> external multi", p.getFlag<bool>("--b", false), true);
        expect(p.getFlag<int>("--i", 0) == 9, "getFlag<int> external multi");
        expect(p.getFlag<std::int64_t>("--l", 0) == 10, "getFlag<int64> external multi");
        expect(p.getFlag<std::uint32_t>("--j", 0) == 11, "getFlag<uint32> external multi");
        expect(p.getFlag<std::uint64_t>("--m", 0) == 12, "getFlag<uint64> external multi");
        expect(p.getFlag<float>("--f", 0.0f) == 0.5f, "getFlag<float> external multi");
        expect(p.getFlag<double>("--d", 0.0) == 0.25, "getFlag<double> external multi");
        expect(p.getFlag<std::chrono::milliseconds>("--ms", std::chrono::milliseconds{0}).count() == 1000,
               "getFlag<milliseconds> external multi");
        expect(p.getFlag<std::string>("--s", "") == "multi", "getFlag<string> external multi");
    }
    // Caller-provided default for unregistered flag names.
    {
        auto p = makeParser({}, flags);
        checkBool("getFlag<bool> caller default", p.getFlag<bool>("--nope", true), true);
        expect(p.getFlag<int>("--nope", 42) == 42, "getFlag<int> caller default");
        expect(p.getFlag<std::int64_t>("--nope", 43) == 43, "getFlag<int64> caller default");
        expect(p.getFlag<std::uint32_t>("--nope", 44) == 44, "getFlag<uint32> caller default");
        expect(p.getFlag<std::uint64_t>("--nope", 45) == 45, "getFlag<uint64> caller default");
        expect(p.getFlag<float>("--nope", 0.75f) == 0.75f, "getFlag<float> caller default");
        expect(p.getFlag<double>("--nope", 0.875) == 0.875, "getFlag<double> caller default");
        expect(p.getFlag<std::chrono::milliseconds>("--nope", std::chrono::milliseconds{7}).count() == 7,
               "getFlag<milliseconds> caller default");
        expect(p.getFlag<std::string>("--nope", "cd") == "cd", "getFlag<string> caller default");
    }
    // Float: also CLI value, external single value and declared default paths.
    {
        auto p = makeParser({"--f", "4.5"}, flags);
        expect(p.getFlag<float>("--f", 0.0f) == 4.5f, "getFlag<float> cli value");
    }
    {
        auto p = makeParser({}, flags);
        p.setExternalValues({{"--f", "5.5"}});
        expect(p.getFlag<float>("--f", 0.0f) == 5.5f, "getFlag<float> external single");
    }
    {
        auto p = makeParser({}, flags);
        expect(p.getFlag<float>("--f", 0.0f) == 1.5f, "getFlag<float> declared default");
    }
}

// getStringTo<V> parses values through every typed instantiation.
void testGetStringToHappyPaths() {
    std::vector<clasp::Flag> flags;
    flags.push_back(clasp::Flag("--list", "", "L", "var", std::string("")));
    {
        auto p = makeParser({"--list", "a=1.5"}, flags);
        expect(p.getStringTo<float>("--list").at("a") == 1.5f, "getStringTo<float> parses value");
    }
    {
        auto p = makeParser({"--list", "a=true"}, flags);
        expect(p.getStringTo<bool>("--list").at("a"), "getStringTo<bool> parses value");
    }
    {
        auto p = makeParser({"--list", "a=1s"}, flags);
        expect(p.getStringTo<std::chrono::milliseconds>("--list").at("a").count() == 1000,
               "getStringTo<milliseconds> parses value");
    }
    {
        auto p = makeParser({"--list", "a=7"}, flags);
        expect(p.getStringTo<std::int64_t>("--list").at("a") == 7, "getStringTo<int64> parses value");
    }
    {
        auto p = makeParser({"--list", "a=8"}, flags);
        expect(p.getStringTo<std::uint64_t>("--list").at("a") == 8, "getStringTo<uint64> parses value");
    }
}

// A non-hex digit inside an IPv6 hextet must be rejected.
void testIPv6InvalidHexDigit() {
    std::vector<clasp::Flag> flags;
    clasp::Flag f("--ip", "", "ip", "var", std::string(""));
    f.setAnnotation("ip", "true");
    flags.push_back(f);
    auto p = makeParser({"--ip", "2001:db8::zz"}, flags);
    expect(!p.ok(), "ipv6 hextet with non-hex digit rejected");
}

// contextAs<T> returns the stored std::any for both const and mutable paths.
void testContextAsInt() {
    std::ostringstream out, err;
    clasp::Command root("app", "ctx");
    root.setOut(out);
    root.setErr(err);
    root.setContext(42);
    auto* mutVal = root.contextAs<int>();
    expect(mutVal != nullptr && *mutVal == 42, "contextAs<int> mutable");
    const clasp::Command& croot = root;
    const auto* constVal = croot.contextAs<int>();
    expect(constVal != nullptr && *constVal == 42, "contextAs<int> const");
}

// The Error color role paints the "Error:" prefix when color is forced on.
void testColorErrorRole() {
    std::ostringstream out, err;
    clasp::Command root("app", "err color");
    root.setOut(out);
    root.setErr(err);
    root.enableColor(clasp::ColorMode::Always);
    const int rc = runArgs(root, {"nosuchcmd"});
    expect(rc == 1, "unknown command with color always exits 1");
    expect(err.str().find("\033[") != std::string::npos, "error prefix painted with color");
}

// Help output renders numeric/duration defaults and quoted string defaults.
void testHelpDefaultsAndTypes() {
    std::ostringstream out, err;
    clasp::Command root("app", "help defaults");
    root.setOut(out);
    root.setErr(err);
    root.withFlag("--num", "", "n", "N", 7);
    root.withFlag("--big", "", "b", "B", std::int64_t{9});
    root.withFlag("--cnt", "", "c", "C", std::int32_t{5});
    root.withFlag("--rate", "", "r", "R", 0.5f);
    root.withFlag("--precise", "", "p", "P", 0.25);
    root.withFlag("--port", "", "o", "O", std::uint32_t{80});
    root.withFlag("--lim", "", "l", "L", std::uint64_t{99});
    root.withFlag("--ttl", "", "t", "T", std::chrono::milliseconds{250});
    root.withFlag("--name", "", "s", "S", std::string("he\"llo"));
    root.withFlag("--flag", "-f", "f", "F", true);
    const int rc = runArgs(root, {"--help"});
    expect(rc == 0, "help with typed defaults prints");
    const std::string help = out.str();
    expect(help.find("(default: 7)") != std::string::npos, "int default rendered");
    expect(help.find("(default: 250ms)") != std::string::npos, "duration default rendered");
    expect(help.find("he\\\"llo") != std::string::npos, "string default escaped");
}

// Usage and version templates exercise every placeholder corner of
// renderTemplate: plain text (no "{{.") and an unclosed "{{.".
void testTemplateRenderCorners() {
    {
        std::ostringstream out, err;
        clasp::Command root("app", "tpl");
        root.setOut(out);
        root.setErr(err);
        root.setUsageTemplate("plain-usage-line");
        runArgs(root, {"--help"});
        expect(out.str().find("plain-usage-line") != std::string::npos, "usage template without placeholder");
    }
    {
        std::ostringstream out, err;
        clasp::Command root("app", "tpl");
        root.setOut(out);
        root.setErr(err);
        root.setUsageTemplate("u{{.unclosed");
        runArgs(root, {"--help"});
        expect(out.str().find("{{.unclosed") != std::string::npos, "usage template unclosed placeholder");
    }
    {
        std::ostringstream out, err;
        clasp::Command root("app", "tpl");
        root.setOut(out);
        root.setErr(err);
        root.version("1.2.3");
        root.setVersionTemplate("ver{{.Oops");
        runArgs(root, {"--version"});
        expect(out.str().find("ver{{.Oops") != std::string::npos, "version template unclosed placeholder");
    }
}

// Completion scanning skips a noOpt short-group tail when the next word is a
// subcommand name.
void testCompletionNoOptBeforeSubcommand() {
    std::ostringstream out, err;
    clasp::Command root("app", "cmpl");
    root.setOut(out);
    root.setErr(err);
    root.enableCompletion();
    root.withFlag("--xflag", "-x", "x", "X", true);
    root.withFlag("--message", "-m", "m", "M", std::string(""));
    root.markFlagNoOptDefaultValue("--message", "auto");
    clasp::Command sub("p", "P cmd");
    root.addCommand(std::move(sub));
    const int rc = runArgs(root, {"__completeNoDesc", "-xm", "p", ""});
    expect(rc == 0, "completion noOpt short group before subcommand");
}

// JSON config files accept true/false/null scalar literals.
void testJsonScalarLiterals() {
    std::ostringstream out, err;
    clasp::Command root("app", "json literals");
    root.setOut(out);
    root.setErr(err);
    root.withFlag("--config", "", "cfg", "C", std::string(""));
    root.withFlag("--sv", "", "sv", "S", std::string(""));
    root.configFileFlag("--config");
    writeTextFile("final_tfn.json", "{\"sv\": true, \"other\": false, \"third\": null}");
    const int rc = runArgs(root, {"--config", "final_tfn.json"});
    std::remove("final_tfn.json");
    expect(rc == 0, "json true/false/null config accepted");
}

// TOML double-quoted values pass unknown escape characters through verbatim.
void testTomlUnknownEscape() {
    std::ostringstream out, err;
    clasp::Command root("app", "toml escape");
    root.setOut(out);
    root.setErr(err);
    root.withFlag("--config", "", "cfg", "C", std::string(""));
    root.withFlag("--sv", "", "sv", "S", std::string(""));
    root.configFileFlag("--config");
    writeTextFile("final_tq.toml", "sv = \"a\\qb\"\n");
    const int rc = runArgs(root, {"--config", "final_tq.toml"});
    std::remove("final_tq.toml");
    expect(rc == 0, "toml unknown escape accepted");
}

} // namespace

int main() {
    testThrowingNormalizeKey();
    testGetFlagBranchMatrix();
    testGetStringToHappyPaths();
    testIPv6InvalidHexDigit();
    testContextAsInt();
    testColorErrorRole();
    testHelpDefaultsAndTypes();
    testTemplateRenderCorners();
    testCompletionNoOptBeforeSubcommand();
    testJsonScalarLiterals();
    testTomlUnknownEscape();

    std::cout << (g_failures == 0 ? "ALL OK\n" : "FAILURES\n");
    return g_failures == 0 ? 0 : 1;
}
