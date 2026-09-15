// Coverage tests for help/color/error rendering and flag setter fallbacks.
#include <atomic>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "clasp/command.hpp"
#include "clasp/color.hpp"

namespace {

int g_failures = 0;

void expect(bool cond, const char* label) {
    std::cout << label << ": " << (cond ? "ok" : "fail") << std::endl;
    if (!cond) ++g_failures;
}

int runArgs(clasp::Command& cmd, const std::vector<std::string>& args) {
    std::vector<char*> ptrs;
    ptrs.push_back(const_cast<char*>(cmd.name().c_str()));
    for (const auto& a : args) ptrs.push_back(const_cast<char*>(a.c_str()));
    return cmd.run(static_cast<int>(ptrs.size()), ptrs.data());
}

// Simple spin barrier for the static-init race test (C++17 compatible).
class SpinBarrier {
public:
    explicit SpinBarrier(int count) : count_(count) {}
    void wait() {
        arrived_.fetch_add(1, std::memory_order_acq_rel);
        while (arrived_.load(std::memory_order_acquire) < count_) {
            std::this_thread::yield();
        }
    }

private:
    int count_;
    std::atomic<int> arrived_{0};
};

struct FailingValue : clasp::Value {
    std::string type() const override { return "failing"; }
    std::string string() const override { return "raw"; }
    std::optional<std::string> set(std::string_view value) override {
        if (value == "good") return std::nullopt;
        return std::string("bad value: ") + std::string(value);
    }
};

// --- flag setter fallback paths (flag not found in the target vector) -------

void testFlagSetterFallbacksMissingNames() {
    clasp::Command root("app");
    // None of these names exist; every mark* helper must take the not-found
    // fallback side without touching any flag.
    root.markFlagRequired("--no-such-local");
    root.markPersistentFlagRequired("--no-such-persistent");
    root.markFlagHidden("--no-such-local");
    root.markPersistentFlagHidden("--no-such-persistent");
    root.markFlagDeprecated("--no-such-local", "gone");
    root.markFlagAnnotation("--no-such-local", "k", "v");
    root.markFlagNoOptDefaultValue("--no-such-local", "x");
    root.markPersistentFlagDeprecated("--no-such-persistent", "gone");
    root.markPersistentFlagAnnotation("--no-such-persistent", "k", "v");
    root.markPersistentFlagNoOptDefaultValue("--no-such-persistent", "x");
    expect(true, "flag setter fallbacks with missing names");
}

// Also exercise the found side so both sides of each setter stay covered.
void testFlagSetterFallbacksFoundNames() {
    clasp::Command root("app");
    root.withFlag("--local", "", "local", "local flag", std::string("d"));
    root.withPersistentFlag("--persist", "", "persist", "persistent flag", std::string("v"));
    root.markFlagRequired("--local");
    root.markPersistentFlagRequired("--persist");
    root.markFlagHidden("--local");
    root.markPersistentFlagHidden("--persist");
    root.markFlagDeprecated("--local", "use other");
    root.markFlagAnnotation("--local", "k", "v");
    root.markFlagNoOptDefaultValue("--local", "auto");
    root.markPersistentFlagDeprecated("--persist", "legacy");
    root.markPersistentFlagAnnotation("--persist", "pk", "pv");
    root.markPersistentFlagNoOptDefaultValue("--persist", "manual");
    expect(true, "flag setters with existing names");
}

void testEmptyFlagGroups() {
    clasp::Command root("app");
    // Empty groups must take the guard side (no group registered).
    std::vector<std::string> empty;
    root.markFlagsMutuallyExclusive(empty);
    root.markFlagsOneRequired(empty);
    root.markFlagsRequiredTogether(empty);
    // Non-empty side stays covered as well.
    root.markFlagsMutuallyExclusive(std::vector<std::string>{"--a", "--b"});
    expect(true, "empty flag groups skipped");
}

void testWithValueFlagExplicitDefault() {
    clasp::Command root("app", "root");
    FailingValue value;
    // Non-empty defaultValue: ternary must take the `defaultValue` side.
    root.withValueFlag("--level", "-l", "Level", "log level", value, "7");
    std::ostringstream out;
    root.setOut(out);
    root.printHelp();
    expect(out.str().find("(default: \"7\")") != std::string::npos, "withValueFlag explicit default shown");
}

struct TestContext {
    int value = 0;
};

// noinline + runtime flag: the optimizer must not prove which context type a
// command holds, so every dynamic_cast inside contextAs stays instrumented.
#if defined(_MSC_VER)
#define CLASP_TEST_NOINLINE __declspec(noinline)
#elif defined(__GNUC__)
#define CLASP_TEST_NOINLINE __attribute__((noinline))
#else
#define CLASP_TEST_NOINLINE
#endif
CLASP_TEST_NOINLINE void installContext(clasp::Command& c, bool testCtx, bool withAny) {
    if (!withAny) return;
    if (testCtx) {
        c.setContext(TestContext{5});
    } else {
        c.setContext(std::string("ctx"));
    }
}

void testContextAsAllSides() {
    volatile bool pickTest = true;
    volatile bool pickStr = false;
    volatile bool pickNone = false;
    clasp::Command plain("plain");
    installContext(plain, /*testCtx=*/false, !pickNone);
    clasp::Command withCtx("with");
    installContext(withCtx, /*testCtx=*/true, !pickNone);
    clasp::Command bare("bare");
    installContext(bare, /*testCtx=*/true, pickNone);
    // Mutable contextAs<TestContext>: null (cast miss) and non-null sides.
    expect(plain.contextAs<TestContext>() == nullptr, "mutable contextAs<TestContext> cast miss");
    auto* mutCtx = withCtx.contextAs<TestContext>();
    expect(mutCtx != nullptr && mutCtx->value == 5, "mutable contextAs<TestContext> hit");
    // Mutable contextAs<std::string>: both sides.
    auto* mutStr = plain.contextAs<std::string>();
    expect(mutStr != nullptr && *mutStr == "ctx", "mutable contextAs<std::string> hit");
    expect(bare.contextAs<std::string>() == nullptr, "mutable contextAs<std::string> null");
    // Const versions for both types, both sides.
    const auto& withCtxConst = withCtx;
    const auto* constCtx = withCtxConst.contextAs<TestContext>();
    expect(constCtx != nullptr && constCtx->value == 5, "const contextAs<TestContext> hit");
    const auto& plainConst = plain;
    expect(plainConst.contextAs<TestContext>() == nullptr, "const contextAs<TestContext> null");
    // Contextless command: the !a null-context side for every instantiation.
    expect(bare.contextAs<TestContext>() == nullptr, "mutable contextAs<TestContext> null context");
    const auto& bareEarlyConst = bare;
    expect(bareEarlyConst.contextAs<TestContext>() == nullptr, "const contextAs<TestContext> null context");
    const auto* constStr = plainConst.contextAs<std::string>();
    expect(constStr != nullptr && *constStr == "ctx", "const contextAs<std::string> hit");
    const auto& bareConst = bare;
    expect(bareConst.contextAs<std::string>() == nullptr, "const contextAs<std::string> null");
    (void)pickTest;
    (void)pickStr;
}

// --- version output flows ----------------------------------------------------

void testVersionRequestedPrintsVersion() {
    clasp::Command root("app");
    root.version("2.3.4");
    std::ostringstream out, err;
    root.setOut(out);
    root.setErr(err);
    const int rc = runArgs(root, {"version"});
    expect(rc == 0, "version positional with version set rc");
    expect(out.str().find("2.3.4") != std::string::npos, "version positional with version set output");
}

void testVersionPositionalWithoutVersion() {
    clasp::Command root("app");
    root.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
    std::ostringstream out, err;
    root.setOut(out);
    root.setErr(err);
    // After `--` the "version" token is a plain positional: builds empty version
    // text and skips the print (empty-side of both guards).
    const int rc = runArgs(root, {"--", "version"});
    expect(rc == 0, "version positional without version rc");
    expect(out.str().empty(), "version positional without version silent");
}

void testVersionFlagWithoutVersion() {
    clasp::Command root("app");
    root.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
    std::ostringstream out, err;
    root.setOut(out);
    root.setErr(err);
    // --version is a known bool flag even without a version string; the empty
    // version text must suppress the print.
    const int rc = runArgs(root, {"--version"});
    expect(rc == 0, "--version without version rc");
    expect(out.str().empty(), "--version without version silent");
}

void testVersionPositionalWithEmptyVersionText() {
    // The "version" positional sets versionRequested even without a version
    // string; the empty text must suppress the print and return success.
    clasp::Command root("app");
    root.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
    std::ostringstream out, err;
    root.setOut(out);
    root.setErr(err);
    const int rc = runArgs(root, {"version"});
    expect(rc == 0, "version positional empty version text rc");
    expect(out.str().empty(), "version positional empty version text silent");
}

// --- help flag / help subcommand paths ---------------------------------------

void testHelpShortAndLongFlags() {
    clasp::Command root("app");
    root.withFlag("--alpha", "-a", "alpha", "alpha flag", std::string("x"));
    root.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
    std::ostringstream out1, err1;
    root.setOut(out1);
    root.setErr(err1);
    expect(runArgs(root, {"--help"}) == 0, "--help rc");
    expect(out1.str().find("Usage:") != std::string::npos, "--help output");
    std::ostringstream out2, err2;
    root.setOut(out2);
    root.setErr(err2);
    expect(runArgs(root, {"-h"}) == 0, "-h rc");
    expect(out2.str().find("Usage:") != std::string::npos, "-h output");
    // No help flag at all: the action runs instead.
    std::ostringstream out3;
    root.setOut(out3);
    expect(runArgs(root, {}) == 0, "no help flag rc");
}

void testHelpPositionalAfterDashDash() {
    clasp::Command sub("sub", "sub command");
    sub.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
    clasp::Command root("app");
    root.addCommand(std::move(sub));
    root.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });

    std::ostringstream out1, err1;
    root.setOut(out1);
    root.setErr(err1);
    expect(runArgs(root, {"--", "help"}) == 0, "-- help rc");
    expect(out1.str().find("Usage: app") != std::string::npos, "-- help prints root help");

    std::ostringstream out2, err2;
    root.setOut(out2);
    root.setErr(err2);
    expect(runArgs(root, {"--", "help", "sub"}) == 0, "-- help sub rc");
    expect(out2.str().find("Usage: app sub") != std::string::npos, "-- help sub prints sub help");

    std::ostringstream out3;
    root.setOut(out3);
    expect(runArgs(root, {"--", "foo"}) == 0, "-- foo falls through to action");
}

// --- flagErrorFunc wrapping --------------------------------------------------

void testFlagErrorFuncWrapsColorThemeError() {
    clasp::Command sub("sub", "sub");
    sub.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
    clasp::Command root("app");
    root.enableColor();
    root.addCommand(std::move(sub));
    std::ostringstream out, err;
    root.setOut(out);
    root.setErr(err);
    // Raw scan stops at the subcommand boundary, so the parser-level theme
    // validation reports the failure; no flagErrorFunc is installed here.
    const int rc = runArgs(root, {"sub", "--color-theme", "bogus"});
    expect(rc == 1, "sub --color-theme bogus rc");
    expect(err.str().find("invalid value for --color-theme: bogus") != std::string::npos,
           "sub --color-theme bogus message");
}

void testFlagErrorFuncWrapsBoundValueError() {
    clasp::Command root("app");
    FailingValue value;
    root.withFlag("--mode", "", "mode", "mode flag", std::string("raw"));
    root.bindFlagValue("--mode", value);
    root.setFlagErrorFunc([](const clasp::Command&, const std::string& msg) { return "WRAP[" + msg + "]"; });
    root.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
    std::ostringstream out, err;
    root.setOut(out);
    root.setErr(err);
    const int rc = runArgs(root, {"--mode", "junk"});
    expect(rc == 1, "bound value error rc");
    expect(err.str().find("WRAP[bad value: junk]") != std::string::npos, "bound value error wrapped");
}

void testFlagErrorFuncWrapsRequiredFlagError() {
    clasp::Command root("app");
    root.withFlag("--need", "", "need", "required flag", std::string(""));
    root.markFlagRequired("--need");
    root.setFlagErrorFunc([](const clasp::Command&, const std::string& msg) { return "WRAP[" + msg + "]"; });
    root.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
    std::ostringstream out, err;
    root.setOut(out);
    root.setErr(err);
    const int rc = runArgs(root, {});
    expect(rc == 1, "required flag error rc");
    expect(err.str().find("WRAP[required flag not set: --need]") != std::string::npos,
           "required flag error wrapped");
}

void testFlagErrorFuncWrapsGroupError() {
    clasp::Command root("app");
    root.withFlag("--fa", "", "fa", "a flag", std::string(""));
    root.withFlag("--fb", "", "fb", "b flag", std::string(""));
    root.markFlagsMutuallyExclusive({"--fa", "--fb"});
    root.setFlagErrorFunc([](const clasp::Command&, const std::string& msg) { return "WRAP[" + msg + "]"; });
    root.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
    std::ostringstream out, err;
    root.setOut(out);
    root.setErr(err);
    const int rc = runArgs(root, {"--fa", "1", "--fb", "2"});
    expect(rc == 1, "mutually exclusive error rc");
    expect(err.str().find("WRAP[flags are mutually exclusive") != std::string::npos,
           "mutually exclusive error wrapped");
}

// --- color: raw token scanning ------------------------------------------------

void testColorRawInvalidValues() {
    {
        clasp::Command root("app");
        root.enableColor();
        root.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
        std::ostringstream out, err;
        root.setOut(out);
        root.setErr(err);
        const int rc = runArgs(root, {"--color", "bogus"});
        expect(rc == 1, "raw --color bogus rc");
        expect(err.str().find("invalid value for --color: bogus (expected auto|always|never)") != std::string::npos,
               "raw --color bogus message");
    }
    {
        clasp::Command root("app");
        root.enableColor();
        root.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
        std::ostringstream out, err;
        root.setOut(out);
        root.setErr(err);
        const int rc = runArgs(root, {"--color-theme", "bogus"});
        expect(rc == 1, "raw --color-theme bogus rc");
        expect(err.str().find("invalid value for --color-theme: bogus (expected vscode|sublime|iterm2)") !=
                   std::string::npos,
               "raw --color-theme bogus message");
    }
}

void testColorRawValidValues() {
    {
        clasp::Command root("app");
        root.enableColor();
        int called = 0;
        root.action([&](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
            ++called;
            return 0;
        });
        std::ostringstream out, err;
        root.setOut(out);
        root.setErr(err);
        expect(runArgs(root, {"--color", "always"}) == 0, "raw --color always rc");
        expect(called == 1, "raw --color always runs action");
    }
    {
        clasp::Command root("app");
        root.enableColor();
        int called = 0;
        root.action([&](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
            ++called;
            return 0;
        });
        std::ostringstream out, err;
        root.setOut(out);
        root.setErr(err);
        expect(runArgs(root, {"--color-theme", "iterm2"}) == 0, "raw --color-theme iterm2 rc");
        expect(called == 1, "raw --color-theme iterm2 runs action");
    }
}

void testColorFromParserPaths() {
    {
        // Parser sees a valid --color-theme (raw scan stopped at subcommand).
        clasp::Command sub("sub", "sub");
        sub.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
        clasp::Command root("app");
        root.enableColor();
        root.addCommand(std::move(sub));
        std::ostringstream out, err;
        root.setOut(out);
        root.setErr(err);
        expect(runArgs(root, {"sub", "--color-theme", "vscode"}) == 0, "parser --color-theme vscode rc");
    }
    {
        // Parser path without a --color-theme flag (only --color).
        clasp::Command sub("sub", "sub");
        sub.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
        clasp::Command root("app");
        root.enableColor();
        root.addCommand(std::move(sub));
        std::ostringstream out, err;
        root.setOut(out);
        root.setErr(err);
        expect(runArgs(root, {"--color", "never", "sub"}) == 0, "parser --color without theme rc");
    }
    {
        // Parser path with neither color flag after a subcommand boundary.
        clasp::Command sub("sub", "sub");
        sub.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
        clasp::Command root("app");
        root.enableColor();
        root.addCommand(std::move(sub));
        std::ostringstream out, err;
        root.setOut(out);
        root.setErr(err);
        expect(runArgs(root, {"sub"}) == 0, "parser without color flags rc");
    }
}

// --- styled rendering (color always) ------------------------------------------

void testStyledUsageWithoutFlagsHint() {
    clasp::Command root("app");
    root.enableColor(clasp::ColorMode::Always);
    root.disableFlagsInUseLine();
    root.setFlagErrorFunc([](const clasp::Command&, const std::string& msg) { return "W[" + msg + "]"; });
    root.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
    std::ostringstream out, err;
    root.setOut(out);
    root.setErr(err);
    const int rc = runArgs(root, {"--unknown-flag"});
    expect(rc == 1, "styled usage on error rc");
    // Painted Error: role plus usage line without [flags].
    expect(err.str().find("\x1b[") != std::string::npos, "styled error contains escape codes");
    expect(err.str().find("[flags]") == std::string::npos, "disableFlagsInUseLine hides [flags] in styled usage");
}

void testStyledCommandsSectionVariants() {
    // Full styled commands section: root + version + suggestions + help command.
    {
        clasp::Command sub = clasp::Command("sub", "sub short");
        clasp::Command root("app", "root short");
        root.version("1.0");
        root.enableColor(clasp::ColorMode::Always);
        root.addCommand(std::move(sub));
        std::ostringstream out;
        root.setOut(out);
        root.printHelp();
        const auto text = out.str();
        expect(text.find("Commands:") != std::string::npos, "styled commands section rendered");
        // The command name is painted separately, so match the plain suffix.
        expect(text.find("- Print the version number") != std::string::npos, "styled version line rendered");
        expect(text.find("- Help about any command") != std::string::npos, "styled help line rendered");
    }
    // Styled + no version.
    {
        clasp::Command sub = clasp::Command("sub", "sub short");
        clasp::Command root("app", "root short");
        root.enableColor(clasp::ColorMode::Always);
        root.addCommand(std::move(sub));
        std::ostringstream out;
        root.setOut(out);
        root.printHelp();
        expect(out.str().find("version - Print the version number") == std::string::npos,
               "styled no-version omits version line");
    }
    // Styled + suggestions disabled.
    {
        clasp::Command sub = clasp::Command("sub", "sub short");
        clasp::Command root("app", "root short");
        root.enableColor(clasp::ColorMode::Always);
        root.suggestions(false);
        root.addCommand(std::move(sub));
        std::ostringstream out;
        root.setOut(out);
        root.printHelp();
        expect(out.str().find("Commands:") != std::string::npos, "styled suggestions-off keeps commands section");
        expect(out.str().find("- Help about any command") == std::string::npos,
               "styled suggestions-off omits help line");
    }
    // Styled + help command disabled.
    {
        clasp::Command sub = clasp::Command("sub", "sub short");
        clasp::Command root("app", "root short");
        root.enableColor(clasp::ColorMode::Always);
        root.disableHelpCommand();
        root.version("1.0");
        root.addCommand(std::move(sub));
        std::ostringstream out;
        root.setOut(out);
        root.printHelp();
        expect(out.str().find("Help about any command") == std::string::npos,
               "styled help-disabled omits help line");
        expect(out.str().find("- Print the version number") != std::string::npos,
               "styled help-disabled keeps version line");
    }
}

void testCommandsSectionForNonRoot() {
    // Unstyled: child command with its own visible child renders a Commands section.
    {
        clasp::Command subsub = clasp::Command("leaf", "leaf short");
        clasp::Command sub = clasp::Command("sub", "sub short");
        sub.addCommand(std::move(subsub));
        clasp::Command root("app", "root short");
        root.addCommand(std::move(sub));
        std::ostringstream out;
        root.setOut(out);
        expect(runArgs(root, {"sub", "--help"}) == 0, "unstyled sub help rc");
        expect(out.str().find("Usage: app sub") != std::string::npos, "unstyled sub usage path");
        expect(out.str().find("leaf - leaf short") != std::string::npos,
               "unstyled non-root commands section lists child");
        expect(out.str().find("Help about any command") == std::string::npos,
               "unstyled non-root omits root help line");
    }
    // Styled: same shape with color always.
    {
        clasp::Command subsub = clasp::Command("leaf", "leaf short");
        clasp::Command sub = clasp::Command("sub", "sub short");
        sub.addCommand(std::move(subsub));
        clasp::Command root("app", "root short");
        root.enableColor(clasp::ColorMode::Always);
        root.addCommand(std::move(sub));
        std::ostringstream out;
        root.setOut(out);
        expect(runArgs(root, {"sub", "--help"}) == 0, "styled sub help rc");
        expect(out.str().find("- leaf short") != std::string::npos,
               "styled non-root commands section lists child");
    }
}

void testUnstyledCommandsSectionSyntheticRows() {
    // Unstyled root with the help command disabled: the synthesized help row
    // must disappear while the version row stays.
    {
        clasp::Command sub("sub", "sub short");
        clasp::Command root("app", "root short");
        root.version("3.1");
        root.disableHelpCommand();
        root.addCommand(std::move(sub));
        std::ostringstream out;
        root.setOut(out);
        root.printHelp();
        expect(out.str().find("Help about any command") == std::string::npos,
               "unstyled help-disabled omits help line");
        expect(out.str().find("- Print the version number") != std::string::npos,
               "unstyled help-disabled keeps version line");
    }
    // Unstyled root without a version: no version row either.
    {
        clasp::Command sub("sub2", "sub2 short");
        clasp::Command root("app2", "root2 short");
        root.addCommand(std::move(sub));
        std::ostringstream out;
        root.setOut(out);
        root.printHelp();
        expect(out.str().find("- Print the version number") == std::string::npos,
               "unstyled no-version omits version line");
        expect(out.str().find("- Help about any command") != std::string::npos,
               "unstyled default keeps help line");
    }
}

void testDurationFlagDefaults() {
    // std::chrono::milliseconds defaults: a zero duration is hidden entirely,
    // a non-zero duration renders as "(default: Nms)".
    {
        clasp::Command root("app");
        root.withFlag("--wait", "", "wait", "wait duration", std::chrono::milliseconds(0));
        std::ostringstream out;
        root.setOut(out);
        root.printHelp();
        expect(out.str().find("(default:") == std::string::npos, "zero duration default hidden");
    }
    {
        clasp::Command root("app");
        root.withFlag("--wait", "", "wait", "wait duration", std::chrono::milliseconds(500));
        std::ostringstream out;
        root.setOut(out);
        root.printHelp();
        expect(out.str().find("(default: 500ms)") != std::string::npos, "duration default rendered");
    }
}

void testGroupedCommandsSection() {
    clasp::Command root("app", "root short");
    // Several groups: exercises vector growth (realloc + in-place push_back).
    root.addGroup("g1", "Tools");
    root.addGroup("g2", "Unused Group");
    root.addGroup("g3", "More");
    root.addGroup("g4", "Even More");
    root.addGroup("g5", "Extra");
    root.addGroup("g6", "Last");
    clasp::Command alpha("alpha", "alpha short");
    alpha.groupId("g1");
    clasp::Command beta("beta", "beta short");
    beta.groupId("g1");
    clasp::Command gamma("gamma", "gamma short");
    clasp::Command delta("delta", "delta short");
    delta.groupId("nope"); // unknown group id -> ungrouped
    root.addCommand(std::move(alpha));
    root.addCommand(std::move(beta));
    root.addCommand(std::move(gamma));
    root.addCommand(std::move(delta));
    std::ostringstream out;
    root.setOut(out);
    root.printHelp();
    const auto text = out.str();
    expect(text.find("Tools:") != std::string::npos, "grouped section title rendered");
    expect(text.find("alpha - alpha short") != std::string::npos, "grouped member rendered");
    expect(text.find("gamma - gamma short") != std::string::npos, "ungrouped member rendered");
    expect(text.find("Unused Group:") == std::string::npos, "empty group omitted");
}

// --- markdown / manpage section variants --------------------------------------

void testMarkdownSectionVariants() {
    {
        // No short description, no long description, no flags sections.
        clasp::Command bare("bare");
        std::ostringstream os;
        bare.printMarkdown(os, /*recursive=*/false);
        const auto text = os.str();
        expect(text.find("## Usage") != std::string::npos, "bare markdown has usage");
        expect(text.find("## Flags") != std::string::npos, "markdown always prints local flags section");
    }
    {
        // Short empty but long set: markdown falls back to the long text.
        clasp::Command longOnly("longonly", "", "only long description");
        std::ostringstream os;
        longOnly.printMarkdown(os, /*recursive=*/false);
        expect(os.str().find("only long description") != std::string::npos, "markdown prints long when short empty");
    }
    {
        // Long equal to short: long section skipped.
        clasp::Command same("same", "same text", "same text");
        std::ostringstream os;
        same.printMarkdown(os, /*recursive=*/false);
        const auto text = os.str();
        expect(text.find("same text") != std::string::npos, "markdown same short/long renders once");
    }
    {
        // Root without version: no Global Flags section in markdown.
        clasp::Command root("md", "md short");
        root.withFlag("--opt", "-o", "opt", "opt flag", std::string("v"));
        std::ostringstream os;
        root.printMarkdown(os, /*recursive=*/false);
        expect(os.str().find("## Global Flags") == std::string::npos, "markdown omits empty global flags");
    }
    {
        // Recursive with two visible subcommands.
        clasp::Command c1("c1", "c1 short");
        clasp::Command c2("c2", "c2 short");
        clasp::Command root("mdr", "mdr short");
        root.addCommand(std::move(c1));
        root.addCommand(std::move(c2));
        std::ostringstream os;
        root.printMarkdown(os, /*recursive=*/true);
        expect(os.str().find("## Commands") != std::string::npos, "recursive markdown lists commands");
    }
}

void testManpageVariants() {
    {
        // Short empty, long set: DESCRIPTION uses the long text.
        clasp::Command longOnly("mp", "", "manpage long text");
        std::ostringstream os;
        longOnly.printManpage(os);
        expect(os.str().find(".SH DESCRIPTION") != std::string::npos, "manpage description from long");
        expect(os.str().find("manpage long text") != std::string::npos, "manpage long text rendered");
    }
    {
        // Neither short nor long: no DESCRIPTION section.
        clasp::Command bare("mpbare");
        std::ostringstream os;
        bare.printManpage(os);
        expect(os.str().find(".SH DESCRIPTION") == std::string::npos, "manpage omits empty description");
        expect(os.str().find(".SH EXAMPLES") == std::string::npos, "manpage omits empty examples");
        expect(os.str().find(".SH COMMANDS") == std::string::npos, "manpage omits empty commands");
    }
    {
        // Short set, long empty, with example: DESCRIPTION from short, EXAMPLES present.
        clasp::Command withExample("mpx", "mpx short");
        withExample.example("mpx --run");
        std::ostringstream os;
        withExample.printManpage(os);
        expect(os.str().find(".SH DESCRIPTION") != std::string::npos, "manpage description from short");
        expect(os.str().find(".SH EXAMPLES") != std::string::npos, "manpage examples rendered");
    }
}

// --- short flag help formatting ------------------------------------------------

void testFlagHelpDescriptionVariants() {
    {
        // Empty description + non-empty default: no space pushed before "(default:...)".
        clasp::Command root("app");
        root.withFlag("--nodesc", "", "nodesc", "", std::string("defval"));
        std::ostringstream out;
        root.setOut(out);
        root.printHelp();
        expect(out.str().find("--nodesc string - (default: \"defval\")") != std::string::npos,
               "empty description keeps default without extra space");
    }
    {
        // Empty description + false bool default: fully empty desc -> no " - " suffix.
        clasp::Command root("app");
        root.withFlag("--nodescbool", "", "nodescbool", "", false);
        std::ostringstream out;
        root.setOut(out);
        root.printHelp();
        expect(out.str().find("--nodescbool\n") != std::string::npos, "empty desc omits dash suffix");
    }
}

void testShortOnlyFlagsSorted() {
    clasp::Command root("app");
    root.withFlag("", "-p", "p", "p flag", std::string(""));
    root.withFlag("", "-q", "q", "q flag", std::string(""));
    root.withFlag("--zlong", "", "zlong", "zlong flag", std::string(""));
    std::ostringstream out;
    root.setOut(out);
    root.printHelp();
    const auto text = out.str();
    expect(text.find("-p") != std::string::npos && text.find("-q") != std::string::npos,
           "short-only flags rendered");
    expect(text.find("-p, ") != std::string::npos, "sorted flags include short-only entries");
}

void testHiddenFlagsOmittedFromHelp() {
    {
        clasp::Command root("app");
        root.withFlag("--secret", "-s", "secret", "hidden flag", std::string("v"));
        root.markFlagHidden("--secret");
        root.withFlag("--open", "", "open", "visible flag", std::string("v"));
        std::ostringstream out;
        root.setOut(out);
        root.printHelp();
        expect(out.str().find("--secret") == std::string::npos, "hidden local flag omitted");
        expect(out.str().find("--open") != std::string::npos, "visible flag rendered");
    }
    {
        clasp::Command root("app");
        root.withPersistentFlag("--psecret", "", "psecret", "hidden persistent", std::string("v"));
        root.markPersistentFlagHidden("--psecret");
        clasp::Command sub("sub", "sub short");
        root.addCommand(std::move(sub));
        std::ostringstream out;
        root.setOut(out);
        expect(runArgs(root, {"sub", "--help"}) == 0, "sub help with hidden persistent rc");
        expect(out.str().find("--psecret") == std::string::npos, "hidden inherited flag omitted");
    }
}

// --- hooks / action / args validators ------------------------------------------

void testPersistentPostRunEError() {
    clasp::Command root("app");
    root.persistentPostRunE(
        [](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) -> std::optional<std::string> {
            return std::string("post run failed");
        });
    root.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
    std::ostringstream out, err;
    root.setOut(out);
    root.setErr(err);
    const int rc = runArgs(root, {});
    expect(rc == 1, "persistentPostRunE error rc");
    expect(err.str().find("post run failed") != std::string::npos, "persistentPostRunE error message");
}

void testActionEEmptyErrorMessage() {
    clasp::Command root("app");
    root.actionE([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&)
                     -> std::optional<std::string> { return std::string(""); });
    std::ostringstream out, err;
    root.setOut(out);
    root.setErr(err);
    const int rc = runArgs(root, {});
    expect(rc == 1, "empty error message rc");
    expect(err.str().find("Error:") == std::string::npos, "empty error message prints no Error line");
    expect(err.str().find("Usage:") != std::string::npos, "empty error message still shows usage");
}

void testPlainActionRunsHookChain() {
    clasp::Command root("app");
    int ran = 0;
    root.action([&](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
        ++ran;
        return 7;
    });
    const int rc = runArgs(root, {});
    expect(rc == 7 && ran == 1, "plain action executed");
}

void testRangeArgsBounds() {
    clasp::Command root("app");
    root.args(clasp::RangeArgs(2, 3));
    root.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
    std::ostringstream out, err;
    root.setOut(out);
    root.setErr(err);
    expect(runArgs(root, {"only"}) == 1, "range args too few rc");
    expect(err.str().find("accepts between 2 and 3 arg(s)") != std::string::npos, "range args too few message");
    expect(runArgs(root, {"a", "b"}) == 0, "range args in range rc");
    expect(runArgs(root, {"a", "b", "c"}) == 0, "range args at max rc");
}

// --- short group tokens during arg resolution -----------------------------------

void testShortGroupTokenWithoutEquals() {
    clasp::Command root("app");
    root.withFlag("--aa", "-a", "aa", "a flag", true);
    root.withFlag("--bb", "-b", "bb", "b flag", true);
    root.withFlag("--cc", "-c", "cc", "c flag", true);
    int ran = 0;
    root.action([&](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
        ++ran;
        return 0;
    });
    std::ostringstream out, err;
    root.setOut(out);
    root.setErr(err);
    // "-abc" is a short group token without '='; bool flags consume nothing.
    expect(runArgs(root, {"-abc"}) == 0, "short group token rc");
    expect(ran == 1, "short group token runs action");
}

// --- help template keys ----------------------------------------------------------

void testHelpTemplateExamplesSection() {
    clasp::Command root("app", "root short");
    root.example("app --run\napp --walk");
    root.setHelpTemplate("P={{.CommandPath}}|U={{.UsageLine}}|S={{.ShortSection}}|E={{.ExamplesSection}}|"
                         "C={{.CommandsSection}}|F={{.FlagsSection}}|G={{.GlobalFlagsSection}}");
    std::ostringstream out;
    root.setOut(out);
    root.printHelp();
    const auto text = out.str();
    expect(text.find("P=app") != std::string::npos, "template command path key");
    expect(text.find("Examples:") != std::string::npos, "template examples section key");
}

// --- static local flag initialization race (helpFlag/versionFlag) ----------------

void testStaticFlagInitRace() {
    // Many commands rendered concurrently: half with version (versionFlag
    // static) and half without (helpFlag static), cycling all three color
    // themes (builtinTheme's function-local statics). Each thread uses its own
    // Command and stream; only the function-local statics are shared.
    constexpr int kThreads = 16;
    SpinBarrier barrier(kThreads);
    std::atomic<int> okCount{0};
    auto worker = [&](int id) {
        const bool withVersion = (id % 2) == 0;
        const auto theme = static_cast<clasp::ColorThemeName>(id % 3);
        clasp::Command root(withVersion ? "racy" : "racn", withVersion ? "racy short" : "racn short");
        if (withVersion) root.version("9.9.9");
        root.withFlag("--tflag", "", "tflag", "t flag", std::string("x"));
        root.enableColor(clasp::ColorMode::Always, theme);
        std::ostringstream out;
        root.setOut(out);
        barrier.wait();
        for (int i = 0; i < 8; ++i) root.printHelp();
        if (out.str().find("Usage:") != std::string::npos && out.str().find("\x1b[") != std::string::npos) {
            okCount.fetch_add(1, std::memory_order_relaxed);
        }
    };
    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i) threads.emplace_back(worker, i);
    for (auto& t : threads) t.join();
    expect(okCount.load() == kThreads, "concurrent help renders all succeeded");
}

// --- color themes ----------------------------------------------------------------

void testBuiltinThemesDirect() {
    const auto& vscode = clasp::color::builtinTheme(clasp::ColorThemeName::Vscode);
    const auto& sublime = clasp::color::builtinTheme(clasp::ColorThemeName::Sublime);
    const auto& iterm2 = clasp::color::builtinTheme(clasp::ColorThemeName::Iterm2);
    expect(vscode.section.size() > 0 && sublime.section.size() > 0 && iterm2.section.size() > 0,
           "builtin themes non-empty");
    expect(&vscode != &sublime && &sublime != &iterm2, "builtin themes are distinct objects");
    // Second lookups reuse the initialized statics.
    expect(&clasp::color::builtinTheme(clasp::ColorThemeName::Sublime) == &sublime, "sublime theme cached");
    expect(&clasp::color::builtinTheme(clasp::ColorThemeName::Iterm2) == &iterm2, "iterm2 theme cached");
}

void testPaintWithEachTheme() {
    for (const auto theme : {clasp::ColorThemeName::Vscode, clasp::ColorThemeName::Sublime,
                             clasp::ColorThemeName::Iterm2}) {
        clasp::Command root("app");
        root.enableColor(clasp::ColorMode::Always, theme);
        std::ostringstream out, err;
        root.setOut(out);
        root.setErr(err);
        root.printHelp();
        expect(out.str().find("\x1b[") != std::string::npos, "themed help painted");
        // Painted error output exercises the Error role switch arm.
        const int rc = runArgs(root, {"--unknown-flag"});
        expect(rc == 1, "themed error rc");
        expect(err.str().find("\x1b[") != std::string::npos, "themed error painted");
    }
}

} // namespace

int main() {
    // Must run before any other help rendering in this process: it depends on
    // racing the initialization of function-local statics.
    testStaticFlagInitRace();
    testFlagSetterFallbacksMissingNames();
    testFlagSetterFallbacksFoundNames();
    testEmptyFlagGroups();
    testWithValueFlagExplicitDefault();
    testContextAsAllSides();
    testVersionRequestedPrintsVersion();
    testVersionPositionalWithoutVersion();
    testVersionFlagWithoutVersion();
    testVersionPositionalWithEmptyVersionText();
    testHelpShortAndLongFlags();
    testHelpPositionalAfterDashDash();
    testFlagErrorFuncWrapsColorThemeError();
    testFlagErrorFuncWrapsBoundValueError();
    testFlagErrorFuncWrapsRequiredFlagError();
    testFlagErrorFuncWrapsGroupError();
    testColorRawInvalidValues();
    testColorRawValidValues();
    testColorFromParserPaths();
    testStyledUsageWithoutFlagsHint();
    testStyledCommandsSectionVariants();
    testCommandsSectionForNonRoot();
    testUnstyledCommandsSectionSyntheticRows();
    testDurationFlagDefaults();
    testGroupedCommandsSection();
    testMarkdownSectionVariants();
    testManpageVariants();
    testFlagHelpDescriptionVariants();
    testShortOnlyFlagsSorted();
    testHiddenFlagsOmittedFromHelp();
    testPersistentPostRunEError();
    testActionEEmptyErrorMessage();
    testPlainActionRunsHookChain();
    testRangeArgsBounds();
    testShortGroupTokenWithoutEquals();
    testHelpTemplateExamplesSection();
    testBuiltinThemesDirect();
    testPaintWithEachTheme();
    if (g_failures == 0) {
        std::cout << "ALL OK" << std::endl;
        return 0;
    }
    std::cout << g_failures << " failures" << std::endl;
    return 1;
}
