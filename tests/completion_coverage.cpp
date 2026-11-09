#include "completion_support.hpp"

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
    testShellEscapingEndToEnd();
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


