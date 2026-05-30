#include <chrono>
#include <iostream>
#include <limits>
#include <string>

#include "clasp/detail/value_parse.hpp"

namespace {

void testTrimWs() {
    using namespace clasp::detail;

    // Test basic trimming
    auto result1 = trimWs("  hello  ");
    std::cout << "trimWs basic: " << (result1 == "hello" ? "pass" : "fail") << std::endl;

    // Test leading whitespace
    auto result2 = trimWs("\t\n  world");
    std::cout << "trimWs leading: " << (result2 == "world" ? "pass" : "fail") << std::endl;

    // Test trailing whitespace
    auto result3 = trimWs("test\r\n\t");
    std::cout << "trimWs trailing: " << (result3 == "test" ? "pass" : "fail") << std::endl;

    // Test empty string
    auto result4 = trimWs("");
    std::cout << "trimWs empty: " << (result4.empty() ? "pass" : "fail") << std::endl;

    // Test all whitespace
    auto result5 = trimWs("   \t\n  ");
    std::cout << "trimWs all ws: " << (result5.empty() ? "pass" : "fail") << std::endl;

    // Test no whitespace
    auto result6 = trimWs("nochange");
    std::cout << "trimWs no ws: " << (result6 == "nochange" ? "pass" : "fail") << std::endl;
}

void testTryParseBool() {
    using namespace clasp::detail;

    // Test true values
    bool out;
    std::cout << "tryParseBool '1': " << (tryParseBool("1", out) && out ? "pass" : "fail") << std::endl;
    std::cout << "tryParseBool 'true': " << (tryParseBool("true", out) && out ? "pass" : "fail") << std::endl;
    std::cout << "tryParseBool 'True': " << (tryParseBool("True", out) && out ? "pass" : "fail") << std::endl;
    std::cout << "tryParseBool 'TRUE': " << (tryParseBool("TRUE", out) && out ? "pass" : "fail") << std::endl;
    std::cout << "tryParseBool 'on': " << (tryParseBool("on", out) && out ? "pass" : "fail") << std::endl;
    std::cout << "tryParseBool 'yes': " << (tryParseBool("yes", out) && out ? "pass" : "fail") << std::endl;

    // Test false values
    std::cout << "tryParseBool '0': " << (tryParseBool("0", out) && !out ? "pass" : "fail") << std::endl;
    std::cout << "tryParseBool 'false': " << (tryParseBool("false", out) && !out ? "pass" : "fail") << std::endl;
    std::cout << "tryParseBool 'False': " << (tryParseBool("False", out) && !out ? "pass" : "fail") << std::endl;
    std::cout << "tryParseBool 'FALSE': " << (tryParseBool("FALSE", out) && !out ? "pass" : "fail") << std::endl;
    std::cout << "tryParseBool 'off': " << (tryParseBool("off", out) && !out ? "pass" : "fail") << std::endl;
    std::cout << "tryParseBool 'no': " << (tryParseBool("no", out) && !out ? "pass" : "fail") << std::endl;

    // Test with whitespace
    std::cout << "tryParseBool ' true ': " << (tryParseBool(" true ", out) && out ? "pass" : "fail") << std::endl;

    // Test invalid values
    std::cout << "tryParseBool 'invalid': " << (!tryParseBool("invalid", out) ? "pass" : "fail") << std::endl;
    std::cout << "tryParseBool '': " << (!tryParseBool("", out) ? "pass" : "fail") << std::endl;
}

void testTryParseSignedInt() {
    using namespace clasp::detail;

    // Test int parsing
    int out;
    std::cout << "tryParseSignedInt '42': " << (tryParseSignedInt("42", out) && out == 42 ? "pass" : "fail") << std::endl;
    std::cout << "tryParseSignedInt '-42': " << (tryParseSignedInt("-42", out) && out == -42 ? "pass" : "fail") << std::endl;
    std::cout << "tryParseSignedInt '0': " << (tryParseSignedInt("0", out) && out == 0 ? "pass" : "fail") << std::endl;

    // Test hex parsing
    std::cout << "tryParseSignedInt '0xFF': " << (tryParseSignedInt("0xFF", out) && out == 255 ? "pass" : "fail") << std::endl;

    // Test int64_t parsing
    std::int64_t out64;
    std::cout << "tryParseSignedInt int64 '9223372036854775807': "
              << (tryParseSignedInt("9223372036854775807", out64) && out64 == std::numeric_limits<std::int64_t>::max() ? "pass" : "fail") << std::endl;

    // Test with whitespace
    std::cout << "tryParseSignedInt ' 100 ': " << (tryParseSignedInt(" 100 ", out) && out == 100 ? "pass" : "fail") << std::endl;

    // Test invalid values
    std::cout << "tryParseSignedInt 'abc': " << (!tryParseSignedInt("abc", out) ? "pass" : "fail") << std::endl;
    std::cout << "tryParseSignedInt '': " << (!tryParseSignedInt("", out) ? "pass" : "fail") << std::endl;
}

void testTryParseUnsignedInt() {
    using namespace clasp::detail;

    // Test unsigned parsing
    std::uint32_t out32;
    std::cout << "tryParseUnsignedInt uint32 '42': " << (tryParseUnsignedInt("42", out32) && out32 == 42 ? "pass" : "fail") << std::endl;
    std::cout << "tryParseUnsignedInt uint32 '0': " << (tryParseUnsignedInt("0", out32) && out32 == 0 ? "pass" : "fail") << std::endl;

    // Test uint64_t max
    std::uint64_t out64;
    std::cout << "tryParseUnsignedInt uint64 max: "
              << (tryParseUnsignedInt("18446744073709551615", out64) && out64 == std::numeric_limits<std::uint64_t>::max() ? "pass" : "fail") << std::endl;

    // Test hex parsing
    std::cout << "tryParseUnsignedInt '0xFF': " << (tryParseUnsignedInt("0xFF", out32) && out32 == 255 ? "pass" : "fail") << std::endl;

    // Test with whitespace
    std::cout << "tryParseUnsignedInt ' 100 ': " << (tryParseUnsignedInt(" 100 ", out32) && out32 == 100 ? "pass" : "fail") << std::endl;

    // Test negative values should fail
    std::cout << "tryParseUnsignedInt '-42': " << (!tryParseUnsignedInt("-42", out32) ? "pass" : "fail") << std::endl;

    // Test invalid values
    std::cout << "tryParseUnsignedInt 'abc': " << (!tryParseUnsignedInt("abc", out32) ? "pass" : "fail") << std::endl;
    std::cout << "tryParseUnsignedInt '': " << (!tryParseUnsignedInt("", out32) ? "pass" : "fail") << std::endl;
}

void testTryParseFloat() {
    using namespace clasp::detail;

    // Test float parsing
    float outF;
    std::cout << "tryParseFloat float '3.14': " << (tryParseFloat("3.14", outF) && outF > 3.13f && outF < 3.15f ? "pass" : "fail") << std::endl;
    std::cout << "tryParseFloat float '-2.5': " << (tryParseFloat("-2.5", outF) && outF > -2.51f && outF < -2.49f ? "pass" : "fail") << std::endl;

    // Test double parsing
    double outD;
    std::cout << "tryParseFloat double '3.1415926535': " << (tryParseFloat("3.1415926535", outD) && outD > 3.14 && outD < 3.15 ? "pass" : "fail") << std::endl;
    std::cout << "tryParseFloat double '1e-10': " << (tryParseFloat("1e-10", outD) && outD > 0 && outD < 1e-9 ? "pass" : "fail") << std::endl;

    // Test with whitespace
    std::cout << "tryParseFloat ' 2.5 ': " << (tryParseFloat(" 2.5 ", outF) && outF > 2.49f && outF < 2.51f ? "pass" : "fail") << std::endl;

    // Test invalid values
    std::cout << "tryParseFloat 'abc': " << (!tryParseFloat("abc", outF) ? "pass" : "fail") << std::endl;
    std::cout << "tryParseFloat '': " << (!tryParseFloat("", outF) ? "pass" : "fail") << std::endl;
}

void testTryParseDuration() {
    using namespace clasp::detail;
    using namespace std::chrono;

    // Test basic units
    milliseconds out;
    std::cout << "tryParseDuration '100ms': " << (tryParseDuration("100ms", out) && out == 100ms ? "pass" : "fail") << std::endl;
    std::cout << "tryParseDuration '1s': " << (tryParseDuration("1s", out) && out == 1000ms ? "pass" : "fail") << std::endl;
    std::cout << "tryParseDuration '1m': " << (tryParseDuration("1m", out) && out == 60000ms ? "pass" : "fail") << std::endl;
    std::cout << "tryParseDuration '1h': " << (tryParseDuration("1h", out) && out == 3600000ms ? "pass" : "fail") << std::endl;

    // Test small units
    std::cout << "tryParseDuration '100ns': " << (tryParseDuration("100ns", out) && out == 0ms ? "pass" : "fail") << std::endl;
    std::cout << "tryParseDuration '100us': " << (tryParseDuration("100us", out) && out == 0ms ? "pass" : "fail") << std::endl;

    // Test microseconds symbol
    std::cout << "tryParseDuration '100µs': " << (tryParseDuration("100µs", out) && out == 0ms ? "pass" : "fail") << std::endl;

    // Test negative duration
    std::cout << "tryParseDuration '-1s': " << (tryParseDuration("-1s", out) && out == -1000ms ? "pass" : "fail") << std::endl;
    std::cout << "tryParseDuration '+500ms': " << (tryParseDuration("+500ms", out) && out == 500ms ? "pass" : "fail") << std::endl;

    // Test compound duration
    std::cout << "tryParseDuration '1h30m': " << (tryParseDuration("1h30m", out) && out == 5400000ms ? "pass" : "fail") << std::endl;
    std::cout << "tryParseDuration '1s500ms': " << (tryParseDuration("1s500ms", out) && out == 1500ms ? "pass" : "fail") << std::endl;

    // Test zero
    std::cout << "tryParseDuration '0': " << (tryParseDuration("0", out) && out == 0ms ? "pass" : "fail") << std::endl;

    // Test decimal values
    std::cout << "tryParseDuration '1.5s': " << (tryParseDuration("1.5s", out) && out == 1500ms ? "pass" : "fail") << std::endl;
    std::cout << "tryParseDuration '0.5ms': " << (tryParseDuration("0.5ms", out) && out == 1ms ? "pass" : "fail") << std::endl;

    // Test invalid values
    std::cout << "tryParseDuration '': " << (!tryParseDuration("", out) ? "pass" : "fail") << std::endl;
    std::cout << "tryParseDuration 'invalid': " << (!tryParseDuration("invalid", out) ? "pass" : "fail") << std::endl;
    std::cout << "tryParseDuration '10': " << (!tryParseDuration("10", out) ? "pass" : "fail") << std::endl;  // No unit
}

void testParseDuration() {
    using namespace clasp::detail;
    using namespace std::chrono;

    // Test with valid input
    auto result1 = parseDuration("1s", milliseconds(0));
    std::cout << "parseDuration '1s': " << (result1 == 1000ms ? "pass" : "fail") << std::endl;

    // Test with invalid input (returns default)
    auto result2 = parseDuration("invalid", milliseconds(999));
    std::cout << "parseDuration invalid default: " << (result2 == 999ms ? "pass" : "fail") << std::endl;

    // Test with empty input (returns default)
    auto result3 = parseDuration("", milliseconds(500));
    std::cout << "parseDuration empty default: " << (result3 == 500ms ? "pass" : "fail") << std::endl;
}

} // namespace

int main() {
    std::cout << "=== Testing trimWs ===" << std::endl;
    testTrimWs();

    std::cout << "\n=== Testing tryParseBool ===" << std::endl;
    testTryParseBool();

    std::cout << "\n=== Testing tryParseSignedInt ===" << std::endl;
    testTryParseSignedInt();

    std::cout << "\n=== Testing tryParseUnsignedInt ===" << std::endl;
    testTryParseUnsignedInt();

    std::cout << "\n=== Testing tryParseFloat ===" << std::endl;
    testTryParseFloat();

    std::cout << "\n=== Testing tryParseDuration ===" << std::endl;
    testTryParseDuration();

    std::cout << "\n=== Testing parseDuration ===" << std::endl;
    testParseDuration();

    std::cout << "\nok\n";
    return 0;
}
