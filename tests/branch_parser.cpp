#include "branch_support.hpp"

int main() {
    testUnknownFlagPaths();
    testNoOptPaths();
    testBoolLiterals();
    testGetFlagChain();
    testNarrowIntRanges();
    testCheckedExternal();
    testCountPaths();
    testLookupPaths();
    testArrayAndSplitPaths();
    testSuggest();
    testValueParseDirect();
    testBytesUnits();
    testIPParsing();
    testIPMaskAndCIDR();
    testURLParsing();
    testAnnotationKinds();
    testShortGroupPaths();
    testTokenEdges();

    if (g_failures == 0) {
        std::cout << "ALL OK" << std::endl;
        return 0;
    }
    std::cout << g_failures << " failures" << std::endl;
    return 1;
}


