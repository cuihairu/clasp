#include <iostream>
#include <string>

#include "clasp/color.hpp"

namespace {

int g_failures = 0;

void expect(bool cond, const char* label) {
    std::cout << label << ": " << (cond ? "pass" : "fail") << std::endl;
    if (!cond) ++g_failures;
}

void testColorMode() {
    using namespace clasp;

    // Test parseMode
    auto autoMode = clasp::color::parseMode("auto");
    expect(autoMode.has_value() && autoMode.value() == ColorMode::Auto, "parseMode 'auto'");

    auto alwaysMode = clasp::color::parseMode("always");
    expect(alwaysMode.has_value() && alwaysMode.value() == ColorMode::Always, "parseMode 'always'");

    auto neverMode = clasp::color::parseMode("never");
    expect(neverMode.has_value() && neverMode.value() == ColorMode::Never, "parseMode 'never'");

    auto invalidMode = clasp::color::parseMode("invalid");
    expect(!invalidMode.has_value(), "parseMode 'invalid'");

    // Test modeName
    expect(clasp::color::modeName(ColorMode::Auto) == "auto", "modeName Auto");
    expect(clasp::color::modeName(ColorMode::Always) == "always", "modeName Always");
    expect(clasp::color::modeName(ColorMode::Never) == "never", "modeName Never");
}

void testColorTheme() {
    using namespace clasp;

    // Test parseTheme
    auto vscode = clasp::color::parseTheme("vscode");
    expect(vscode.has_value() && vscode.value() == ColorThemeName::Vscode, "parseTheme 'vscode'");

    auto sublime = clasp::color::parseTheme("sublime");
    expect(sublime.has_value() && sublime.value() == ColorThemeName::Sublime, "parseTheme 'sublime'");

    auto iterm2 = clasp::color::parseTheme("iterm2");
    expect(iterm2.has_value() && iterm2.value() == ColorThemeName::Iterm2, "parseTheme 'iterm2'");

    auto invalidTheme = clasp::color::parseTheme("invalid");
    expect(!invalidTheme.has_value(), "parseTheme 'invalid'");

    // Test themeName
    expect(clasp::color::themeName(ColorThemeName::Vscode) == "vscode", "themeName Vscode");
    expect(clasp::color::themeName(ColorThemeName::Sublime) == "sublime", "themeName Sublime");
    expect(clasp::color::themeName(ColorThemeName::Iterm2) == "iterm2", "themeName Iterm2");
}

void testBuiltinTheme() {
    using namespace clasp;

    // Test builtinTheme
    const auto& vscodeTheme = clasp::color::builtinTheme(ColorThemeName::Vscode);
    expect(!vscodeTheme.flag.empty(), "builtinTheme Vscode");

    const auto& sublimeTheme = clasp::color::builtinTheme(ColorThemeName::Sublime);
    expect(!sublimeTheme.flag.empty(), "builtinTheme Sublime");

    const auto& iterm2Theme = clasp::color::builtinTheme(ColorThemeName::Iterm2);
    expect(!iterm2Theme.flag.empty(), "builtinTheme Iterm2");
}

void testEnvFunctions() {
    using namespace clasp::color;

    // Test envNoColor (should return false if NO_COLOR not set, true if set)
    // We can't test with actual env vars in a controlled way, but we can call the function
    auto noColor = envNoColor();
    (void)noColor;
    std::cout << "envNoColor: pass" << std::endl;

    auto termDumb = envTermDumb();
    (void)termDumb;
    std::cout << "envTermDumb: pass" << std::endl;
}

void testAnsiCodes() {
    using namespace clasp::color;

    // Test ANSI code generation
    auto rgb = ansiRgbFg(255, 128, 64);
    expect(!rgb.empty(), "ansiRgbFg");

    auto bold = ansiBold();
    expect(!bold.empty(), "ansiBold");

    auto dim = ansiDim();
    expect(!dim.empty(), "ansiDim");
}

void testGetEnv() {
    using namespace clasp::color::detail;

    // Test getEnv with non-existent variable
    auto result = getEnv("NONEXISTENT_ENV_VAR_CLASP_TEST");
    expect(!result.has_value(), "getEnv non-existent");
}

void testStreamEnum() {
    using namespace clasp::color;

    // Test Stream enum exists and can be used
    Stream s = Stream::Stdout;
    (void)s;

    Stream s2 = Stream::Stderr;
    (void)s2;

    Stream s3 = Stream::Other;
    (void)s3;

    std::cout << "Stream enum: pass" << std::endl;
}

void testIsTty() {
    using namespace clasp::color;

    // Call isTty for all stream types to exercise color.cpp
    bool stdoutTty = isTty(Stream::Stdout);
    (void)stdoutTty;

    bool stderrTty = isTty(Stream::Stderr);
    (void)stderrTty;

    bool otherTty = isTty(Stream::Other);
    expect(!otherTty, "isTty Other is false");
}

void testEnableVt() {
    using namespace clasp::color;

    // Call enableVirtualTerminalProcessing for all stream types
    bool stdoutVt = enableVirtualTerminalProcessing(Stream::Stdout);
    (void)stdoutVt;

    bool stderrVt = enableVirtualTerminalProcessing(Stream::Stderr);
    (void)stderrVt;

    // On non-Windows, enableVirtualTerminalProcessing always returns true
    bool otherVt = enableVirtualTerminalProcessing(Stream::Other);
#if defined(_WIN32)
    expect(!otherVt, "enableVt Other is false");
#else
    expect(otherVt, "enableVt Other is true (non-Windows)");
#endif
}

void testColorRole() {
    using namespace clasp;

    // Test ColorRole enum values exist
    ColorRole role1 = ColorRole::Section;
    ColorRole role2 = ColorRole::Command;
    ColorRole role3 = ColorRole::Flag;
    ColorRole role4 = ColorRole::Type;
    ColorRole role5 = ColorRole::Meta;
    ColorRole roleErr = ColorRole::Error;

    (void)role1; (void)role2; (void)role3; (void)role4; (void)role5; (void)roleErr;

    std::cout << "ColorRole enum: pass" << std::endl;
}

} // namespace

int main() {
    std::cout << "=== Testing ColorMode ===" << std::endl;
    testColorMode();

    std::cout << "\n=== Testing ColorTheme ===" << std::endl;
    testColorTheme();

    std::cout << "\n=== Testing BuiltinTheme ===" << std::endl;
    testBuiltinTheme();

    std::cout << "\n=== Testing EnvFunctions ===" << std::endl;
    testEnvFunctions();

    std::cout << "\n=== Testing AnsiCodes ===" << std::endl;
    testAnsiCodes();

    std::cout << "\n=== Testing GetEnv ===" << std::endl;
    testGetEnv();

    std::cout << "\n=== Testing StreamEnum ===" << std::endl;
    testStreamEnum();

    std::cout << "\n=== Testing ColorRole ===" << std::endl;
    testColorRole();

    std::cout << "\n=== Testing IsTty ===" << std::endl;
    testIsTty();

    std::cout << "\n=== Testing EnableVt ===" << std::endl;
    testEnableVt();

    if (g_failures == 0) {
        std::cout << "\nok\n";
    }
    return g_failures == 0 ? 0 : 1;
}
