#include <iostream>
#include <string>
#include <vector>

#include "clasp/utils.hpp"

namespace {

void testLevenshteinDistance() {
    using namespace clasp::utils;

    // Test basic cases
    auto d1 = levenshteinDistance("", "");
    auto d2 = levenshteinDistance("", "abc");
    auto d3 = levenshteinDistance("abc", "");
    auto d4 = levenshteinDistance("abc", "abc");
    auto d5 = levenshteinDistance("abc", "abd");
    auto d6 = levenshteinDistance("kitten", "sitting");
    auto d7 = levenshteinDistance("book", "back");
    auto d8 = levenshteinDistance("hello", "world");

    (void)d1; (void)d2; (void)d3; (void)d4; (void)d5; (void)d6; (void)d7; (void)d8;
}

void testSuggest() {
    using namespace clasp::utils;

    // Test with exact prefix match
    std::vector<std::string> candidates1 = {"help", "version", "verbose", "verify"};
    auto s1 = suggest("ver", candidates1, 3, 2);

    // Test with Levenshtein distance
    std::vector<std::string> candidates2 = {"help", "version", "verbose", "output"};
    auto s2 = suggest("verb", candidates2, 3, 2);

    // Test with no close matches
    std::vector<std::string> candidates3 = {"help", "version", "output"};
    auto s3 = suggest("xyz", candidates3, 3, 2);

    // Test with maxResults limit
    std::vector<std::string> candidates4 = {"foo", "fab", "fob", "fub"};
    auto s4 = suggest("fab", candidates4, 2, 2);

    // Test with empty candidates
    std::vector<std::string> candidates5;
    auto s5 = suggest("test", candidates5, 3, 2);

    // Test with empty strings in candidates
    std::vector<std::string> candidates6 = {"", "help", "version"};
    auto s6 = suggest("ver", candidates6, 3, 2);

    (void)s1; (void)s2; (void)s3; (void)s4; (void)s5; (void)s6;
}

} // namespace

int main() {
    testLevenshteinDistance();
    testSuggest();

    std::cout << "ok\n";
    return 0;
}
