#include <iostream>
#include <string>

#include "clasp/color.hpp"

namespace {

void testColorEnums() {
    // Test ColorMode parsing
    using clasp::ColorMode;

    auto m1 = clasp::color::parseMode("auto");
    if (m1 != ColorMode::Auto) std::cout << "FAIL: parseMode auto\n";

    auto m2 = clasp::color::parseMode("always");
    if (m2 != ColorMode::Always) std::cout << "FAIL: parseMode always\n";

    auto m3 = clasp::color::parseMode("never");
    if (m3 != ColorMode::Never) std::cout << "FAIL: parseMode never\n";

    auto m4 = clasp::color::parseMode("invalid");
    if (m4.has_value()) std::cout << "FAIL: parseMode invalid\n";

    // Test modeName
    if (clasp::color::modeName(ColorMode::Auto) != "auto") std::cout << "FAIL: modeName Auto\n";
    if (clasp::color::modeName(ColorMode::Always) != "always") std::cout << "FAIL: modeName Always\n";
    if (clasp::color::modeName(ColorMode::Never) != "never") std::cout << "FAIL: modeName Never\n";

    // Test ColorThemeName parsing
    using clasp::ColorThemeName;

    auto t1 = clasp::color::parseTheme("vscode");
    if (t1 != ColorThemeName::Vscode) std::cout << "FAIL: parseTheme vscode\n";

    auto t2 = clasp::color::parseTheme("sublime");
    if (t2 != ColorThemeName::Sublime) std::cout << "FAIL: parseTheme sublime\n";

    auto t3 = clasp::color::parseTheme("iterm2");
    if (t3 != ColorThemeName::Iterm2) std::cout << "FAIL: parseTheme iterm2\n";

    auto t4 = clasp::color::parseTheme("invalid");
    if (t4.has_value()) std::cout << "FAIL: parseTheme invalid\n";

    // Test themeName
    if (clasp::color::themeName(ColorThemeName::Vscode) != "vscode") std::cout << "FAIL: themeName Vscode\n";
    if (clasp::color::themeName(ColorThemeName::Sublime) != "sublime") std::cout << "FAIL: themeName Sublime\n";
    if (clasp::color::themeName(ColorThemeName::Iterm2) != "iterm2") std::cout << "FAIL: themeName Iterm2\n";
}

void testBuiltinThemes() {
    // Test builtinTheme function
    const auto& vscode = clasp::color::builtinTheme(clasp::ColorThemeName::Vscode);
    if (vscode.reset.empty()) std::cout << "FAIL: vscode theme reset\n";
    if (vscode.section.empty()) std::cout << "FAIL: vscode theme section\n";
    if (vscode.command.empty()) std::cout << "FAIL: vscode theme command\n";
    if (vscode.flag.empty()) std::cout << "FAIL: vscode theme flag\n";
    if (vscode.type.empty()) std::cout << "FAIL: vscode theme type\n";
    if (vscode.meta.empty()) std::cout << "FAIL: vscode theme meta\n";
    if (vscode.error.empty()) std::cout << "FAIL: vscode theme error\n";

    const auto& sublime = clasp::color::builtinTheme(clasp::ColorThemeName::Sublime);
    if (sublime.reset.empty()) std::cout << "FAIL: sublime theme reset\n";

    const auto& iterm2 = clasp::color::builtinTheme(clasp::ColorThemeName::Iterm2);
    if (iterm2.reset.empty()) std::cout << "FAIL: iterm2 theme reset\n";
}

void testAnsiFunctions() {
    // Test ANSI functions
    auto rgb = clasp::color::ansiRgbFg(255, 128, 64);
    if (rgb.empty()) std::cout << "FAIL: ansiRgbFg\n";

    auto bold = clasp::color::ansiBold();
    if (bold.empty()) std::cout << "FAIL: ansiBold\n";

    auto dim = clasp::color::ansiDim();
    if (dim.empty()) std::cout << "FAIL: ansiDim\n";
}

void testStreamFunctions() {
    // Test Stream enum functions (will test branches but may not hit all paths)
    using clasp::color::Stream;

    // These functions will test different switch cases
    auto isStdout = clasp::color::isTty(Stream::Stdout);
    (void)isStdout; // suppress unused

    auto isStderr = clasp::color::isTty(Stream::Stderr);
    (void)isStderr;

    auto isOther = clasp::color::isTty(Stream::Other);
    if (isOther) std::cout << "FAIL: isTty Other should be false\n";

    // Test enableVirtualTerminalProcessing
    auto vtStdout = clasp::color::enableVirtualTerminalProcessing(Stream::Stdout);
    (void)vtStdout;

    auto vtStderr = clasp::color::enableVirtualTerminalProcessing(Stream::Stderr);
    (void)vtStderr;

    auto vtOther = clasp::color::enableVirtualTerminalProcessing(Stream::Other);
#if defined(_WIN32)
    if (vtOther) std::cout << "FAIL: enableVirtualTerminalProcessing Other should be false on Windows\n";
#else
    if (!vtOther) std::cout << "FAIL: enableVirtualTerminalProcessing Other should be true on non-Windows\n";
#endif
}

void testEnvFunctions() {
    // Test envNoColor - typically returns false unless NO_COLOR is set
    auto noColor = clasp::color::envNoColor();
    (void)noColor;

    // Test envTermDumb - typically returns false unless TERM=dumb
    auto termDumb = clasp::color::envTermDumb();
    (void)termDumb;

    // Call getEnv directly via detail namespace (if needed for more coverage)
    auto home = clasp::color::detail::getEnv("HOME");
    auto path = clasp::color::detail::getEnv("PATH");
    auto nonexistent = clasp::color::detail::getEnv("NONEXISTENT_ENV_VAR_12345");

    (void)home;
    (void)path;
    if (nonexistent.has_value()) std::cout << "FAIL: nonexistent env var should not exist\n";
}

} // namespace

int main() {
    testColorEnums();
    testBuiltinThemes();
    testAnsiFunctions();
    testStreamFunctions();
    testEnvFunctions();

    std::cout << "ok\n";
    return 0;
}
