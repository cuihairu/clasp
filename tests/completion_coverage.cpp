// Coverage tests for completion internals (shell script generation,
// completion entries, value completion).
#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <typeinfo>
#include <vector>
#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>

// Debug-CRT violations that skip _CrtDbgReport still funnel through the
// invalid-parameter handler; print the details so the ctest log shows them.
void claspReportInvalidParameter(const wchar_t* expr, const wchar_t* func,
                                 const wchar_t* file, unsigned line, uintptr_t) {
    std::fwprintf(stderr, L"INVALID_PARAMETER expr=%ls func=%ls file=%ls line=%u\n",
                  expr ? expr : L"?", func ? func : L"?", file ? file : L"?", line);
    std::fflush(stderr);
}
#endif

#include "clasp/command.hpp"

namespace {

int g_failures = 0;

void expect(bool cond, const char* label) {
    std::cout << label << ": " << (cond ? "ok" : "fail") << std::endl;
    if (!cond) ++g_failures;
}

bool contains(const std::string& hay, const std::string& needle) {
    return hay.find(needle) != std::string::npos;
}

int countOf(const std::string& hay, const std::string& needle) {
    int n = 0;
    std::size_t pos = 0;
    while ((pos = hay.find(needle, pos)) != std::string::npos) {
        ++n;
        pos += needle.size();
    }
    return n;
}

int runArgs(clasp::Command& root, std::vector<std::string> args) {
    std::vector<std::string> argv;
    argv.push_back(root.name());
    for (auto& a : args) argv.push_back(std::move(a));
    std::vector<char*> ptrs;
    for (auto& a : argv) ptrs.push_back(a.data());
    return root.run(static_cast<int>(ptrs.size()), ptrs.data());
}

// Runs the hidden completion command in-process; output lands in `sink`.
std::string complete(clasp::Command& root, std::ostringstream& sink, std::vector<std::string> args, bool withDesc) {
    sink.str("");
    args.insert(args.begin(), withDesc ? "__complete" : "__completeNoDesc");
    runArgs(root, std::move(args));
    return sink.str();
}

// Custom Value with a non-empty type name (for help rendering).
struct TypedValue final : clasp::Value {
    std::string value = "seed";
    std::string type() const override { return "counting"; }
    std::string string() const override { return value; }
    std::optional<std::string> set(std::string_view v) override {
        value = std::string(v);
        return std::nullopt;
    }
};

// Custom Value whose type() is empty: help rendering must fall through.
struct EmptyTypeValue final : clasp::Value {
    std::string value = "seed";
    std::string type() const override { return ""; }
    std::string string() const override { return value; }
    std::optional<std::string> set(std::string_view v) override {
        value = std::string(v);
        return std::nullopt;
    }
};

// ------------------------------------------------------------------
// resolvedCompletionConfig(): fall-through when no override exists.
// ------------------------------------------------------------------
void testResolvedCompletionConfigFallback() {
    clasp::Command bare("bare-app", "Bare");
    std::ostringstream bash, zsh, fish, ps;
    bare.printCompletionBash(bash);
    bare.printCompletionZsh(zsh);
    bare.printCompletionFish(fish);
    bare.printCompletionPowerShell(ps);
    expect(contains(bash.str(), "_bare_app_complete() {"), "config fallback: static bash");
    expect(contains(zsh.str(), "#compdef bare-app"), "config fallback: zsh header");
    expect(contains(fish.str(), "__clasp_bare_app_fish_complete"), "config fallback: default fish is dynamic");
    expect(contains(ps.str(), "Register-ArgumentCompleter"), "config fallback: powershell emitted");
}

// ------------------------------------------------------------------
// printUsageTo(): usage template rendering with {{.CommandPath}}.
// ------------------------------------------------------------------
void testUsageTemplateCommandPathKey() {
    clasp::Command root("pathapp", "Template root");
    clasp::Command sub("leaf", "Leaf");
    root.addCommand(std::move(sub));
    root.setUsageTemplate("Usage:{{.UsageLine}}|Path:{{.CommandPath}}|X:{{.BogusKey}}");
    std::ostringstream md;
    root.printMarkdown(md);
    expect(contains(md.str(), "Path:pathapp"), "usage template renders CommandPath");
    expect(contains(md.str(), "Usage:Usage: pathapp [command] [flags]\n|Path:pathapp|"),
           "usage template renders UsageLine");
}

// ------------------------------------------------------------------
// flagTypeForHelp(): value bindings and annotation-driven types.
// ------------------------------------------------------------------
void testFlagTypeForHelp() {
    clasp::Command root("helpapp", "Help flags");
    TypedValue typed;
    EmptyTypeValue emptyTyped;

    // Binding registered on the long name.
    root.withValueFlag("--level", "-l", "Level", "Level value", typed);
    // Binding registered ONLY on the short name (long lookup misses).
    root.withFlag("--name", "-n", "name", "Name", std::string("x"));
    root.bindFlagValue("-n", typed);
    // Bound value whose type() is empty -> falls through to annotations.
    root.withValueFlag("--ghost", "-g", "Ghost", "Ghost value", emptyTyped);
    // Plain flags without any binding.
    root.withFlag("--plain", "", "plain", "Plain string", std::string(""));
    root.withFlag("--num", "", "num", "Number", 3);
    root.withFlag("--vbool", "", "vbool", "Bool", true);

    // Annotation-driven types (all need a non-bool default). Every accepted
    // spelling ("true"/"1"/"yes") takes a different short-circuit branch.
    root.withFlag("--fb", "", "fb", "bytes true", std::string(""));
    root.markFlagAnnotation("--fb", "bytes", "true");
    root.withFlag("--fb1", "", "fb1", "bytes 1", std::string(""));
    root.markFlagAnnotation("--fb1", "bytes", "1");
    root.withFlag("--fby", "", "fby", "bytes yes", std::string(""));
    root.markFlagAnnotation("--fby", "bytes", "yes");
    root.withFlag("--fbno", "", "fbno", "bytes no", std::string(""));
    root.markFlagAnnotation("--fbno", "bytes", "no");
    root.withFlag("--fc", "", "fc", "count true", std::string(""));
    root.markFlagAnnotation("--fc", "count", "true");
    root.withFlag("--fc1", "", "fc1", "count 1", std::string(""));
    root.markFlagAnnotation("--fc1", "count", "1");
    root.withFlag("--fcy", "", "fcy", "count yes", std::string(""));
    root.markFlagAnnotation("--fcy", "count", "yes");
    root.withFlag("--fi", "", "fi", "ip true", std::string(""));
    root.markFlagAnnotation("--fi", "ip", "true");
    root.withFlag("--fi1", "", "fi1", "ip 1", std::string(""));
    root.markFlagAnnotation("--fi1", "ip", "1");
    root.withFlag("--fiy", "", "fiy", "ip yes", std::string(""));
    root.markFlagAnnotation("--fiy", "ip", "yes");
    root.withFlag("--fm", "", "fm", "ipmask true", std::string(""));
    root.markFlagAnnotation("--fm", "ipmask", "true");
    root.withFlag("--fm1", "", "fm1", "ipmask 1", std::string(""));
    root.markFlagAnnotation("--fm1", "ipmask", "1");
    root.withFlag("--fmy", "", "fmy", "ipmask yes", std::string(""));
    root.markFlagAnnotation("--fmy", "ipmask", "yes");
    root.withFlag("--fcd", "", "fcd", "cidr true", std::string(""));
    root.markFlagAnnotation("--fcd", "cidr", "true");
    root.withFlag("--fcd1", "", "fcd1", "cidr 1", std::string(""));
    root.markFlagAnnotation("--fcd1", "cidr", "1");
    root.withFlag("--fcdy", "", "fcdy", "cidr yes", std::string(""));
    root.markFlagAnnotation("--fcdy", "cidr", "yes");
    root.withFlag("--fn", "", "fn", "ipnet true", std::string(""));
    root.markFlagAnnotation("--fn", "ipnet", "true");
    root.withFlag("--fn1", "", "fn1", "ipnet 1", std::string(""));
    root.markFlagAnnotation("--fn1", "ipnet", "1");
    root.withFlag("--fny", "", "fny", "ipnet yes", std::string(""));
    root.markFlagAnnotation("--fny", "ipnet", "yes");
    root.withFlag("--fu", "", "fu", "url true", std::string(""));
    root.markFlagAnnotation("--fu", "url", "true");
    root.withFlag("--fu1", "", "fu1", "url 1", std::string(""));
    root.markFlagAnnotation("--fu1", "url", "1");
    root.withFlag("--fuy", "", "fuy", "url yes", std::string(""));
    root.markFlagAnnotation("--fuy", "url", "yes");
    // Rejected spellings: annotation present but value not accepted.
    root.withFlag("--fcno", "", "fcno", "count no", std::string(""));
    root.markFlagAnnotation("--fcno", "count", "no");
    root.withFlag("--fino", "", "fino", "ip no", std::string(""));
    root.markFlagAnnotation("--fino", "ip", "no");
    root.withFlag("--fmno", "", "fmno", "ipmask no", std::string(""));
    root.markFlagAnnotation("--fmno", "ipmask", "no");
    root.withFlag("--fcdno", "", "fcdno", "cidr no", std::string(""));
    root.markFlagAnnotation("--fcdno", "cidr", "no");
    root.withFlag("--fnno", "", "fnno", "ipnet no", std::string(""));
    root.markFlagAnnotation("--fnno", "ipnet", "no");
    root.withFlag("--funo", "", "funo", "url no", std::string(""));
    root.markFlagAnnotation("--funo", "url", "no");

    // Short-only flag: resolvedFlagValueBinding sees an empty long name.
    root.withFlag("", "-s", "shortonly", "Short only", std::string("v"));

    std::ostringstream out;
    root.setOut(out);
    root.printHelp();

    expect(contains(out.str(), "counting"), "flagTypeForHelp: bound value type shown");
    expect(contains(out.str(), "string"), "flagTypeForHelp: string fallback type");
    expect(contains(out.str(), "--num int - Number (default: 3)"), "flagTypeForHelp: int type");
    expect(contains(out.str(), "bytes"), "flagTypeForHelp: bytes annotation type");
    expect(contains(out.str(), "ipmask"), "flagTypeForHelp: ipmask annotation type");
    expect(contains(out.str(), "cidr"), "flagTypeForHelp: cidr annotation type");
    expect(contains(out.str(), "ipnet"), "flagTypeForHelp: ipnet annotation type");
    expect(contains(out.str(), " url"), "flagTypeForHelp: url annotation type");
    expect(contains(out.str(), "-s,  string"), "flagTypeForHelp: short-only flag rendered");
}

// ------------------------------------------------------------------
// Value completion for --flag=prefix and "--flag <prefix>".
// ------------------------------------------------------------------
clasp::Command makeValueCompletionRoot(std::ostringstream& out) {
    clasp::Command root("vapp", "Value completion");
    root.setOut(out);
    root.enableCompletion();
    root.version("1.0.0");

    clasp::Command paint("paint", "Paint");
    paint.withFlag("--color", "-c", "color", "Color to use", std::string(""));
    paint.registerFlagCompletion("--color",
        [](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&, std::string_view) {
            return std::vector<std::string>{"red", "green", ":4"};
        });
    paint.withFlag("--file", "-f", "file", "File", std::string(""));
    paint.markFlagFilename("--file", {"txt", "md"});
    paint.withFlag("--plain", "", "plain", "No completion func", std::string(""));
    paint.withFlag("--fast", "", "fast", "Bool flag", true);
    paint.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
    root.addCommand(std::move(paint));
    return root;
}

void testValueCompletionEqualSign() {
    std::ostringstream sink;
    auto root = makeValueCompletionRoot(sink);

    auto r = complete(root, sink, {"paint", "--color=r"}, false);
    expect(contains(r, "--color=red"), "eq completion: prefix match");
    expect(!contains(r, "green"), "eq completion: prefix filters others");
    expect(contains(r, ":4"), "eq completion: directive line");

    auto r2 = complete(root, sink, {"paint", "-c="}, false);
    expect(contains(r2, "-c=red") && contains(r2, "-c=green"), "eq completion: short name resolves to long");

    auto r3 = complete(root, sink, {"paint", "--file="}, false);
    expect(contains(r3, "txt") && contains(r3, "md"), "eq completion: metadata ext list");
    expect(!contains(r3, "--file=txt"), "eq completion: ext list is raw metadata");

    auto r4 = complete(root, sink, {"paint", "--nope=x"}, false);
    expect(contains(r4, ":4") && countOf(r4, "\n") == 1, "eq completion: unknown flag yields directive only");

    auto r5 = complete(root, sink, {"paint", "--plain="}, false);
    expect(contains(r5, ":4") && countOf(r5, "\n") == 1, "eq completion: flag without func yields directive only");
}

void testValueCompletionSeparateArg() {
    std::ostringstream sink;
    auto root = makeValueCompletionRoot(sink);

    auto r = complete(root, sink, {"paint", "--color", "r"}, false);
    expect(contains(r, "red"), "separate completion: func candidates");
    expect(!contains(r, "green"), "separate completion: prefix filter");
    expect(contains(r, ":4"), "separate completion: directive");

    auto r2 = complete(root, sink, {"paint", "-c", ""}, false);
    expect(contains(r2, "red") && contains(r2, "green"), "separate completion: short flag empty prefix");

    auto r3 = complete(root, sink, {"paint", "--fast", ""}, false);
    expect(!contains(r3, "red"), "separate completion: bool flag falls through");

    auto r4 = complete(root, sink, {"paint", "--plain", ""}, false);
    expect(contains(r4, ":4") && countOf(r4, "\n") == 1, "separate completion: no func yields directive only");

    auto r5 = complete(root, sink, {"paint", "--nope", ""}, false);
    expect(!contains(r5, "red"), "separate completion: unknown flag falls through");
}

// ------------------------------------------------------------------
// Flag name completion: hidden flags, long-only/short-only, version.
// ------------------------------------------------------------------
void testFlagNameCompletion() {
    std::ostringstream sink;
    clasp::Command root("fapp", "Flag name completion");
    root.setOut(sink);
    root.enableCompletion();
    root.version("1.0.0");
    root.withFlag("--flag", "-f", "flag", "Flag description", std::string(""));
    root.withFlag("--longonly", "", "longonly", "Long only flag", std::string(""));
    root.withFlag("", "-s", "shortonly", "Short only flag", std::string(""));
    root.withFlag("--nodesc", "", "nodesc", "", std::string("")); // empty description
    root.withFlag("--hid", "", "hid", "Hidden flag", std::string(""));
    // Count-annotated flags: flagInfoLocal treats counts as booleans.
    root.withFlag("--lcount", "", "lcount", "Count true", std::string(""));
    root.markFlagAnnotation("--lcount", "count", "true");
    root.withFlag("--lcount1", "", "lcount1", "Count 1", std::string(""));
    root.markFlagAnnotation("--lcount1", "count", "1");
    root.withFlag("--lcounty", "", "lcounty", "Count yes", std::string(""));
    root.markFlagAnnotation("--lcounty", "count", "yes");
    root.withFlag("--lcountno", "", "lcountno", "Count no", std::string(""));
    root.markFlagAnnotation("--lcountno", "count", "no");
    root.markFlagHidden("--hid");
    clasp::Command sub("run", "Run");
    sub.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
    root.addCommand(std::move(sub));

    auto r = complete(root, sink, {"--"}, true);
    expect(contains(r, "--flag\tFlag description"), "flag name completion: long with description");
    expect(contains(r, "--longonly"), "flag name completion: long-only flag");
    expect(!contains(r, "--hid"), "flag name completion: hidden flag skipped");
    expect(contains(r, "--help\tHelp for this command"), "flag name completion: help flag");
    expect(contains(r, "--version\tVersion for this command"), "flag name completion: version flag");
    expect(!contains(r, "-h\t"), "flag name completion: short names need dash prefix");

    auto rd = complete(root, sink, {"-"}, true);
    expect(contains(rd, "-f\tFlag description"), "flag name completion: short with description");
    expect(contains(rd, "-h\tHelp for this command"), "flag name completion: -h flag");
    expect(contains(rd, "-s\tShort only flag"), "flag name completion: short-only flag listed");

    auto r2 = complete(root, sink, {"--v"}, true);
    expect(contains(r2, "--version") && !contains(r2, "--flag"), "flag name completion: prefix --v");

    auto r3 = complete(root, sink, {"-"}, false);
    expect(contains(r3, "--flag\n") && contains(r3, "-f\n"), "flag name completion: no-desc mode omits descriptions");
    expect(!contains(r3, "\t"), "flag name completion: no tabs in no-desc mode");

    auto r4 = complete(root, sink, {"-", ""}, false);
    expect(contains(r4, ":4"), "flag name completion: lone dash is positional");

    // Short-only flag in a --flag= token: long-name resolution skips it.
    complete(root, sink, {"-s="}, false);

    // Local flag-info lookups for help/version/count spellings.
    complete(root, sink, {"--help", ""}, false);
    complete(root, sink, {"-h", ""}, false);
    complete(root, sink, {"--version", ""}, false);
    complete(root, sink, {"--lcount", ""}, false);
    complete(root, sink, {"--lcount1", ""}, false);
    complete(root, sink, {"--lcounty", ""}, false);
    complete(root, sink, {"--lcountno", ""}, false);

    // Execution-side flag-info lookups need an action on the root.
    root.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
    expect(runArgs(root, {"--lcount"}) == 0, "flag name completion: run count=true");
    expect(runArgs(root, {"--lcount1"}) == 0, "flag name completion: run count=1");
    expect(runArgs(root, {"--lcounty"}) == 0, "flag name completion: run count=yes");
    expect(runArgs(root, {"--lcountno", "x"}) == 0, "flag name completion: run count=no");

    // A long flag name exercises the heap-allocation branch of inlined string
    // copies inside candidate collection.
    root.withFlag("--verylongflagname", "", "verylongflagname", "A long flag description here", std::string(""));
    auto r5 = complete(root, sink, {"--verylong"}, true);
    expect(contains(r5, "--verylongflagname\tA long flag description here"),
           "flag name completion: long name and description");

    std::ostringstream sink2;
    clasp::Command root2("nvapp", "No version");
    root2.setOut(sink2);
    root2.enableCompletion();
    root2.withFlag("--flag", "", "flag", "Flag", std::string(""));
    auto r6 = complete(root2, sink2, {"--"}, false);
    expect(!contains(r6, "--version"), "flag name completion: no version flag without version");
}

// ------------------------------------------------------------------
// Subcommand completion: dedup, help/version entries.
// ------------------------------------------------------------------
void testSubcommandCompletion() {
    std::ostringstream sink;
    clasp::Command root("sapp", "Sub command completion");
    root.setOut(sink);
    root.enableCompletion();
    root.version("1.0.0");

    clasp::Command run("run", "Run things");
    run.addAlias("r");
    run.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
    root.addCommand(std::move(run));

    clasp::Command rush("rush", "Rush things");
    rush.addAlias("run");  // collides with the "run" subcommand name.
    rush.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
    root.addCommand(std::move(rush));

    clasp::Command hid("hidsub", "Hidden");
    hid.hidden(true);
    root.addCommand(std::move(hid));

    // A description longer than the SSO limit exercises the heap-allocation
    // branch of the inlined string copy in candidate collection.
    clasp::Command longish("longish", "A description longer than fifteen chars");
    longish.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
    root.addCommand(std::move(longish));

    auto r = complete(root, sink, {""}, false);
    expect(countOf(r, "run\n") == 1, "subcommand completion: duplicate invocation deduped");
    expect(contains(r, "rush") && contains(r, "r\n"), "subcommand completion: aliases listed");
    expect(contains(r, "help") && contains(r, "version"), "subcommand completion: root help/version");
    expect(!contains(r, "hidsub"), "subcommand completion: hidden sub skipped");

    auto rd = complete(root, sink, {""}, true);
    expect(contains(rd, "run\tRun things"), "subcommand completion: descriptions shown");
    expect(contains(rd, "longish\tA description longer than fifteen chars"),
           "subcommand completion: long description");

    auto rv = complete(root, sink, {"v"}, false);
    expect(contains(rv, "version") && !contains(rv, "run"), "subcommand completion: v prefix");

    auto rz = complete(root, sink, {"zz"}, false);
    expect(contains(rz, ":4") && countOf(rz, "\n") == 1, "subcommand completion: unmatched prefix");

    auto rsub = complete(root, sink, {"run", ""}, false);
    expect(contains(rsub, ":4") && countOf(rsub, "\n") == 1, "subcommand completion: leaf level has no subs");

    // Root with help command disabled: no "help" entry.
    std::ostringstream sink2;
    clasp::Command root2("nhapp", "No help command");
    root2.setOut(sink2);
    root2.disableHelpCommand();
    root2.enableCompletion();
    clasp::Command leaf("leaf", "Leaf");
    root2.addCommand(std::move(leaf));
    auto r2 = complete(root2, sink2, {""}, false);
    expect(contains(r2, "leaf") && !contains(r2, "help"), "subcommand completion: help disabled");
}

// ------------------------------------------------------------------
// validArgs completion with directive override entries.
// ------------------------------------------------------------------
void testValidArgsDirectiveOverride() {
    std::ostringstream sink;
    clasp::Command root("vaapp", "Valid args");
    root.setOut(sink);
    root.enableCompletion();

    clasp::Command sub("pick", "Pick");
    sub.validArgs({"alpha", ":16", ":abc", ":.", ":", "zz"});
    sub.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
    root.addCommand(std::move(sub));

    auto r = complete(root, sink, {"pick", ""}, false);
    expect(contains(r, "alpha") && contains(r, "zz"), "validArgs completion: candidates kept");
    expect(contains(r, ":16\n"), "validArgs completion: directive entry overrides");

    auto r2 = complete(root, sink, {"pick", "a"}, false);
    expect(contains(r2, "alpha") && !contains(r2, "zz\n"), "validArgs completion: prefix filter");
}

// ------------------------------------------------------------------
// suggestCommands(): alias prefix matching and top-3 cutoff.
// ------------------------------------------------------------------
void testSuggestCommands() {
    std::ostringstream out, err;
    clasp::Command root("sgapp", "Suggestions");
    root.setOut(out);
    root.setErr(err);

    auto add = [&root](const std::string& name, std::vector<std::string> aliases) {
        clasp::Command c(name, name);
        c.aliases(std::move(aliases));
        root.addCommand(std::move(c));
    };
    add("foo", {"fuzzy"});   // alias with token as prefix.
    add("bar", {"baz"});     // alias matched only via edit distance.
    add("aa1", {});
    add("aa2", {});
    add("aa3", {});
    add("aa4", {});
    add("zzz", {});

    expect(runArgs(root, {"fu"}) == 1, "suggest: unknown command exits 1");
    expect(contains(err.str(), "foo"), "suggest: alias prefix match suggested");
    expect(!contains(err.str(), "aa1"), "suggest: distant commands not suggested");

    err.str("");
    expect(runArgs(root, {"bat"}) == 1, "suggest: run for distance case");
    expect(contains(err.str(), "bar"), "suggest: alias edit distance match");

    err.str("");
    expect(runArgs(root, {"aaX"}) == 1, "suggest: run for cutoff case");
    expect(contains(err.str(), "aa1") && contains(err.str(), "aa2") && contains(err.str(), "aa3"),
           "suggest: first three suggested");
    expect(!contains(err.str(), "aa4"), "suggest: fourth candidate cut off");
}

// ------------------------------------------------------------------
// resolveForExecution/resolveForCompletion: short flag grouping.
// ------------------------------------------------------------------
void testShortFlagGroupingResolution() {
    std::ostringstream sink;
    clasp::Command root("gapp", "Grouping");
    root.setOut(sink);
    root.enableCompletion();
    root.shortFlagGrouping(true);
    root.withPersistentFlag("--aa", "-a", "aa", "A bool", true);
    root.withPersistentFlag("--bb", "-b", "bb", "B bool", true);
    root.withPersistentFlag("--oo", "-o", "oo", "O value", std::string(""));
    root.markPersistentFlagNoOptDefaultValue("--oo", "auto");
    root.withPersistentFlag("--vv", "-v", "vv", "V value", std::string(""));
    clasp::Command sub("sub", "Sub");
    sub.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
    root.addCommand(std::move(sub));

    complete(root, sink, {"-ab", ""}, false);          // all bools: consume nothing.
    complete(root, sink, {"-ax", ""}, false);          // unknown group member.
    complete(root, sink, {"-ao", "sub", ""}, false);   // noOptDefault + next is subcommand.
    complete(root, sink, {"-ao", ""}, false);          // noOptDefault at end of words.
    complete(root, sink, {"-ao", "zzz", ""}, false);   // next word is not a subcommand.
    complete(root, sink, {"-av", "hello", ""}, false); // value flag consumes next word.
    complete(root, sink, {"-av", ""}, false);          // value flag with no next word.
    complete(root, sink, {"-avb", ""}, false);         // value flag in the middle of the group.
    complete(root, sink, {"-aob", ""}, false);         // noOptDefault flag in the middle of the group.
    complete(root, sink, {"--oo", "sub", ""}, false);  // long noOptDefault before subcommand.
    complete(root, sink, {"--oo", ""}, false);         // long noOptDefault at end of words.
    complete(root, sink, {"--oo", "zzz", ""}, false);  // next word is not a subcommand.
    complete(root, sink, {"--vv", "hello", ""}, false);// long value consumes next word.

    expect(runArgs(root, {"-ab"}) == 0, "group execution: bool-only group");
    expect(runArgs(root, {"-ao", "sub"}) == 0, "group execution: noOptDefault followed by subcommand");
    expect(runArgs(root, {"-ao"}) == 0, "group execution: noOptDefault at end of argv");
    expect(runArgs(root, {"-ao", "zzz"}) == 0, "group execution: noOptDefault before non-subcommand");
    expect(runArgs(root, {"-av", "hello"}) == 0, "group execution: value consumed from next arg");
    expect(runArgs(root, {"-avb", "x"}) == 1, "group execution: mid-group value leaves 'x' as positional");
    expect(runArgs(root, {"-aob", "x"}) == 1, "group execution: mid-group noOptDefault leaves 'x' as positional");
    expect(runArgs(root, {"--vv", "hello"}) == 0, "group execution: long value consumed");

    expect(runArgs(root, {"-ab"}) == 0, "group execution: bool-only group");
    expect(runArgs(root, {"-ao", "sub"}) == 0, "group execution: noOptDefault followed by subcommand");
    expect(runArgs(root, {"-ao"}) == 0, "group execution: noOptDefault at end of argv");
    expect(runArgs(root, {"-ao", "zzz"}) == 0, "group execution: noOptDefault before non-subcommand");
    expect(runArgs(root, {"-ao", "sux"}) == 0, "group execution: same-length non-subcommand word");
    expect(runArgs(root, {"--oo", "sub"}) == 0, "group execution: long noOptDefault before subcommand");
    expect(runArgs(root, {"--oo"}) == 0, "group execution: long noOptDefault at end of argv");
    expect(runArgs(root, {"--oo", "zzz"}) == 0, "group execution: long noOptDefault before non-subcommand");
    expect(runArgs(root, {"--oo", "sux"}) == 0, "group execution: long flag with same-length non-subcommand");

    // Grouping disabled: tokens stay untouched.
    std::ostringstream sink2;
    clasp::Command root2("ngapp", "No grouping");
    root2.setOut(sink2);
    root2.enableCompletion();
    root2.shortFlagGrouping(false);
    root2.withFlag("--aa", "-a", "aa", "A bool", true);
    root2.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
    complete(root2, sink2, {"-ab", ""}, false);
    expect(runArgs(root2, {"-a"}) == 0, "grouping disabled: plain parse");
}

// ------------------------------------------------------------------
// Bool negation and --flag=value tokens in resolution.
// ------------------------------------------------------------------
void testNegationAndEqResolution() {
    std::ostringstream sink;
    clasp::Command root("napp", "Negation");
    root.setOut(sink);
    root.enableCompletion();
    root.withFlag("--debug", "-d", "debug", "Debug", true);
    root.withFlag("--opt", "", "opt", "Opt", std::string(""));
    root.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });

    complete(root, sink, {"--no-debug", "x", ""}, false);  // negated bool: skip nothing.
    complete(root, sink, {"--debug=1", ""}, false);        // --flag=value token.
    complete(root, sink, {"--debug", ""}, false);          // plain bool.
    complete(root, sink, {"--opt", "v", ""}, false);       // value flag consumes next.

    expect(runArgs(root, {"--no-debug"}) == 0, "negation execution: --no-debug");
    expect(runArgs(root, {"--debug=true"}) == 0, "negation execution: --debug=value");

    // Negation disabled: --no-debug treated as an ordinary (unknown) key.
    std::ostringstream sink2;
    clasp::Command root2("nnapp", "No negation");
    root2.setOut(sink2);
    root2.enableCompletion();
    root2.boolNegation(false);
    root2.withFlag("--debug", "-d", "debug", "Debug", true);
    root2.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
    complete(root2, sink2, {"--no-debug", ""}, false);
    expect(runArgs(root2, {"--debug"}) == 0, "negation disabled: plain bool parse");
}

// ------------------------------------------------------------------
// resolveForCompletion: "--" separator and plain positionals.
// ------------------------------------------------------------------
void testDoubleDashResolution() {
    std::ostringstream sink;
    clasp::Command root("dapp", "Double dash");
    root.setOut(sink);
    root.enableCompletion();
    root.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
    clasp::Command sub("sub", "Sub");
    sub.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
    root.addCommand(std::move(sub));

    auto r = complete(root, sink, {"--", ""}, false);
    expect(contains(r, "sub"), "double dash completion: completes after --");
    complete(root, sink, {"--", "x"}, false);  // break on the token after --.
    complete(root, sink, {"--", "-a", ""}, false); // flag token after -- breaks the walk.
    complete(root, sink, {"--", "--", ""}, false); // second -- is a positional token.
    complete(root, sink, {"x"}, false);        // unknown token ends the walk.
    complete(root, sink, {"-"}, false);        // single dash is not a flag token.
    complete(root, sink, {"sub", "--zz=1"}, false); // value completion against a flagless command.
    expect(contains(complete(root, sink, {""}, false), "sub"), "double dash completion: plain completion");

    expect(runArgs(root, {"--", "pos"}) == 0, "double dash execution: positional passthrough");
    expect(runArgs(root, {"sub", "--", "x"}) == 0, "double dash execution: sub then --");
}

// ------------------------------------------------------------------
// resolveForExecution: help/version token handling.
// ------------------------------------------------------------------
void testExecutionHelpVersionResolution() {
    std::ostringstream out, err;
    clasp::Command root("happ", "Help resolution");
    root.setOut(out);
    root.setErr(err);
    root.version("3.2.1");

    clasp::Command sub1("one", "One");
    sub1.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>& args) {
        return static_cast<int>(args.size()) + 10;
    });
    clasp::Command sub2("two", "Two");
    sub1.addCommand(std::move(sub2));
    root.addCommand(std::move(sub1));

    expect(runArgs(root, {"help"}) == 0, "help resolution: root help runs");
    expect(contains(out.str(), "happ"), "help resolution: root help output");
    out.str("");

    expect(runArgs(root, {"help", "one", "two"}) == 0, "help resolution: nested path");
    expect(contains(out.str(), "two"), "help resolution: nested help output");
    out.str("");

    expect(runArgs(root, {"help", "one", "--", "x"}) == 0, "help resolution: -- ends path");
    out.str("");
    expect(runArgs(root, {"help", "one", "-q"}) == 0, "help resolution: flag ends path");
    out.str("");
    expect(runArgs(root, {"help", "nope"}) == 1, "help resolution: unknown help path rejected");

    expect(runArgs(root, {"version"}) == 0, "help resolution: version prints");
    expect(contains(out.str(), "3.2.1"), "help resolution: version output");
    out.str("");

    // "help"/"version" below the root are plain positionals.
    expect(runArgs(root, {"one", "help"}) == 11, "help resolution: help is positional in subcommand");
    expect(runArgs(root, {"one", "version"}) == 11, "help resolution: version is positional in subcommand");

    // Disabled help command: "help" falls through to the root action.
    std::ostringstream out2, err2;
    clasp::Command root2("nh", "No help");
    root2.setOut(out2);
    root2.setErr(err2);
    root2.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>& args) {
        return args.empty() ? 0 : 42;
    });
    root2.disableHelpCommand();
    expect(runArgs(root2, {"help"}) == 42, "help resolution: disabled help command passes through");
}

// ------------------------------------------------------------------
// completionEntries()/collectCompletionEntries(): static scripts.
// ------------------------------------------------------------------
void testCompletionEntriesStaticScripts() {
    clasp::Command::CompletionConfig staticCfg;
    staticCfg.addCompletionCommand = false;
    staticCfg.addCompleteCommands = false;

    clasp::Command root("staticapp", "Static scripts");
    root.enableCompletion(staticCfg);
    root.version("2.0");
    root.withFlag("--gflag", "-G", "gflag", "Root global flag", std::string(""));
    root.withFlag("", "-R", "gshort", "Root short-only flag", std::string(""));
    root.withFlag("--gndesc", "", "gndesc", "", std::string(""));
    clasp::Command sub("sub", "Sub cmd");
    sub.addAlias("s");
    sub.withFlag("--both", "-b", "both", "Both names", std::string(""));
    sub.withFlag("--longonly", "", "longonly", "Long only", std::string(""));
    sub.withFlag("", "-x", "shortonly", "Short only", std::string(""));
    sub.withFlag("", "", "nameless", "Nameless flag", std::string(""));
    sub.withFlag("--quoted", "-q", "quoted", "Say \"hi\"", std::string(""));
    sub.withFlag("--sndesc", "", "sndesc", "", std::string(""));
    root.addCommand(std::move(sub));

    std::ostringstream bash, zsh, fish, ps;
    root.printCompletionBash(bash);
    root.printCompletionZsh(zsh);
    root.printCompletionFish(fish);
    root.printCompletionPowerShell(ps);

    expect(contains(bash.str(), "\"staticapp sub\"|\"staticapp s\""), "static bash: alias case label");
    expect(contains(bash.str(), "COMPREPLY"), "static bash: COMPREPLY emitted");
    expect(contains(zsh.str(), "#compdef staticapp"), "static zsh: header");
    expect(contains(fish.str(), "\"sub s help version\""), "static fish: root subcommand list");
    expect(contains(fish.str(), "-l version -d \"Version for this command\""), "static fish: version flag line");
    expect(contains(fish.str(), "-l longonly"), "static fish: long-only flag");
    expect(contains(fish.str(), "-s x"), "static fish: short-only flag");
    expect(contains(fish.str(), "-l both -s b"), "static fish: both-name flag");
    expect(contains(fish.str(), "-d \"Say \\\"hi\\\"\""), "static fish: escaped quotes in description");
    expect(contains(fish.str(), "__fish_seen_subcommand_from sub s"), "static fish: sub flag condition");
    expect(contains(ps.str(), "$__claspSubs['staticapp sub']"), "static powershell: sub map entry");
    expect(contains(ps.str(), "$__claspFlags['staticapp sub']"), "static powershell: flag map entry");
    expect(contains(ps.str(), "'sub', 's', 'help', 'version'"), "static powershell: quoted list");
    expect(contains(ps.str(), "$__claspSubs['staticapp help']"), "static powershell: help entry");
    expect(contains(ps.str(), "$__claspSubs['staticapp version']"), "static powershell: version entry");

    // Without a version: no version entries anywhere.
    clasp::Command root2("nover", "No version");
    root2.enableCompletion(staticCfg);
    clasp::Command sub2("leaf", "Leaf");
    root2.addCommand(std::move(sub2));
    std::ostringstream bash2, fish2, ps2;
    root2.printCompletionBash(bash2);
    root2.printCompletionFish(fish2);
    root2.printCompletionPowerShell(ps2);
    expect(!contains(fish2.str(), "-l version"), "static fish: no version line without version");
    expect(!contains(ps2.str(), "'nover version'"), "static powershell: no version entry");
    expect(!contains(bash2.str(), "\"nover version\""), "static bash: no version case label");

    // With the help command disabled: no help entries.
    clasp::Command root3("nohelp", "No help");
    root3.disableHelpCommand();
    root3.enableCompletion(staticCfg);
    clasp::Command sub3("leaf", "Leaf");
    sub3.addAlias("l");
    root3.addCommand(std::move(sub3));
    std::ostringstream bash3, fish3, ps3;
    root3.printCompletionBash(bash3);
    root3.printCompletionFish(fish3);
    root3.printCompletionPowerShell(ps3);
    expect(contains(fish3.str(), "\"leaf l\""), "static fish: no help in subcommand list");
    expect(!contains(fish3.str(), "\"leaf l help\""), "static fish: help command disabled");
    expect(!contains(bash3.str(), "\"nohelp help\""), "static bash: no help case label");
    expect(!contains(ps3.str(), "'nohelp help'"), "static powershell: no help entry");
}

// ------------------------------------------------------------------
// Dynamic shell scripts (addCompleteCommands / complete command names).
// ------------------------------------------------------------------
void testDynamicShellScripts() {
    clasp::Command root("dynapp", "Dynamic scripts");
    root.enableCompletion();
    clasp::Command sub("leaf", "Leaf");
    sub.withFlag("--opt", "-o", "opt", "Opt", std::string(""));
    root.addCommand(std::move(sub));

    std::ostringstream bash, zsh, fish, ps;
    root.printCompletionBash(bash);
    root.printCompletionZsh(zsh);
    root.printCompletionFish(fish);
    root.printCompletionPowerShell(ps);
    expect(contains(bash.str(), "__completeNoDesc"), "dynamic bash: calls complete command");
    expect(contains(bash.str(), "ShellCompDirectiveError"), "dynamic bash: directive handling");
    expect(contains(zsh.str(), "#compdef dynapp"), "dynamic zsh: header");
    expect(contains(fish.str(), "__clasp_dynapp_fish_complete"), "dynamic fish: helper function");
    expect(contains(fish.str(), " __complete "), "dynamic fish: uses desc command");
    expect(contains(ps.str(), "$__claspCompleteCmd = '__completeNoDesc'"), "dynamic powershell: complete command");
    expect(contains(ps.str(), "Register-ArgumentCompleter"), "dynamic powershell: registration");

    // Only the desc command name configured.
    clasp::Command::CompletionConfig descOnly;
    descOnly.completeNoDescCommandName = "";
    clasp::Command root2("descapp", "Desc only");
    root2.enableCompletion(descOnly);
    std::ostringstream fish2;
    root2.printCompletionFish(fish2);
    expect(contains(fish2.str(), " __complete "), "dynamic fish: desc-only config");

    // Only the no-desc command name configured.
    clasp::Command::CompletionConfig noDescOnly;
    noDescOnly.completeCommandName = "";
    clasp::Command root3("ndapp", "NoDesc only");
    root3.enableCompletion(noDescOnly);
    std::ostringstream fish3;
    root3.printCompletionFish(fish3);
    expect(contains(fish3.str(), " __completeNoDesc "), "dynamic fish: nodesc-only config");

    // Both names empty: fish falls back to the static script.
    clasp::Command::CompletionConfig noNames;
    noNames.completeCommandName = "";
    noNames.completeNoDescCommandName = "";
    clasp::Command root4("nonames", "No names");
    root4.enableCompletion(noNames);
    std::ostringstream fish4, bash4, ps4;
    root4.printCompletionFish(fish4);
    root4.printCompletionBash(bash4);
    root4.printCompletionPowerShell(ps4);
    expect(contains(fish4.str(), "complete -c nonames -f -a"), "fish static fallback: static output");
    expect(contains(bash4.str(), "_nonames_complete() {"), "bash static fallback: static output");
    expect(contains(ps4.str(), "$__claspSubs['nonames']"), "powershell static fallback: static output");

    // addCompleteCommands disabled: static scripts everywhere.
    clasp::Command::CompletionConfig staticCfg;
    staticCfg.addCompleteCommands = false;
    clasp::Command root5("allstatic", "All static");
    root5.enableCompletion(staticCfg);
    std::ostringstream fish5, bash5, ps5;
    root5.printCompletionFish(fish5);
    root5.printCompletionBash(bash5);
    root5.printCompletionPowerShell(ps5);
    expect(contains(fish5.str(), "complete -c allstatic -f -a"), "fish addCompleteCommands=false: static");
    expect(contains(bash5.str(), "_allstatic_complete() {"), "bash addCompleteCommands=false: static");
    expect(contains(ps5.str(), "$__claspSubs['allstatic']"), "powershell addCompleteCommands=false: static");

    // Long identifiers (>15 chars) exercise the heap-allocation branches of
    // inlined std::string copies in the script generators.
    clasp::Command::CompletionConfig longNodesc;
    longNodesc.completeNoDescCommandName = "a-long-complete-nodesc-name";
    clasp::Command longRoot("a-very-long-root-application", "Long identifiers");
    longRoot.enableCompletion(longNodesc);
    clasp::Command longSub("a-long-subcommand-name", "With a really long description text");
    longSub.addAlias("a-long-alias-name-x");
    longSub.withFlag("--a-very-long-flag-name", "", "a-very-long-flag-name",
                     "A really long flag description text", std::string(""));
    longRoot.addCommand(std::move(longSub));

    std::ostringstream lbash, lfish, lps;
    longRoot.printCompletionBash(lbash);
    longRoot.printCompletionFish(lfish);
    longRoot.printCompletionPowerShell(lps);
    expect(contains(lbash.str(), "_a_very_long_root_application_complete"), "long identifiers: bash sanitized");
    expect(contains(lps.str(), "$__claspCompleteCmd = 'a-long-complete-nodesc-name'"),
           "long identifiers: powershell nodesc command");
    expect(contains(lfish.str(), "__clasp_a_very_long_root_application_fish_complete"),
           "long identifiers: dynamic fish helper");

    clasp::Command::CompletionConfig longStatic;
    longStatic.addCompletionCommand = false;
    longStatic.addCompleteCommands = false;
    clasp::Command longStaticRoot("another-very-long-root-name", "Long static");
    longStaticRoot.enableCompletion(longStatic);
    longStaticRoot.version("1.0.0");
    clasp::Command longStaticSub("a-long-subcommand-name", "With a really long description text");
    longStaticSub.withFlag("--a-very-long-flag-name", "", "a-very-long-flag-name",
                           "A really long flag description text", std::string(""));
    longStaticRoot.addCommand(std::move(longStaticSub));
    std::ostringstream lsbash, lsfish, lsps;
    longStaticRoot.printCompletionBash(lsbash);
    longStaticRoot.printCompletionFish(lsfish);
    longStaticRoot.printCompletionPowerShell(lsps);
    expect(contains(lsbash.str(), "\"another-very-long-root-name help\""), "long identifiers: static bash help entry");
    expect(contains(lsfish.str(), "-l a-very-long-flag-name -d \"A really long flag description text\""),
           "long identifiers: static fish long flag description");
    expect(contains(lsps.str(), "$__claspSubs['another-very-long-root-name a-long-subcommand-name']"),
           "long identifiers: static powershell sub entry");
    expect(contains(lsps.str(), "$__claspFlags['another-very-long-root-name a-long-subcommand-name']"),
           "long identifiers: static powershell flag map entry");
}

// ------------------------------------------------------------------
// Shell escaping helpers: apostrophes and identifier sanitization.
// ------------------------------------------------------------------
void testShellEscaping() {
    clasp::Command::CompletionConfig staticCfg;
    staticCfg.addCompletionCommand = false;
    staticCfg.addCompleteCommands = false;

    clasp::Command root("esc", "Escaping");
    root.enableCompletion(staticCfg);
    clasp::Command sub("it's", "Apostrophe");
    sub.addAlias("a'b");
    root.addCommand(std::move(sub));

    std::ostringstream ps, bash;
    root.printCompletionPowerShell(ps);
    expect(contains(ps.str(), "it''s"), "powershell escaping: apostrophe doubled");
    expect(contains(ps.str(), "a''b"), "powershell escaping: alias apostrophe doubled");

    root.printCompletionBash(bash);
    expect(contains(bash.str(), "\"esc it's\""), "bash escaping: apostrophe kept verbatim");

    // Non-identifier characters in the root name.
    clasp::Command weird("my-app.v2", "Weird");
    weird.enableCompletion(staticCfg);
    std::ostringstream wb;
    weird.printCompletionBash(wb);
    expect(contains(wb.str(), "_my_app_v2_complete"), "sanitize: non-alnum chars replaced");

    // Underscores are kept as-is by sanitization.
    clasp::Command under("app_x", "Underscore");
    under.enableCompletion(staticCfg);
    std::ostringstream ub2;
    under.printCompletionBash(ub2);
    expect(contains(ub2.str(), "_app_x_complete"), "sanitize: underscore kept");

    // A leading underscore suppresses the auto-prefix.
    clasp::Command lead("_lead", "Leading underscore");
    lead.enableCompletion(staticCfg);
    std::ostringstream lb;
    lead.printCompletionBash(lb);
    expect(contains(lb.str(), "___lead_complete") || contains(lb.str(), "__lead_complete"),
           "sanitize: leading underscore tolerated");

    clasp::Command digit("7app", "Digit start");
    digit.enableCompletion(staticCfg);
    std::ostringstream db;
    digit.printCompletionBash(db);
    expect(contains(db.str(), "__7app_complete"), "sanitize: leading digit gets underscore prefix");

    // An empty root name sanitizes to a single underscore.
    clasp::Command empty("", "Empty name");
    empty.enableCompletion(staticCfg);
    std::ostringstream eb;
    empty.printCompletionBash(eb);
    expect(!eb.str().empty(), "sanitize: empty name tolerated");
}

// ------------------------------------------------------------------
// enableCompletion(): registration variants.
// ------------------------------------------------------------------
void testEnableCompletionRegistration() {
    // Default registration adds completion/__complete/__completeNoDesc.
    clasp::Command root("regapp", "Registration");
    root.enableCompletion();
    clasp::Command sub("leaf", "Leaf");
    root.addCommand(std::move(sub));
    {
        std::ostringstream out;
        root.setOut(out);
        expect(runArgs(root, {"completion", "bash"}) == 0, "enableCompletion: completion bash runs");
        expect(contains(out.str(), "_regapp_complete"), "enableCompletion: bash script printed");
    }
    {
        std::ostringstream out;
        root.setOut(out);
        expect(runArgs(root, {"completion", "zsh"}) == 0, "enableCompletion: completion zsh runs");
        expect(runArgs(root, {"completion", "fish"}) == 0, "enableCompletion: completion fish runs");
        expect(runArgs(root, {"completion", "powershell"}) == 0, "enableCompletion: completion powershell runs");
        expect(runArgs(root, {"completion", "tcsh"}) == 1, "enableCompletion: unknown shell rejected");
    }

    // Existing "completion" subcommand wins over the built-in one.
    std::ostringstream out2, err2;
    clasp::Command root2("preapp", "Pre-existing");
    root2.setOut(out2);
    root2.setErr(err2);
    {
        clasp::Command completion("completion", "Custom");
        completion.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 42; });
        root2.addCommand(std::move(completion));
    }
    root2.enableCompletion();
    expect(runArgs(root2, {"completion", "bash"}) == 42, "enableCompletion: existing completion command kept");

    // Existing __complete / __completeNoDesc subcommands are not replaced.
    std::ostringstream out3;
    clasp::Command root3("precomp", "Pre complete");
    root3.setOut(out3);
    {
        clasp::Command c1("__complete", "Custom desc");
        c1.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 7; });
        root3.addCommand(std::move(c1));
        clasp::Command c2("__completeNoDesc", "Custom nodesc");
        c2.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 8; });
        root3.addCommand(std::move(c2));
    }
    root3.enableCompletion();
    expect(runArgs(root3, {"__complete", ""}) == 7, "enableCompletion: existing __complete kept");
    expect(runArgs(root3, {"__completeNoDesc", ""}) == 8, "enableCompletion: existing __completeNoDesc kept");

    // Disabled completion command: only the hidden complete commands exist.
    clasp::Command::CompletionConfig cfgNoCompCmd;
    cfgNoCompCmd.addCompletionCommand = false;
    {
        std::ostringstream o;
        clasp::Command root4("nocomp", "No completion command");
        root4.setOut(o);
        root4.setErr(o);
        root4.enableCompletion(cfgNoCompCmd);
        expect(runArgs(root4, {"completion", "bash"}) == 1, "enableCompletion: addCompletionCommand=false");
    }

    // Empty completion command name: treated as disabled.
    clasp::Command::CompletionConfig cfgEmptyName;
    cfgEmptyName.completionCommandName = "";
    {
        std::ostringstream o;
        clasp::Command root5("emptyname", "Empty completion name");
        root5.setOut(o);
        root5.setErr(o);
        root5.enableCompletion(cfgEmptyName);
        expect(runArgs(root5, {"completion", "bash"}) == 1, "enableCompletion: empty completionCommandName");
    }

    // addCompleteCommands disabled: no hidden complete commands.
    clasp::Command::CompletionConfig cfgNoComplete;
    cfgNoComplete.addCompleteCommands = false;
    {
        std::ostringstream o;
        clasp::Command root6("nocomp2", "No complete commands");
        root6.setOut(o);
        root6.setErr(o);
        root6.enableCompletion(cfgNoComplete);
        expect(runArgs(root6, {"__complete", ""}) == 1, "enableCompletion: addCompleteCommands=false");
        expect(runArgs(root6, {"__completeNoDesc", ""}) == 1, "enableCompletion: nodesc not added either");
    }

    // Empty complete names: not added.
    clasp::Command::CompletionConfig cfgNoNames;
    cfgNoNames.completeCommandName = "";
    cfgNoNames.completeNoDescCommandName = "";
    {
        std::ostringstream o;
        clasp::Command root7("nonames7", "Empty complete names");
        root7.setOut(o);
        root7.setErr(o);
        root7.enableCompletion(cfgNoNames);
        expect(runArgs(root7, {"__complete", ""}) == 1, "enableCompletion: empty complete names not added");
        expect(runArgs(root7, {"__completeNoDesc", ""}) == 1, "enableCompletion: empty nodesc name not added");
    }

    // The __complete command with no args completes the root level.
    std::ostringstream sink8;
    clasp::Command root8("noargs", "No args completion");
    root8.setOut(sink8);
    root8.enableCompletion();
    clasp::Command leaf8("leaf8", "Leaf eight");
    root8.addCommand(std::move(leaf8));
    {
        std::ostringstream o;
        root8.setOut(o);
        expect(runArgs(root8, {"__complete"}) == 0, "__complete: no args runs");
        expect(contains(o.str(), "leaf8\tLeaf eight"), "__complete: no args lists root commands with descriptions");
    }
    {
        std::ostringstream o;
        root8.setOut(o);
        expect(runArgs(root8, {"__completeNoDesc", ""}) == 0, "__completeNoDesc: runs");
        expect(contains(o.str(), "leaf8\n") && !contains(o.str(), "\t"), "__completeNoDesc: no descriptions");
    }
}

// ------------------------------------------------------------------
// normalizeFlagKeys(): custom key normalization plumbed into the
// completion parser and flag lookups.
// ------------------------------------------------------------------
void testNormalizeFlagKeyCompletion() {
    std::ostringstream sink;
    clasp::Command root("nkey", "Normalize keys");
    root.setOut(sink);
    root.enableCompletion();
    root.normalizeFlagKeys([](std::string key) {
        for (auto& ch : key)
            if (ch == '_') ch = '-';
        return key;
    });
    root.withFlag("--some-flag", "", "some-flag", "Some flag", std::string(""));
    root.registerFlagCompletion("--some-flag",
        [](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&, std::string_view) {
            return std::vector<std::string>{"v1"};
        });
    root.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });

    // "--some_" is normalized to "--some-" before value-completion matching.
    auto r = complete(root, sink, {"--some_flag="}, false);
    expect(contains(r, "v1"), "normalize keys: underscore eq token resolves value candidates");

    auto r2 = complete(root, sink, {"--some_flag", ""}, false);
    expect(contains(r2, "v1"), "normalize keys: underscore separate token resolves value candidates");

    // Plain prefix completion keeps literal matching (no normalization).
    auto r3 = complete(root, sink, {"--some-"}, false);
    expect(contains(r3, "--some-flag"), "normalize keys: dashed prefix lists the flag");

    expect(runArgs(root, {"--some_flag=x"}) == 0, "normalize keys: run accepts underscore spelling");
}

// ------------------------------------------------------------------
// traverseChildren(): subtree-wide flag lookups in completion and
// execution resolution.
// ------------------------------------------------------------------
void testTraverseChildrenResolution() {
    std::ostringstream sink;
    clasp::Command root("tapp", "Traverse");
    root.setOut(sink);
    root.enableCompletion();
    root.traverseChildren();
    root.version("1.0");
    root.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
    root.withFlag("--tswitch", "", "tswitch", "Bool in root", true);
    root.withFlag("--tvalue", "", "tvalue", "Value in root", std::string(""));
    root.withFlag("--tcount", "", "tcount", "Count true", std::string(""));
    root.markFlagAnnotation("--tcount", "count", "true");
    root.withFlag("--tcount1", "", "tcount1", "Count 1", std::string(""));
    root.markFlagAnnotation("--tcount1", "count", "1");
    root.withFlag("--tcounty", "", "tcounty", "Count yes", std::string(""));
    root.markFlagAnnotation("--tcounty", "count", "yes");
    root.withFlag("--tcountno", "", "tcountno", "Count no", std::string(""));
    root.markFlagAnnotation("--tcountno", "count", "no");
    clasp::Command mid("mid", "Mid");
    mid.withFlag("--deep", "", "deep", "Bool in mid", true);
    mid.withFlag("--deepv", "", "deepv", "Value in mid", std::string(""));
    clasp::Command leaf("leaf", "Leaf");
    leaf.withFlag("--deeper", "", "deeper", "Value in leaf", std::string(""));
    leaf.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
    mid.addCommand(std::move(leaf));
    root.addCommand(std::move(mid));
    clasp::Command tailsub("tailsub", "Tail");
    tailsub.withFlag("--tailflag", "", "tailflag", "Tail flag", true);
    tailsub.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
    root.addCommand(std::move(tailsub));

    // Completion side: subtree lookup for bool, nested bool, unknown keys.
    complete(root, sink, {"--tswitch", ""}, false);        // found bool at root.
    complete(root, sink, {"--tcount", ""}, false);         // found count=true at root.
    complete(root, sink, {"--tcount1", ""}, false);        // found count=1 at root.
    complete(root, sink, {"--tcounty", ""}, false);        // found count=yes at root.
    complete(root, sink, {"--tcountno", ""}, false);       // count annotation rejected spelling.
    complete(root, sink, {"--deep", "mid", ""}, false);    // found bool one level down.
    complete(root, sink, {"--deepv", ""}, false);          // non-bool below root -> subtree bail.
    complete(root, sink, {"--tvalue", ""}, false);         // found non-bool -> bail out.
    complete(root, sink, {"--deep", ""}, false);           // subtree search visits tailsub after mid.
    complete(root, sink, {"--nope", ""}, false);           // not found anywhere.
    complete(root, sink, {"--help", ""}, false);           // short-circuit on help key.
    complete(root, sink, {"-h", ""}, false);               // short-circuit on -h.
    complete(root, sink, {"--version", ""}, false);        // short-circuit on --version.

    // Execution side.
    expect(runArgs(root, {"--tswitch"}) == 0, "traverse execution: bool at root");
    expect(runArgs(root, {"--tcount"}) == 0, "traverse execution: count=true at root");
    expect(runArgs(root, {"--tcount1"}) == 0, "traverse execution: count=1 at root");
    expect(runArgs(root, {"--tcounty"}) == 0, "traverse execution: count=yes at root");
    expect(runArgs(root, {"--tcountno", "x"}) == 0, "traverse execution: count=no treated as value flag");
    expect(runArgs(root, {"mid", "--deep"}) == 0, "traverse execution: bool below root");
    expect(runArgs(root, {"--tvalue", "x"}) == 0, "traverse execution: root value flag consumes next");
    expect(runArgs(root, {"mid", "leaf", "--deeper", "x"}) == 0, "traverse execution: leaf value flag");

    // allowUnknownFlags so a non-matching flag still reaches the action while
    // the subtree lookup walks past the matching sibling command.
    root.allowUnknownFlags(true);
    expect(runArgs(root, {"--deep", "zz"}) == 0, "traverse execution: nested bool with unknown-value arg");
    expect(runArgs(root, {"-h"}) == 0, "traverse execution: -h prints help");
    expect(runArgs(root, {"--version"}) == 0, "traverse execution: --version prints");
    expect(runArgs(root, {"--deepv"}) == 0, "traverse execution: non-bool below root with unknown flag");
}

// ------------------------------------------------------------------
// applyBoundFlagValues(): flags with an empty long name are skipped.
// ------------------------------------------------------------------
void testApplyBoundFlagValues() {
    TypedValue typed;
    clasp::Command root("bapp", "Bound values");
    // Short-only flag: invisible to the parser, but present in effectiveFlags.
    root.withFlag("", "-s", "shortonly", "Short only", std::string("v"));
    // Normal bound flag.
    root.withValueFlag("--lvl", "", "lvl", "Level", typed);
    root.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });

    expect(runArgs(root, {}) == 0, "bound values: bare run succeeds");
    expect(typed.value == "seed", "bound values: untouched without explicit flag");

    expect(runArgs(root, {"--lvl", "updated"}) == 0, "bound values: run with explicit flag");
    expect(typed.value == "updated", "bound values: explicit value applied to bound Value");
}

} // namespace

int main() {
#if defined(_MSC_VER) && defined(_DEBUG)
    // Headless CI: a Debug-CRT report (ucrtbased!CrtDbgReportW) pops a modal
    // MessageBox that nobody will ever dismiss, hanging the test. Route the
    // reports to stderr instead, so a violation shows in the ctest log and
    // the process aborts rather than waiting forever.
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
    _set_invalid_parameter_handler(claspReportInvalidParameter);
#endif
    try {
    expect(true, "scaffold");
    testResolvedCompletionConfigFallback();
    testUsageTemplateCommandPathKey();
    testFlagTypeForHelp();
    testValueCompletionEqualSign();
    testValueCompletionSeparateArg();
    testFlagNameCompletion();
    testSubcommandCompletion();
    testValidArgsDirectiveOverride();
    testSuggestCommands();
    testShortFlagGroupingResolution();
    testNegationAndEqResolution();
    testDoubleDashResolution();
    testExecutionHelpVersionResolution();
    testCompletionEntriesStaticScripts();
    testDynamicShellScripts();
    testShellEscaping();
    testEnableCompletionRegistration();
    testNormalizeFlagKeyCompletion();
    testTraverseChildrenResolution();
    testApplyBoundFlagValues();
    } catch (const std::exception& e) {
        // Surface the exact MSVC-Debug-only failure: type + message before
        // the terminate abort hides them from the ctest log.
        std::cout << "UNCAUGHT exception: " << typeid(e).name() << ": " << e.what() << std::endl;
        return 2;
    } catch (...) {
        std::cout << "UNCAUGHT non-standard exception" << std::endl;
        return 2;
    }
    if (g_failures == 0) {
        std::cout << "ALL OK" << std::endl;
        return 0;
    }
    std::cout << g_failures << " failures" << std::endl;
    return 1;
}
