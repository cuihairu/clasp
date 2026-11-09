#ifndef CLASP_TESTS_BRANCH_SUPPORT_HPP
#define CLASP_TESTS_BRANCH_SUPPORT_HPP

// Shared scaffolding for the branch_parser test executable, split by phase
// (token/parse paths, value/type validation, access + external sources).

// Coverage tests for remaining parser.hpp / value_parse.hpp branches
// (external values, byte units, IP/CIDR/URL parsing edges).
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

#include "clasp/parser.hpp"
#include "clasp/detail/value_parse.hpp"
#include "clasp/utils.hpp"


inline int g_failures = 0;

inline void expect(bool cond, const char* label) {
    std::cout << label << ": " << (cond ? "ok" : "fail") << std::endl;
    if (!cond) ++g_failures;
}

inline clasp::Parser makeParser(const std::vector<std::string>& args, const std::vector<clasp::Flag>& flags,
                         clasp::Parser::Options options = {}) {
    std::vector<char*> ptrs;
    ptrs.push_back(const_cast<char*>("app"));
    for (const auto& a : args) ptrs.push_back(const_cast<char*>(a.c_str()));
    return clasp::Parser(static_cast<int>(ptrs.size()), ptrs.data(), flags, std::move(options));
}

inline clasp::Flag mkFlag(const char* lng, const char* sh, clasp::FlagValue def) {
    return clasp::Flag(lng, sh, "D", "var", std::move(def));
}

inline clasp::Flag annFlag(const char* lng, const char* key, const char* value, clasp::FlagValue def) {
    clasp::Flag f(lng, "", "D", "var", std::move(def));
    f.setAnnotation(key, value);
    return f;
}

inline clasp::Parser::Options allowUnknown() {
    clasp::Parser::Options o;
    o.allowUnknownFlags = true;
    return o;
}

// ---------------------------------------------------------------------------

// Test entry points defined across the split translation units; main() calls each.

void testUnknownFlagPaths();
void testNoOptPaths();
void testBoolLiterals();
void testGetFlagChain();
void testNarrowIntRanges();
void testCheckedExternal();
void testCountPaths();
void testLookupPaths();
void testArrayAndSplitPaths();
void testSuggest();
void testValueParseDirect();
void testBytesUnits();
void testIPParsing();
void testIPMaskAndCIDR();
void testURLParsing();
void testAnnotationKinds();
void testShortGroupPaths();
void testTokenEdges();

#endif // CLASP_TESTS_BRANCH_SUPPORT_HPP
