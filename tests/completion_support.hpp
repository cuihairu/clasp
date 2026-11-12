#ifndef CLASP_TESTS_COMPLETION_SUPPORT_HPP
#define CLASP_TESTS_COMPLETION_SUPPORT_HPP

// Shared scaffolding for the completion coverage test executable, split by concern
// (protocol/resolution, shell script generation + escaping, end-to-end shell execution).

// Coverage tests for completion internals (shell script generation,
// completion entries, value completion).
#include <exception>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <typeinfo>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#ifndef _WIN32
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <filesystem>
#endif
#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>

// Debug-CRT violations that skip _CrtDbgReport still funnel through the
// invalid-parameter handler; print the details so the ctest log shows them.
inline void claspReportInvalidParameter(const wchar_t* expr, const wchar_t* func,
                                 const wchar_t* file, unsigned line, uintptr_t) {
    std::fwprintf(stderr, L"INVALID_PARAMETER expr=%ls func=%ls file=%ls line=%u\n",
                  expr ? expr : L"?", func ? func : L"?", file ? file : L"?", line);
    std::fflush(stderr);
}
#endif

#include "clasp/command.hpp"


inline int g_failures = 0;

inline void expect(bool cond, const char* label) {
    std::cout << label << ": " << (cond ? "ok" : "fail") << std::endl;
    if (!cond) ++g_failures;
}

inline bool contains(const std::string& hay, const std::string& needle) {
    return hay.find(needle) != std::string::npos;
}

inline int countOf(const std::string& hay, const std::string& needle) {
    int n = 0;
    std::size_t pos = 0;
    while ((pos = hay.find(needle, pos)) != std::string::npos) {
        ++n;
        pos += needle.size();
    }
    return n;
}

inline int runArgs(clasp::Command& root, std::vector<std::string> args) {
    std::vector<std::string> argv;
    argv.push_back(root.name());
    for (auto& a : args) argv.push_back(std::move(a));
    std::vector<char*> ptrs;
    for (auto& a : argv) ptrs.push_back(a.data());
    return root.run(static_cast<int>(ptrs.size()), ptrs.data());
}

// Runs the hidden completion command in-process; output lands in `sink`.
inline std::string complete(clasp::Command& root, std::ostringstream& sink, std::vector<std::string> args, bool withDesc) {
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


// Test entry points defined across the split translation units; main() calls each.

void testResolvedCompletionConfigFallback();
void testUsageTemplateCommandPathKey();
void testFlagTypeForHelp();
void testValueCompletionEqualSign();
void testValueCompletionSeparateArg();
void testValueCompletionShortGroupTail();
void testFlagNameCompletion();
void testSubcommandCompletion();
void testValidArgsDirectiveOverride();
void testSuggestCommands();
void testShortFlagGroupingResolution();
void testNegationAndEqResolution();
void testDoubleDashResolution();
void testExecutionHelpVersionResolution();
void testCompletionEntriesStaticScripts();
void testDynamicShellScripts();
void testShellEscaping();
void testShellEscapingEndToEnd();
void testEnableCompletionRegistration();
void testNormalizeFlagKeyCompletion();
void testTraverseChildrenResolution();
void testApplyBoundFlagValues();

#endif // CLASP_TESTS_COMPLETION_SUPPORT_HPP
