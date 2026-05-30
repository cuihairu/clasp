#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "clasp/clasp.hpp"

namespace {

struct TestContext {
    std::string value{"test_context"};
    int count{42};
};

void testContextMethods() {
    clasp::Command root("app", "Test context");

    TestContext ctx;
    ctx.value = "hello";
    ctx.count = 100;

    root.setContext(ctx);

    bool hasCtx = root.hasContext();
    (void)hasCtx;

    // Test const contextAs
    const clasp::Command& constRoot = root;
    auto ctxPtr = constRoot.contextAs<TestContext>();
    if (ctxPtr) {
        auto val = ctxPtr->value;
        auto cnt = ctxPtr->count;
        (void)val; (void)cnt;
    }
}

void testSetArgsAndExecute() {
    clasp::Command root("app", "Test setArgs");
    root.withFlag("--name", "-n", "name", "Name", std::string("default"));

    bool actionCalled = false;
    root.action([&](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
        actionCalled = true;
        return 0;
    });

    // Test setArgs + execute
    root.setArgs({"--name", "test"});
    int rc = root.execute();
    (void)rc;

    // Test execute with args
    const char* argv[] = {"app", "--name", "another"};
    rc = root.execute(3, const_cast<char**>(argv));
    (void)rc;
}

void testExecuteWithContext() {
    clasp::Command root("app", "Test executeWithContext");
    root.withFlag("--debug", "-d", "debug", "Debug", false);

    bool actionCalled = false;
    root.action([&](clasp::Command& cmd, const clasp::Parser&, const std::vector<std::string>&) {
        actionCalled = true;
        auto ctx = cmd.contextAs<TestContext>();
        if (ctx) {
            auto val = ctx->value;
            (void)val;
        }
        return 0;
    });

    TestContext ctx;
    ctx.value = "from_context";

    int rc = root.executeWithContext(ctx);
    (void)rc;
}

void testRunWithContext() {
    clasp::Command root("app", "Test runWithContext");
    root.withFlag("--verbose", "-v", "verbose", "Verbose", false);

    bool actionCalled = false;
    root.action([&](clasp::Command& cmd, const clasp::Parser&, const std::vector<std::string>&) {
        actionCalled = true;
        auto ctx = cmd.contextAs<TestContext>();
        if (ctx) {
            auto cnt = ctx->count;
            (void)cnt;
        }
        return 0;
    });

    TestContext ctx;
    ctx.count = 999;

    const char* argv[] = {"app", "--verbose"};
    int rc = root.runWithContext(ctx, 2, const_cast<char**>(argv));
    (void)rc;
}

void testPrintMarkdown() {
    clasp::Command root("app", "Test markdown");
    root.version("1.0.0");
    root.withFlag("--output", "-o", "output", "Output", std::string(""));
    root.example("app --output file.txt");

    clasp::Command sub("sub", "Sub command");
    sub.withFlag("--debug", "-d", "debug", "Debug", false);
    root.addCommand(std::move(sub));

    std::ostringstream oss;
    root.printMarkdown(oss, /*recursive=*/true);
    auto markdown = oss.str();
    (void)markdown;

    // Test non-recursive
    std::ostringstream oss2;
    root.printMarkdown(oss2, /*recursive=*/false);
    auto markdown2 = oss2.str();
    (void)markdown2;
}

void testPrintManpage() {
    clasp::Command root("app", "Test manpage");
    root.version("1.0.0");
    root.withFlag("--verbose", "-v", "verbose", "Verbose", false);
    root.example("app --verbose");

    clasp::Command sub("cmd", "Command");
    sub.withFlag("--debug", "-d", "debug", "Debug", false);
    root.addCommand(std::move(sub));

    std::ostringstream oss;
    root.printManpage(oss);
    auto manpage = oss.str();
    (void)manpage;
}

void testSetOutSetErr() {
    clasp::Command root("app", "Test setOut/setErr");

    std::ostringstream outStream;
    std::ostringstream errStream;

    root.setOut(outStream);
    root.setErr(errStream);

    root.action([](clasp::Command& cmd, const clasp::Parser&, const std::vector<std::string>&) {
        cmd.outOrStdout() << "test output\n";
        cmd.errOrStderr() << "test error\n";
        return 0;
    });

    const char* argv[] = {"app"};
    int rc = root.run(1, const_cast<char**>(argv));
    (void)rc;

    auto outContent = outStream.str();
    auto errContent = errStream.str();
    (void)outContent; (void)errContent;
}

void testOutOrStdoutErrOrStderr() {
    clasp::Command root("app", "Test outOrStdout/errOrStderr");

    auto& out = root.outOrStdout();
    auto& err = root.errOrStderr();

    out << "to stdout\n";
    err << "to stderr\n";

    (void)out; (void)err;
}

void testCommandPath() {
    clasp::Command root("root", "Root command");

    clasp::Command sub1("sub1", "Sub 1");
    clasp::Command sub2("sub2", "Sub 2");
    clasp::Command subsub("subsub", "Sub Sub");

    sub2.addCommand(std::move(subsub));
    root.addCommand(std::move(sub1)).addCommand(std::move(sub2));

    auto rootPath = root.commandPath();
    auto sub1Path = sub1.commandPath();
    auto sub2Path = sub2.commandPath();
    auto subsubPath = subsub.commandPath();

    (void)rootPath; (void)sub1Path; (void)sub2Path; (void)subsubPath;
}

void testSilenceMethods() {
    clasp::Command root("app", "Test silence");

    root.silenceUsage(true);
    root.silenceErrors(true);

    root.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
        return 0;
    });

    const char* argv[] = {"app"};
    int rc = root.run(1, const_cast<char**>(argv));
    (void)rc;
}

void testDisableSortMethods() {
    clasp::Command root("app", "Test disable sort");

    root.disableSortCommands(true);
    root.disableSortFlags(true);
    root.disableFlagsInUseLine(true);

    root.withFlag("--flag1", "-f", "flag1", "Flag1", std::string(""));

    const char* argv[] = {"app", "--help"};
    int rc = root.run(2, const_cast<char**>(argv));
    (void)rc;
}

void testDeprecatedAndHidden() {
    clasp::Command root("app", "Test deprecated and hidden");
    root.deprecated("This command is deprecated");
    root.hidden(true);

    root.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
        return 0;
    });

    const char* argv[] = {"app"};
    int rc = root.run(1, const_cast<char**>(argv));
    (void)rc;
}

void testVersionMethods() {
    clasp::Command root("app", "Test version");

    root.version("2.0.0");
    root.setVersionTemplate("Ver:{{.Version}} Path:{{.CommandPath}} Name:{{.Name}}");

    const char* argv[] = {"app", "--version"};
    int rc = root.run(2, const_cast<char**>(argv));
    (void)rc;
}

void testHelpTemplateMethods() {
    clasp::Command root("app", "Test help templates");

    root.setHelpTemplate("HELP: {{.CommandPath}} {{.UsageLine}} {{.ShortSection}}");
    root.setUsageTemplate("USAGE: {{.CommandPath}} {{.UsageLine}}");

    const char* argv[] = {"app", "--help"};
    int rc = root.run(2, const_cast<char**>(argv));
    (void)rc;
}

void testCompletionPrintMethods() {
    clasp::Command root("app", "Test completion print");
    root.enableCompletion();

    root.withFlag("--flag", "-f", "flag", "Flag", std::string(""));

    clasp::Command sub("cmd", "Command");
    sub.withFlag("--opt", "-o", "opt", "Opt", std::string(""));
    root.addCommand(std::move(sub));

    std::ostringstream bash;
    std::ostringstream zsh;
    std::ostringstream fish;
    std::ostringstream powerShell;

    root.printCompletionBash(bash);
    root.printCompletionZsh(zsh);
    root.printCompletionFish(fish);
    root.printCompletionPowerShell(powerShell);

    auto bashOut = bash.str();
    auto zshOut = zsh.str();
    auto fishOut = fish.str();
    auto psOut = powerShell.str();

    (void)bashOut; (void)zshOut; (void)fishOut; (void)psOut;
}

void testExampleMethods() {
    clasp::Command root("app", "Test examples");

    root.example("Example 1: app --flag value");
    root.examples("Example 2: app --other flag");

    const char* argv[] = {"app", "--help"};
    int rc = root.run(2, const_cast<char**>(argv));
    (void)rc;
}

void testFlagGroupMethods() {
    clasp::Command root("app", "Test flag groups");

    root.withFlag("--a", "", "a", "A", std::string(""));
    root.withFlag("--b", "", "b", "B", std::string(""));
    root.withFlag("--c", "", "c", "C", std::string(""));
    root.withFlag("--user", "-u", "user", "User", std::string(""));
    root.withFlag("--pass", "-p", "pass", "Pass", std::string(""));

    root.markFlagsMutuallyExclusive({"--a", "--c"});
    root.markFlagsOneRequired({"--a", "--b"});
    root.markFlagsRequiredTogether({"--user", "--pass"});

    root.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
        return 0;
    });

    const char* argv[] = {"app", "--a", "value", "--user", "name", "--pass", "secret"};
    int rc = root.run(5, const_cast<char**>(argv));
    (void)rc;
}

} // namespace

int main() {
    testContextMethods();
    testSetArgsAndExecute();
    testExecuteWithContext();
    testRunWithContext();
    testPrintMarkdown();
    testPrintManpage();
    testSetOutSetErr();
    testOutOrStdoutErrOrStderr();
    testCommandPath();
    testSilenceMethods();
    testDisableSortMethods();
    testDeprecatedAndHidden();
    testVersionMethods();
    testHelpTemplateMethods();
    testCompletionPrintMethods();
    testExampleMethods();
    testFlagGroupMethods();

    std::cout << "ok\n";
    return 0;
}
