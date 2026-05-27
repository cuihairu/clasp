#include <iostream>
#include <string>
#include <vector>

#include "clasp/clasp.hpp"

namespace {

void testEnableColorCompletion() {
    clasp::Command root("app", "Test color completion");

    // Enable color with default settings - this should register completion callbacks
    root.enableColor();

    // Create subcommands and flags to test completion
    clasp::Command sub("print", "Print command");
    sub.withFlag("--message", "-m", "msg", "Message", std::string("default"))
        .withFlag("--count", "-c", "count", "Count", 10);

    root.addCommand(std::move(sub));
}

void testEnableCompletionMethods() {
    clasp::Command root("app", "Test enable completion");

    // Test enableCompletion with default config
    root.enableCompletion();

    // Test enableCompletion with custom config
    clasp::Command::CompletionConfig cfg;
    cfg.addCompletionCommand = true;
    cfg.completionCommandName = "complete";
    cfg.addCompleteCommands = true;
    cfg.completeCommandName = "commands";
    cfg.completeNoDescCommandName = "commandsNoDesc";

    clasp::Command root2("app2", "Test enable completion with config");
    root2.enableCompletion(cfg);
}

void testCompletionWithSubcommands() {
    clasp::Command root("app", "Test completion with subcommands");

    // Enable completion
    root.enableCompletion();

    // Add subcommands
    clasp::Command cmd1("cmd1", "Command 1");
    cmd1.withFlag("--opt", "-o", "opt", "Opt", std::string("value"));

    clasp::Command cmd2("cmd2", "Command 2");
    cmd2.withFlag("--debug", "-d", "debug", "Debug", true);

    root.addCommand(std::move(cmd1)).addCommand(std::move(cmd2));
}

void testCompletionWithValidArgs() {
    clasp::Command root("app", "Test completion with valid args");

    // Enable completion
    root.enableCompletion();

    // Add subcommand with valid args
    clasp::Command sub("process", "Process command");
    sub.withFlag("--file", "-f", "file", "File", std::string(""));

    // Set valid args for completion
    sub.validArgs({"arg1", "arg2", "arg3"});

    // Set valid args function
    sub.validArgsFunction([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&, std::string_view) {
        return std::vector<std::string>{"func1", "func2"};
    });

    // Register flag completion
    sub.registerFlagCompletion("--file",
        [](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&, std::string_view) {
            return std::vector<std::string>{"file1.txt", "file2.txt"};
        });

    root.addCommand(std::move(sub));
}

void testPersistentFlagCompletion() {
    clasp::Command root("app", "Test persistent flag completion");

    // Enable completion
    root.enableCompletion();

    // Add persistent flags
    root.withPersistentFlag("--verbose", "-v", "Verbose")
        .withPersistentFlag("--debug", "-d", "Debug")
        .withPersistentFlag("--output", "-o", "Output");

    // Register completion for persistent flags
    root.registerFlagCompletion("--output",
        [](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&, std::string_view) {
            return std::vector<std::string>{"output.txt", "output.log"};
        });
}

void testCompletionWithTraverse() {
    clasp::Command root("app", "Test completion with traverse");

    // Enable completion and traverse
    root.enableCompletion();
    root.traverseChildren(true);

    // Add nested subcommands
    clasp::Command sub1("sub1", "Sub 1");
    sub1.withFlag("--opt", "-o", "opt", "Opt", std::string("value"));

    clasp::Command sub2("sub2", "Sub 2");
    clasp::Command subsub("subsub", "Sub Sub");
    subsub.withFlag("--value", "-v", "value", "Value", 42);

    sub2.addCommand(std::move(subsub));
    root.addCommand(std::move(sub1)).addCommand(std::move(sub2));
}

void testCompletionWithColor() {
    clasp::Command root("app", "Test completion with color");

    // Enable both color and completion
    root.enableColor();
    root.enableCompletion();

    // Add commands and flags
    clasp::Command sub("cmd", "Command");
    sub.withFlag("--opt", "-o", "opt", "Opt", std::string("default"));

    root.addCommand(std::move(sub));
}

void testCompletionDirectives() {
    clasp::Command root("app", "Test completion directives");

    // Set completion directive
    root.completionDirective(4);

    // Enable completion
    root.enableCompletion();

    // Add subcommand with filename completion
    clasp::Command sub("read", "Read file");
    sub.withFlag("--file", "-f", "file", "File", std::string(""));
    sub.markFlagFilename("--file", {"txt", "md", "log"});

    // Add subcommand with dirname completion
    clasp::Command sub2("list", "List directory");
    sub2.withFlag("--dir", "-d", "dir", "Dir", std::string(""));
    sub2.markFlagDirname("--dir");

    root.addCommand(std::move(sub)).addCommand(std::move(sub2));
}

void testComplexCompletionSetup() {
    clasp::Command root("app", "Test complex completion setup");

    // Enable with custom config
    clasp::Command::CompletionConfig cfg;
    cfg.addCompletionCommand = true;
    cfg.completionCommandName = "comp";
    cfg.addCompleteCommands = true;
    cfg.completeCommandName = "cmds";
    cfg.completeNoDescCommandName = "cmdsNoDesc";

    root.enableCompletion(cfg);
    root.traverseChildren(true);

    // Add multiple levels of subcommands
    clasp::Command level1("level1", "Level 1");
    clasp::Command level2("level2", "Level 2");
    clasp::Command level3("level3", "Level 3");

    level2.addCommand(std::move(level3));
    level1.addCommand(std::move(level2));

    // Add flags to each level
    level1.withFlag("--opt1", "-o", "opt1", "Opt1", std::string("v1"));
    level2.withFlag("--opt2", "-o", "opt2", "Opt2", std::string("v2"));
    level3.withFlag("--opt3", "-o", "opt3", "Opt3", std::string("v3"));

    // Register completions
    level1.registerFlagCompletion("--opt1",
        [](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&, std::string_view) {
            return std::vector<std::string>{"val1", "val2"};
        });

    level1.validArgs({"arg1", "arg2"});
    level2.validArgs({"arg3", "arg4"});
    level3.validArgs({"arg5", "arg6"});

    root.addCommand(std::move(level1));
}

} // namespace

int main() {
    testEnableColorCompletion();
    testEnableCompletionMethods();
    testCompletionWithSubcommands();
    testCompletionWithValidArgs();
    testPersistentFlagCompletion();
    testCompletionWithTraverse();
    testCompletionWithColor();
    testCompletionDirectives();
    testComplexCompletionSetup();

    std::cout << "ok\n";
    return 0;
}
