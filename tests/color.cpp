#include <iostream>
#include <string>

#include "clasp/color.hpp"

namespace {

void testColorMode() {
    using namespace clasp;

    // Test parseMode
    auto autoMode = clasp::color::parseMode("auto");
    std::cout << "parseMode 'auto': " << (autoMode.has_value() && autoMode.value() == ColorMode::Auto ? "pass" : "fail") << std::endl;

    auto alwaysMode = clasp::color::parseMode("always");
    std::cout << "parseMode 'always': " << (alwaysMode.has_value() && alwaysMode.value() == ColorMode::Always ? "pass" : "fail") << std::endl;

    auto neverMode = clasp::color::parseMode("never");
    std::cout << "parseMode 'never': " << (neverMode.has_value() && neverMode.value() == ColorMode::Never ? "pass" : "fail") << std::endl;

    auto invalidMode = clasp::color::parseMode("invalid");
    std::cout << "parseMode 'invalid': " << (!invalidMode.has_value() ? "pass" : "fail") << std::endl;

    // Test modeName
    std::cout << "modeName Auto: " << (clasp::color::modeName(ColorMode::Auto) == "auto" ? "pass" : "fail") << std::endl;
    std::cout << "modeName Always: " << (clasp::color::modeName(ColorMode::Always) == "always" ? "pass" : "fail") << std::endl;
    std::cout << "modeName Never: " << (clasp::color::modeName(ColorMode::Never) == "never" ? "pass" : "fail") << std::endl;
}

void testColorTheme() {
    using namespace clasp;

    // Test parseTheme
    auto vscode = clasp::color::parseTheme("vscode");
    std::cout << "parseTheme 'vscode': " << (vscode.has_value() && vscode.value() == ColorThemeName::Vscode ? "pass" : "fail") << std::endl;

    auto sublime = clasp::color::parseTheme("sublime");
    std::cout << "parseTheme 'sublime': " << (sublime.has_value() && sublime.value() == ColorThemeName::Sublime ? "pass" : "fail") << std::endl;

    auto iterm2 = clasp::color::parseTheme("iterm2");
    std::cout << "parseTheme 'iterm2': " << (iterm2.has_value() && iterm2.value() == ColorThemeName::Iterm2 ? "pass" : "fail") << std::endl;

    auto invalidTheme = clasp::color::parseTheme("invalid");
    std::cout << "parseTheme 'invalid': " << (!invalidTheme.has_value() ? "pass" : "fail") << std::endl;

    // Test themeName
    std::cout << "themeName Vscode: " << (clasp::color::themeName(ColorThemeName::Vscode) == "vscode" ? "pass" : "fail") << std::endl;
    std::cout << "themeName Sublime: " << (clasp::color::themeName(ColorThemeName::Sublime) == "sublime" ? "pass" : "fail") << std::endl;
    std::cout << "themeName Iterm2: " << (clasp::color::themeName(ColorThemeName::Iterm2) == "iterm2" ? "pass" : "fail") << std::endl;
}

void testBuiltinTheme() {
    using namespace clasp;

    // Test builtinTheme
    const auto& vscodeTheme = clasp::color::builtinTheme(ColorThemeName::Vscode);
    std::cout << "builtinTheme Vscode: " << (!vscodeTheme.flag.empty() ? "pass" : "fail") << std::endl;

    const auto& sublimeTheme = clasp::color::builtinTheme(ColorThemeName::Sublime);
    std::cout << "builtinTheme Sublime: " << (!sublimeTheme.flag.empty() ? "pass" : "fail") << std::endl;

    const auto& iterm2Theme = clasp::color::builtinTheme(ColorThemeName::Iterm2);
    std::cout << "builtinTheme Iterm2: " << (!iterm2Theme.flag.empty() ? "pass" : "fail") << std::endl;
}

void testEnvFunctions() {
    using namespace clasp::color;

    // Test envNoColor (should return false if NO_COLOR not set, true if set)
    // We can't test with actual env vars in a controlled way, but we can call the function
    auto noColor = envNoColor();
    std::cout << "envNoColor: called" << std::endl;

    auto termDumb = envTermDumb();
    std::cout << "envTermDumb: called" << std::endl;
}

void testAnsiCodes() {
    using namespace clasp::color;

    // Test ANSI code generation
    auto rgb = ansiRgbFg(255, 128, 64);
    std::cout << "ansiRgbFg: " << (!rgb.empty() ? "pass" : "fail") << std::endl;

    auto bold = ansiBold();
    std::cout << "ansiBold: " << (!bold.empty() ? "pass" : "fail") << std::endl;

    auto dim = ansiDim();
    std::cout << "ansiDim: " << (!dim.empty() ? "pass" : "fail") << std::endl;
}

void testGetEnv() {
    using namespace clasp::color::detail;

    // Test getEnv with non-existent variable
    auto result = getEnv("NONEXISTENT_ENV_VAR_CLASP_TEST");
    std::cout << "getEnv non-existent: " << (!result.has_value() ? "pass" : "fail") << std::endl;
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

    std::cout << "\nok\n";
    return 0;
}
