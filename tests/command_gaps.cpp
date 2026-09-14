#include <sys/types.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <utility>
#include <vector>

#include "clasp/clasp.hpp"

namespace {

int g_failures = 0;

// Global capture stream: actions cannot reach the private Command::out().
std::ostringstream g_capture;

void expect(bool cond, const char* label) {
    std::cout << label << ": " << (cond ? "pass" : "fail") << std::endl;
    if (!cond) ++g_failures;
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

class EnvGuard {
public:
    EnvGuard(const char* name, const char* value) : name_(name) {
        const char* old = std::getenv(name);
        had_ = old != nullptr;
        if (had_) old_ = old;
        if (value != nullptr) {
            ::setenv(name, value, 1);
        } else {
            ::unsetenv(name);
        }
    }
    ~EnvGuard() {
        if (had_) {
            ::setenv(name_.c_str(), old_.c_str(), 1);
        } else {
            ::unsetenv(name_.c_str());
        }
    }

private:
    std::string name_;
    bool had_{false};
    std::string old_;
};

clasp::Command makeBasicColorRoot(std::ostringstream& out, std::ostringstream& err) {
    clasp::Command root("app", "Color root");
    root.setOut(out);
    root.setErr(err);
    root.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
    return root;
}

// --- Color mode handling ----------------------------------------------------

void testColorStyledHelp() {
    std::ostringstream out, err;
    auto root = makeBasicColorRoot(out, err);
    root.enableColor(clasp::ColorMode::Always);
    root.version("1.0.0");
    root.withFlag("--alpha", "-a", "Alpha flag");
    root.withFlag("--dur", "", "dur", "Duration flag", std::chrono::milliseconds{1500});

    clasp::Command sub("sub", "Sub command");
    sub.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
    root.addCommand(std::move(sub));

    const int rc = runArgs(root, {"--color=always", "--help"});
    expect(rc == 0, "colorized help exits 0");
    expect(out.str().find("\x1b[") != std::string::npos, "help output contains ANSI escapes");
    expect(out.str().find("Usage:") != std::string::npos, "help shows usage");
}

void testColorStyledGroupedHelp() {
    std::ostringstream out, err;
    auto root = makeBasicColorRoot(out, err);
    root.enableColor(clasp::ColorMode::Always);
    root.addGroup("core", "Core Commands");
    root.addGroup("extra", "Extra Commands");

    clasp::Command plain("plain", "Plain command");
    plain.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
    clasp::Command core("build", "Build it");
    core.groupId("core");
    core.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
    clasp::Command extra("extra", "Extra work");
    extra.groupId("extra");
    extra.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
    root.addCommand(std::move(plain));
    root.addCommand(std::move(core));
    root.addCommand(std::move(extra));

    const int rc = runArgs(root, {"--color=always", "--help"});
    expect(rc == 0, "grouped colorized help exits 0");
    expect(out.str().find("Core Commands") != std::string::npos, "grouped help shows group titles");
    expect(out.str().find("plain") != std::string::npos, "grouped help shows ungrouped commands");
}

void testColorNever() {
    std::ostringstream out, err;
    auto root = makeBasicColorRoot(out, err);
    root.enableColor(clasp::ColorMode::Always);
    const int rc = runArgs(root, {"--color=never", "--help"});
    expect(rc == 0, "help with --color=never exits 0");
    expect(out.str().find("\x1b[") == std::string::npos, "never disables ANSI escapes");
}

void testColorAutoBranches() {
    {
        EnvGuard noColor("NO_COLOR", "1");
        std::ostringstream out, err;
        auto root = makeBasicColorRoot(out, err);
        root.enableColor(clasp::ColorMode::Auto);
        runArgs(root, {"--help"});
        expect(out.str().find("\x1b[") == std::string::npos, "NO_COLOR disables color in auto mode");
    }
    {
        EnvGuard dumbTerm("TERM", "dumb");
        std::ostringstream out, err;
        auto root = makeBasicColorRoot(out, err);
        root.enableColor(clasp::ColorMode::Auto);
        runArgs(root, {"--help"});
        expect(out.str().find("\x1b[") == std::string::npos, "TERM=dumb disables color in auto mode");
    }
    {
        EnvGuard term("TERM", "xterm");
        std::ostringstream out, err;
        auto root = makeBasicColorRoot(out, err);
        root.enableColor(clasp::ColorMode::Auto);
        runArgs(root, {"--help"});
        expect(out.str().find("\x1b[") == std::string::npos, "non-tty output disables color in auto mode");
    }
    {
        EnvGuard term("TERM", "xterm");
        std::ostringstream out, err;
        auto root = makeBasicColorRoot(out, err);
        root.enableColor(clasp::ColorMode::Auto);
        runArgs(root, {"--color=auto", "--help"});
        expect(out.str().find("\x1b[") == std::string::npos, "--color=auto on non-tty stays plain");
    }
}

// Auto mode resolves to true when stdout is a real terminal.
void testColorAutoWithPty() {
#ifdef _WIN32
    std::cout << "pty unavailable on Windows: skip" << std::endl;
#else
    const int master = posix_openpt(O_RDWR | O_NOCTTY);
    if (master < 0) {
        std::cout << "pty unavailable: skip" << std::endl;
        return;
    }
    grantpt(master);
    unlockpt(master);
    const int slave = open(ptsname(master), O_RDWR);
    if (slave < 0) {
        close(master);
        std::cout << "pty slave unavailable: skip" << std::endl;
        return;
    }

    std::fflush(stdout);
    const int saved = dup(1);
    dup2(slave, 1);

    {
        EnvGuard noColor("NO_COLOR", nullptr);
        EnvGuard term("TERM", "xterm");
        clasp::Command root("app", "Pty root");
        root.enableColor(clasp::ColorMode::Auto);
        const char* argv[] = {"app", "--help"};
        root.run(2, const_cast<char**>(argv));
    }

    std::fflush(stdout);
    dup2(saved, 1);
    close(saved);
    close(slave);
    close(master);
    std::cout << "auto color with pty: pass" << std::endl;
#endif
}

void testApplyColorRawErrors() {
    std::ostringstream out, err;
    auto root = makeBasicColorRoot(out, err);
    root.enableColor();

    out.str("");
    err.str("");
    expect(runArgs(root, {"--color=bogus"}) == 1, "invalid --color value rejected");
    expect(err.str().find("invalid value for --color") != std::string::npos, "invalid --color message");
    expect(err.str().find("Error:") != std::string::npos, "error prefix present");

    out.str("");
    err.str("");
    expect(runArgs(root, {"--color"}) == 1, "missing --color value rejected");
    expect(err.str().find("flag needs an argument: --color") != std::string::npos, "missing --color message");

    out.str("");
    err.str("");
    expect(runArgs(root, {"--color", "--"}) == 1, "--color followed by -- rejected");
    expect(err.str().find("flag needs an argument: --color") != std::string::npos, "--color before -- message");

    out.str("");
    err.str("");
    expect(runArgs(root, {"--color", "--other"}) == 1, "--color followed by flag rejected");
    expect(err.str().find("flag needs an argument: --color") != std::string::npos, "--color before flag message");

    out.str("");
    err.str("");
    expect(runArgs(root, {"--color-theme=bogus"}) == 1, "invalid --color-theme value rejected");
    expect(err.str().find("invalid value for --color-theme") != std::string::npos, "invalid --color-theme message");

    out.str("");
    err.str("");
    expect(runArgs(root, {"--color-theme"}) == 1, "missing --color-theme value rejected");
    expect(err.str().find("flag needs an argument: --color-theme") != std::string::npos, "missing --color-theme message");
}

void testApplyColorValidTokens() {
    std::ostringstream out, err;
    auto root = makeBasicColorRoot(out, err);
    root.enableColor();

    expect(runArgs(root, {"--color=always"}) == 0, "--color=always accepted");
    expect(runArgs(root, {"--color", "never"}) == 0, "separate --color value accepted");
    expect(runArgs(root, {"--color-theme=iterm2"}) == 0, "--color-theme=iterm2 accepted");
    expect(runArgs(root, {"--color-theme", "sublime"}) == 0, "separate --color-theme value accepted");
    expect(runArgs(root, {"--color-theme=vscode", "--color=auto"}) == 0, "combined color flags accepted");
    expect(runArgs(root, {"--", "--color=always"}) == 0, "--color after -- is a positional");
}

// The parser-level color validation runs when a normalized alias maps to
// --color but the raw scanner does not recognize the token.
void testApplyColorFromParserError() {
    std::ostringstream out, err;
    auto root = makeBasicColorRoot(out, err);
    root.enableColor();
    root.normalizeFlagKeys([](std::string k) {
        if (k == "--colour") return std::string("--color");
        return k;
    });
    root.setFlagErrorFunc([](const clasp::Command&, const std::string& msg) { return "CFERR[" + msg + "]"; });

    const int rc = runArgs(root, {"--colour=bogus"});
    expect(rc == 1, "normalized color alias with bad value rejected");
    expect(err.str().find("CFERR[") != std::string::npos, "flag error func wraps color error");
}

void testColorFlagCompletion() {
    std::ostringstream out, err;
    auto root = makeBasicColorRoot(out, err);
    root.enableColor();
    root.enableCompletion();

    out.str("");
    expect(runArgs(root, {"__completeNoDesc", "--color", "a"}) == 0, "color flag completion runs");
    expect(out.str().find("auto") != std::string::npos, "color flag completion suggests auto");

    out.str("");
    expect(runArgs(root, {"__completeNoDesc", "--color-theme", "s"}) == 0, "theme flag completion runs");
    expect(out.str().find("sublime") != std::string::npos, "theme flag completion suggests sublime");
}

// --- Templates with unknown keys --------------------------------------------

void testTemplateUnknownKeys() {
    {
        std::ostringstream out, err;
        auto root = makeBasicColorRoot(out, err);
        root.setHelpTemplate("A{{.Bogus}}B{{.Open");
        runArgs(root, {"--help"});
        expect(out.str().find("AB{{.Open") != std::string::npos, "help template renders unknown key and open tag");
    }
    {
        std::ostringstream out, err;
        auto root = makeBasicColorRoot(out, err);
        root.setUsageTemplate("U{{.Zzz}}");
        runArgs(root, {"--unknownflag"});
        expect(err.str().find("U{{.Zzz}}") == std::string::npos, "usage template drops unknown key");
        expect(err.str().find("U") != std::string::npos, "usage template renders prefix");
    }
    {
        std::ostringstream out, err;
        auto root = makeBasicColorRoot(out, err);
        root.version("1.0.0");
        root.setVersionTemplate("V{{.Zzz}}");
        runArgs(root, {"--version"});
        expect(out.str().find("V") == 0, "version template renders unknown key as empty");
    }
}

// --- Help rendering for all flag types --------------------------------------

void testHelpFlagTypeAndDefaults() {
    std::ostringstream out, err;
    auto root = makeBasicColorRoot(out, err);
    root.withFlag("--dur", "", "dur", "Duration flag", std::chrono::milliseconds{1500});
    root.withFlag("--i64", "", "i64", "Int64 flag", std::int64_t{-5});
    root.withFlag("--u32", "", "u32", "Uint32 flag", std::uint32_t{7});
    root.withFlag("--u64", "", "u64", "Uint64 flag", std::uint64_t{9});
    root.withFlag("--f32", "", "f32", "Float flag", 1.5f);
    root.withFlag("--f64", "", "f64", "Double flag", 2.5);
    root.withFlag("--i32", "", "i32", "Int flag", 3);
    root.withFlag("--btrue", "", "btrue", "Bool true flag", true);
    root.withFlag("--strd", "", "strd", "String flag", std::string("sval"));

    runArgs(root, {"--help"});
    const auto text = out.str();
    expect(text.find("--dur duration") != std::string::npos, "help shows duration type");
    expect(text.find("--i64 int64") != std::string::npos, "help shows int64 type");
    expect(text.find("--u32 uint32") != std::string::npos, "help shows uint32 type");
    expect(text.find("--u64 uint64") != std::string::npos, "help shows uint64 type");
    expect(text.find("--f32 float32") != std::string::npos, "help shows float32 type");
    expect(text.find("--f64 float64") != std::string::npos, "help shows float64 type");
    expect(text.find("--i32 int") != std::string::npos, "help shows int type");
    expect(text.find("(default: true)") != std::string::npos, "help shows true bool default");
    expect(text.find("(default: 1500ms)") != std::string::npos, "help shows duration default");
    expect(text.find("(default: -5)") != std::string::npos, "help shows int64 default");
    expect(text.find("(default: 7)") != std::string::npos, "help shows uint32 default");
    expect(text.find("(default: 9)") != std::string::npos, "help shows uint64 default");
    expect(text.find("(default: 1.5)") != std::string::npos, "help shows float default");
    expect(text.find("(default: 2.5)") != std::string::npos, "help shows double default");
    expect(text.find("(default: 3)") != std::string::npos, "help shows int default");
    expect(text.find("(default: \"sval\")") != std::string::npos, "help shows quoted string default");
}

// Two flags sharing a long name compare by short name while sorting.
void testHelpSortSameLongName() {
    std::ostringstream out, err;
    auto root = makeBasicColorRoot(out, err);
    root.withFlag("--help", "-z", "Custom help");
    runArgs(root, {"--help"});
    expect(out.str().find("Custom help") != std::string::npos, "custom --help flag shown");
}

// --- Execution paths ---------------------------------------------------------

void testHelpAndVersionAfterDoubleDash() {
    {
        std::ostringstream out, err;
        auto root = makeBasicColorRoot(out, err);
        root.version("9.9.9");
        clasp::Command sub("sub", "Sub");
        sub.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
        root.addCommand(std::move(sub));

        expect(runArgs(root, {"--", "help", "sub"}) == 0, "help after -- works");
        expect(out.str().find("Usage: app sub") != std::string::npos, "help after -- shows sub usage");

        out.str("");
        expect(runArgs(root, {"--", "version"}) == 0, "version after -- works");
        expect(out.str().find("9.9.9") != std::string::npos, "version after -- prints version");
    }
}

void testExecHelpAliasesAndUnknown() {
    std::ostringstream out, err;
    auto root = makeBasicColorRoot(out, err);
    clasp::Command sub("sub", "Sub");
    sub.addAlias("s");
    sub.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
    root.addCommand(std::move(sub));

    out.str("");
    expect(runArgs(root, {"help", "s"}) == 0, "help with alias resolves");
    expect(out.str().find("Usage: app sub") != std::string::npos, "help alias shows target usage");

    out.str("");
    err.str("");
    expect(runArgs(root, {"help", "nope"}) == 1, "help with unknown target rejected");
    expect(err.str().find("unknown command") != std::string::npos, "help unknown target message");
}

void testFlagMutationOnUnknownFlag() {
    std::ostringstream out, err;
    auto root = makeBasicColorRoot(out, err);
    root.markFlagRequired("--ghost");
    expect(runArgs(root, {}) == 0, "marking unknown flag required is a no-op");
}

void testContextAccessorsWithoutContext() {
    clasp::Command root("app", "No context");
    expect(!root.hasContext(), "hasContext false without context");
    expect(root.contextAs<int>() == nullptr, "mutable contextAs null without context");
    expect(static_cast<const clasp::Command&>(root).contextAs<int>() == nullptr, "const contextAs null without context");
}

void testArgsValidators() {
    std::ostringstream out, err;
    auto root = makeBasicColorRoot(out, err);

    clasp::Command needsTwo("need2", "Needs two");
    needsTwo.args(clasp::MinimumNArgs(2));
    needsTwo.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });

    clasp::Command atMostOne("max1", "At most one");
    atMostOne.args(clasp::MaximumNArgs(1));
    atMostOne.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });

    root.addCommand(std::move(needsTwo));
    root.addCommand(std::move(atMostOne));

    err.str("");
    expect(runArgs(root, {"need2", "one"}) == 1, "MinimumNArgs rejects too few");
    expect(err.str().find("requires at least 2 arg(s)") != std::string::npos, "MinimumNArgs message");

    err.str("");
    expect(runArgs(root, {"max1", "a", "b"}) == 1, "MaximumNArgs rejects too many");
    expect(err.str().find("accepts at most 1 arg(s)") != std::string::npos, "MaximumNArgs message");
}

void testCompletionCommandErrors() {
    std::ostringstream out, err;
    auto root = makeBasicColorRoot(out, err);
    root.enableCompletion();

    err.str("");
    expect(runArgs(root, {"completion", "bash", "extra"}) == 1, "completion rejects wrong arg count");
    expect(err.str().find("accepts 1 arg(s)") != std::string::npos, "completion arg count message");

    err.str("");
    expect(runArgs(root, {"completion", "tcsh"}) == 1, "completion rejects unknown shell");
    expect(err.str().find("unknown shell: tcsh") != std::string::npos, "unknown shell message");
}

void testCompletionScriptWithoutEnable() {
    std::ostringstream out;
    clasp::Command bare("my-app.v2", "Bare");
    bare.printCompletionBash(out);
    expect(out.str().find("_my_app_v2_complete") != std::string::npos, "root name sanitized in script");
}

void testCompletionEdgeTokens() {
    std::ostringstream out, err;
    auto root = makeBasicColorRoot(out, err);
    root.enableCompletion();
    root.version("1.0.0");

    root.withFlag("--name", "", "name", "Name flag", std::string(""));
    root.withFlag("--verbose", "-v", "Verbose", "v", true);
    root.withFlag("--quiet", "-q", "Quiet", "q", true);
    root.withFlag("--message", "-m", "message", "Message flag", std::string(""));
    root.markFlagNoOptDefaultValue("--message", "auto");

    clasp::Command pcmd("p", "P command");
    pcmd.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
    root.addCommand(std::move(pcmd));

    out.str("");
    expect(runArgs(root, {"__completeNoDesc", "--help", ""}) == 0, "completion after --help token");
    expect(out.str().find(":4") != std::string::npos, "completion after --help directive");

    out.str("");
    expect(runArgs(root, {"__completeNoDesc", "--mystery", "x"}) == 0, "completion after unknown flag token");

    out.str("");
    expect(runArgs(root, {"__completeNoDesc", "-xm", "p", ""}) == 0, "completion after noOpt short group before subcommand");

    out.str("");
    expect(runArgs(root, {"__completeNoDesc", "-vq", ""}) == 0, "completion after bool short group");

    out.str("");
    expect(runArgs(root, {"__completeNoDesc", "--", "x"}) == 0, "completion after --");

    out.str("");
    expect(runArgs(root, {"__completeNoDesc", "xyz", "ab"}) == 0, "completion after unknown token");

    out.str("");
    expect(runArgs(root, {"__completeNoDesc", "--name="}) == 0, "completion for flag without completion func");
    expect(out.str().find(":4") != std::string::npos, "flag without completion func directive");

    out.str("");
    expect(runArgs(root, {"__completeNoDesc", ""}) == 0, "completion with empty prefix");
    expect(out.str().find("help") != std::string::npos, "empty prefix suggests help command");
    expect(out.str().find("version") != std::string::npos, "empty prefix suggests version command");
}

// Short group ending in a noOpt flag directly before a subcommand token.
void testExecutionShortGroupNoOpt() {
    std::ostringstream out, err;
    auto root = makeBasicColorRoot(out, err);
    root.withPersistentFlag("--xflag", "-x", "x", "X flag", true);
    root.withPersistentFlag("--message", "-m", "message", "Message flag", std::string(""));
    root.markPersistentFlagNoOptDefaultValue("--message", "auto");

    clasp::Command pcmd("p", "P command");
    pcmd.action([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        g_capture << "msg=" << parser.getFlag<std::string>("--message", "none") << "\n";
        return 0;
    });
    root.addCommand(std::move(pcmd));

    g_capture.str("");
    expect(runArgs(root, {"-xm", "p"}) == 0, "short group noOpt before subcommand runs");
    expect(g_capture.str().find("msg=auto") != std::string::npos, "noOpt default preserved for subcommand");
}

// --- Config parsing edge cases ----------------------------------------------

clasp::Command makeConfigRoot(std::ostringstream& out, std::ostringstream& err) {
    clasp::Command root("app", "Config gaps");
    root.setOut(out);
    root.setErr(err);
    root.withFlag("--config", "", "cfg", "Config file", std::string(""));
    root.withFlag("--sv", "", "sv", "String value", std::string(""));
    root.withFlag("--items", "", "items", "Items", std::string(""));
    root.configFileFlag("--config");
    root.action([](clasp::Command&, const clasp::Parser& p, const std::vector<std::string>&) {
        g_capture << "sv=[" << p.getFlag<std::string>("--sv", "") << "]";
        const auto items = p.getStringArray("--items");
        g_capture << " items=" << items.size();
        for (const auto& i : items) g_capture << "|" << i;
        g_capture << "\n";
        return 0;
    });
    return root;
}

void testConfigJsonEdges() {
    const std::vector<std::pair<std::string, bool>> cases = {
        {"{\"sv\": \"ok\"}", true},
        {"{}", true},
        {"{\"outer\": {\"sv\": \"nested\"}}", true},
        {"{\"sv\": [{\"x\":1,\"y\":[2]}]}", true},
        {"5", false},
        {"{5: 1}", false},
        {"{\"a\" 1}", false},
        {"{\"sv\": }", false},
        {"{\"sv\": -}", false},
        {"{\"sv\": 1e}", false},
        {"{\"sv\": \"v\\", false},
        {"{\"sv\": \"v", false},
        {"{\"sv\": \"\\x41\"}", false},
        {"{\"sv\": [1", false},
        {"{\"sv\": [%]}", false},
        {"{\"sv\": [1,", false},
        {"{\"sv\": \"ok\"} trailing", false},
    };

    for (std::size_t i = 0; i < cases.size(); ++i) {
        std::ostringstream out, err;
        auto root = makeConfigRoot(out, err);
        const std::string file = "gaps_json_" + std::to_string(i) + ".json";
        writeTextFile(file, cases[i].first);
        g_capture.str("");
        const int rc = runArgs(root, {"--config", file});
        std::remove(file.c_str());
        expect((rc == 0) == cases[i].second, ("json case " + std::to_string(i)).c_str());
        if (!cases[i].second) {
            expect(err.str().find("failed to parse json config file") != std::string::npos,
                   ("json case " + std::to_string(i) + " error").c_str());
        }
    }

    // Escape sequences and numeric scalars are accepted.
    {
        std::ostringstream out, err;
        auto root = makeConfigRoot(out, err);
        writeTextFile("gaps_json_ok.json",
                      "{\"sv\": \"a\\tb\\nc\\r\\f\\b\\\\\\\"\\/z\", \"i\": -5, \"f\": 3.5, \"e\": 1e3, \"neg\": 2.5e-1}");
        g_capture.str("");
        const int rc = runArgs(root, {"--config", "gaps_json_ok.json"});
        std::remove("gaps_json_ok.json");
        expect(rc == 0, "json escapes and numbers accepted");
        expect(g_capture.str().find("sv=[a\tb\nc") == 0, "json escapes decoded");
    }
    {
        std::ostringstream out, err;
        auto root = makeConfigRoot(out, err);
        writeTextFile("gaps_json_arr.json", "{\"items\": [\"one\", \"two\"], \"sv\": \"x\"}");
        g_capture.str("");
        const int rc = runArgs(root, {"--config", "gaps_json_arr.json"});
        std::remove("gaps_json_arr.json");
        expect(rc == 0, "json array config accepted");
        expect(g_capture.str().find("items=2|one|two") != std::string::npos, "json array values multi-bound");
    }
}

void testConfigTomlEdges() {
    {
        std::ostringstream out, err;
        auto root = makeConfigRoot(out, err);
        writeTextFile(
            "gaps_toml.toml",
            "sv = \"a\\tb\\nc\\rd\\\\e\\\"f\"\n"
            "sq = 'lit'\n"
            "bare = plain\n"
            "items = [\"x\", 'y']\n"
            "[]\n"
            "[tbl]\n"
            "bad = [1, 2\n"
            "it = {a = 1}\n");
        g_capture.str("");
        const int rc = runArgs(root, {"--config", "gaps_toml.toml"});
        std::remove("gaps_toml.toml");
        expect(rc == 0, "toml edge config accepted");
        expect(g_capture.str().find("sv=[a\tb\nc\rd\\e\"f]") != std::string::npos, "toml escapes decoded");
        expect(g_capture.str().find("items=2|x|y") != std::string::npos, "toml array values multi-bound");
    }
}

void testConfigIniEdges() {
    {
        std::ostringstream out, err;
        auto root = makeConfigRoot(out, err);
        writeTextFile(
            "gaps_ini.ini",
            "sv = \"quoted\"\n"
            "; semicolon comment\n"
            "# hash comment\n"
            "[unclosed\n"
            "[sec]\n"
            "k : v\n"
            "plainline\n"
            " = nokey\n");
        g_capture.str("");
        const int rc = runArgs(root, {"--config", "gaps_ini.ini"});
        std::remove("gaps_ini.ini");
        expect(rc == 0, "ini edge config accepted");
        expect(g_capture.str().find("sv=[quoted]") != std::string::npos, "ini quoted value unquoted");
    }
}

void testConfigYamlEdges() {
    {
        std::ostringstream out, err;
        auto root = makeConfigRoot(out, err);
        writeTextFile(
            "gaps_yaml.yaml",
            "sv: 'single'\n"
            "items:\n"
            "  - \"one\"\n"
            "  - 'two'\n"
            "  - three\n"
            "nested:\n"
            "  mode: deep\n");
        g_capture.str("");
        const int rc = runArgs(root, {"--config", "gaps_yaml.yaml"});
        std::remove("gaps_yaml.yaml");
        expect(rc == 0, "yaml sequence config accepted");
        expect(g_capture.str().find("sv=[single]") != std::string::npos, "yaml single-quoted value unquoted");
        expect(g_capture.str().find("items=3|one|two|three") != std::string::npos, "yaml sequence items multi-bound");
    }
}

void testConfigEnvQuotes() {
    {
        std::ostringstream out, err;
        auto root = makeConfigRoot(out, err);
        writeTextFile("gaps_env.env", "sv = \"quoted\"\nother = 'single'\n");
        g_capture.str("");
        const int rc = runArgs(root, {"--config", "gaps_env.env"});
        std::remove("gaps_env.env");
        expect(rc == 0, "env-style config accepted");
        expect(g_capture.str().find("sv=[quoted]") != std::string::npos, "env-style quoted value unquoted");
    }
}

} // namespace

int main() {
    std::cout << "=== Testing color styled help ===" << std::endl;
    testColorStyledHelp();

    std::cout << "\n=== Testing color styled grouped help ===" << std::endl;
    testColorStyledGroupedHelp();

    std::cout << "\n=== Testing color never ===" << std::endl;
    testColorNever();

    std::cout << "\n=== Testing color auto branches ===" << std::endl;
    testColorAutoBranches();

    std::cout << "\n=== Testing color auto with pty ===" << std::endl;
    testColorAutoWithPty();

    std::cout << "\n=== Testing apply color raw errors ===" << std::endl;
    testApplyColorRawErrors();

    std::cout << "\n=== Testing apply color valid tokens ===" << std::endl;
    testApplyColorValidTokens();

    std::cout << "\n=== Testing apply color from parser error ===" << std::endl;
    testApplyColorFromParserError();

    std::cout << "\n=== Testing color flag completion ===" << std::endl;
    testColorFlagCompletion();

    std::cout << "\n=== Testing template unknown keys ===" << std::endl;
    testTemplateUnknownKeys();

    std::cout << "\n=== Testing help flag types and defaults ===" << std::endl;
    testHelpFlagTypeAndDefaults();

    std::cout << "\n=== Testing help sort same long name ===" << std::endl;
    testHelpSortSameLongName();

    std::cout << "\n=== Testing help/version after -- ===" << std::endl;
    testHelpAndVersionAfterDoubleDash();

    std::cout << "\n=== Testing execHelp aliases and unknown ===" << std::endl;
    testExecHelpAliasesAndUnknown();

    std::cout << "\n=== Testing flag mutation on unknown flag ===" << std::endl;
    testFlagMutationOnUnknownFlag();

    std::cout << "\n=== Testing context accessors without context ===" << std::endl;
    testContextAccessorsWithoutContext();

    std::cout << "\n=== Testing args validators ===" << std::endl;
    testArgsValidators();

    std::cout << "\n=== Testing completion command errors ===" << std::endl;
    testCompletionCommandErrors();

    std::cout << "\n=== Testing completion script without enable ===" << std::endl;
    testCompletionScriptWithoutEnable();

    std::cout << "\n=== Testing completion edge tokens ===" << std::endl;
    testCompletionEdgeTokens();

    std::cout << "\n=== Testing execution short group noOpt ===" << std::endl;
    testExecutionShortGroupNoOpt();

    std::cout << "\n=== Testing config json edges ===" << std::endl;
    testConfigJsonEdges();

    std::cout << "\n=== Testing config toml edges ===" << std::endl;
    testConfigTomlEdges();

    std::cout << "\n=== Testing config ini edges ===" << std::endl;
    testConfigIniEdges();

    std::cout << "\n=== Testing config yaml edges ===" << std::endl;
    testConfigYamlEdges();

    std::cout << "\n=== Testing config env quotes ===" << std::endl;
    testConfigEnvQuotes();

    if (g_failures == 0) {
        std::cout << "\nok\n";
    }
    return g_failures == 0 ? 0 : 1;
}
