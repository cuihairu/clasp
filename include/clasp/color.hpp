#ifndef CLASP_COLOR_HPP
#define CLASP_COLOR_HPP

#include <cstdlib>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace clasp {

enum class ColorMode {
    Auto,
    Always,
    Never,
};

enum class ColorThemeName {
    Vscode,
    Sublime,
    Iterm2,
};

enum class ColorRole {
    Section,
    Command,
    Flag,
    Type,
    Meta,
    Error,
};

struct ColorTheme {
    std::string reset{"\x1b[0m"};
    std::string section;
    std::string command;
    std::string flag;
    std::string type;
    std::string meta;
    std::string error;
};

namespace color {

enum class Stream {
    Stdout,
    Stderr,
    Other,
};

// Returns true if the underlying stream is a terminal.
bool isTty(Stream stream);

// Windows: enables VT sequences for the console. Non-Windows: no-op returning true.
bool enableVirtualTerminalProcessing(Stream stream);

inline bool envNoColor() {
    // https://no-color.org/
    return std::getenv("NO_COLOR") != nullptr;
}

inline bool envTermDumb() {
    const char* term = std::getenv("TERM");
    return term != nullptr && std::string_view(term) == "dumb";
}

inline std::string ansiRgbFg(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
    return "\x1b[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
}

inline std::string ansiBold() { return "\x1b[1m"; }
inline std::string ansiDim() { return "\x1b[2m"; }

inline const ColorTheme& builtinTheme(ColorThemeName name) {
    // Note: these themes intentionally use ANSI truecolor for close matches. Terminals without truecolor
    // will typically approximate, and this feature is opt-in via Command::enableColor().
    static const ColorTheme vscode = [] {
        ColorTheme t;
        t.section = ansiBold() + ansiRgbFg(86, 156, 214);   // blue
        t.command = ansiRgbFg(78, 201, 176);               // teal
        t.flag = ansiRgbFg(156, 220, 254);                 // light blue
        t.type = ansiRgbFg(206, 145, 120);                 // orange
        t.meta = ansiDim() + ansiRgbFg(160, 160, 160);     // dim gray
        t.error = ansiBold() + ansiRgbFg(244, 71, 71);     // red
        return t;
    }();
    static const ColorTheme sublime = [] {
        ColorTheme t;
        t.section = ansiBold() + ansiRgbFg(249, 38, 114);  // pink
        t.command = ansiRgbFg(166, 226, 46);               // green
        t.flag = ansiRgbFg(102, 217, 239);                 // cyan
        t.type = ansiRgbFg(253, 151, 31);                  // orange
        t.meta = ansiDim() + ansiRgbFg(160, 160, 160);     // dim gray
        t.error = ansiBold() + ansiRgbFg(249, 38, 114);    // pink
        return t;
    }();
    static const ColorTheme iterm2 = [] {
        ColorTheme t;
        t.section = "\x1b[1m\x1b[36m"; // bold cyan
        t.command = "\x1b[32m";        // green
        t.flag = "\x1b[33m";           // yellow
        t.type = "\x1b[35m";           // magenta
        t.meta = "\x1b[2m";            // dim
        t.error = "\x1b[1m\x1b[31m";   // bold red
        return t;
    }();
    switch (name) {
        case ColorThemeName::Vscode: return vscode;
        case ColorThemeName::Sublime: return sublime;
        case ColorThemeName::Iterm2: return iterm2;
    }
    return vscode;
}

inline std::optional<ColorMode> parseMode(std::string_view s) {
    if (s == "auto") return ColorMode::Auto;
    if (s == "always") return ColorMode::Always;
    if (s == "never") return ColorMode::Never;
    return std::nullopt;
}

inline std::string_view modeName(ColorMode m) {
    switch (m) {
        case ColorMode::Auto: return "auto";
        case ColorMode::Always: return "always";
        case ColorMode::Never: return "never";
    }
    return "auto";
}

inline std::optional<ColorThemeName> parseTheme(std::string_view s) {
    if (s == "vscode") return ColorThemeName::Vscode;
    if (s == "sublime") return ColorThemeName::Sublime;
    if (s == "iterm2") return ColorThemeName::Iterm2;
    return std::nullopt;
}

inline std::string_view themeName(ColorThemeName t) {
    switch (t) {
        case ColorThemeName::Vscode: return "vscode";
        case ColorThemeName::Sublime: return "sublime";
        case ColorThemeName::Iterm2: return "iterm2";
    }
    return "vscode";
}

} // namespace color
} // namespace clasp

#endif // CLASP_COLOR_HPP
