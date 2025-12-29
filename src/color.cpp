#include "clasp/color.hpp"

#include <cstdio>

#if defined(_WIN32)
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace clasp::color {

bool isTty(Stream stream) {
#if defined(_WIN32)
    switch (stream) {
        case Stream::Stdout: return _isatty(_fileno(stdout)) != 0;
        case Stream::Stderr: return _isatty(_fileno(stderr)) != 0;
        case Stream::Other: return false;
    }
    return false;
#else
    switch (stream) {
        case Stream::Stdout: return ::isatty(fileno(stdout)) != 0;
        case Stream::Stderr: return ::isatty(fileno(stderr)) != 0;
        case Stream::Other: return false;
    }
    return false;
#endif
}

bool enableVirtualTerminalProcessing(Stream stream) {
#if defined(_WIN32)
    HANDLE h = nullptr;
    switch (stream) {
        case Stream::Stdout: h = GetStdHandle(STD_OUTPUT_HANDLE); break;
        case Stream::Stderr: h = GetStdHandle(STD_ERROR_HANDLE); break;
        case Stream::Other: return false;
    }
    if (h == nullptr || h == INVALID_HANDLE_VALUE) return false;

    DWORD mode = 0;
    if (!GetConsoleMode(h, &mode)) return false;

    // ENABLE_VIRTUAL_TERMINAL_PROCESSING: 0x0004
    constexpr DWORD kEnableVt = 0x0004;
    if ((mode & kEnableVt) != 0) return true;
    return SetConsoleMode(h, mode | kEnableVt) != 0;
#else
    (void)stream;
    return true;
#endif
}

} // namespace clasp::color
