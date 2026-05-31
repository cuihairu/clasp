#include <chrono>
#include <iostream>
#include <limits>
#include <string>

#include "clasp/detail/value_parse.hpp"

namespace {

int g_failures = 0;

void expect(bool cond, const char* label) {
    std::cout << label << ": " << (cond ? "pass" : "fail") << std::endl;
    if (!cond) ++g_failures;
}

void testTrimWs() {
    using namespace clasp::detail;

    // Test basic trimming
    auto result1 = trimWs("  hello  ");
    expect(result1 == "hello", "trimWs basic");

    // Test leading whitespace
    auto result2 = trimWs("\t\n  world");
    expect(result2 == "world", "trimWs leading");

    // Test trailing whitespace
    auto result3 = trimWs("test\r\n\t");
    expect(result3 == "test", "trimWs trailing");

    // Test empty string
    auto result4 = trimWs("");
    expect(result4.empty(), "trimWs empty");

    // Test all whitespace
    auto result5 = trimWs("   \t\n  ");
    expect(result5.empty(), "trimWs all ws");

    // Test no whitespace
    auto result6 = trimWs("nochange");
    expect(result6 == "nochange", "trimWs no ws");
}

void testTryParseBool() {
    using namespace clasp::detail;

    // Test true values
    bool out;
    expect(tryParseBool("1", out) && out, "tryParseBool '1'");
    expect(tryParseBool("true", out) && out, "tryParseBool 'true'");
    expect(tryParseBool("True", out) && out, "tryParseBool 'True'");
    expect(tryParseBool("TRUE", out) && out, "tryParseBool 'TRUE'");
    expect(tryParseBool("on", out) && out, "tryParseBool 'on'");
    expect(tryParseBool("yes", out) && out, "tryParseBool 'yes'");

    // Test false values
    expect(tryParseBool("0", out) && !out, "tryParseBool '0'");
    expect(tryParseBool("false", out) && !out, "tryParseBool 'false'");
    expect(tryParseBool("False", out) && !out, "tryParseBool 'False'");
    expect(tryParseBool("FALSE", out) && !out, "tryParseBool 'FALSE'");
    expect(tryParseBool("off", out) && !out, "tryParseBool 'off'");
    expect(tryParseBool("no", out) && !out, "tryParseBool 'no'");

    // Test with whitespace
    expect(tryParseBool(" true ", out) && out, "tryParseBool ' true '");

    // Test invalid values
    expect(!tryParseBool("invalid", out), "tryParseBool 'invalid'");
    expect(!tryParseBool("", out), "tryParseBool ''");
}

void testTryParseSignedInt() {
    using namespace clasp::detail;

    // Test int parsing
    int out;
    expect(tryParseSignedInt("42", out) && out == 42, "tryParseSignedInt '42'");
    expect(tryParseSignedInt("-42", out) && out == -42, "tryParseSignedInt '-42'");
    expect(tryParseSignedInt("0", out) && out == 0, "tryParseSignedInt '0'");

    // Test hex parsing
    expect(tryParseSignedInt("0xFF", out) && out == 255, "tryParseSignedInt '0xFF'");

    // Test int64_t parsing
    std::int64_t out64;
    expect(tryParseSignedInt("9223372036854775807", out64) && out64 == std::numeric_limits<std::int64_t>::max(),
           "tryParseSignedInt int64 max");

    // Test with whitespace
    expect(tryParseSignedInt(" 100 ", out) && out == 100, "tryParseSignedInt ' 100 '");

    // Test invalid values
    expect(!tryParseSignedInt("abc", out), "tryParseSignedInt 'abc'");
    expect(!tryParseSignedInt("", out), "tryParseSignedInt ''");
}

void testTryParseUnsignedInt() {
    using namespace clasp::detail;

    // Test unsigned parsing
    std::uint32_t out32;
    expect(tryParseUnsignedInt("42", out32) && out32 == 42, "tryParseUnsignedInt uint32 '42'");
    expect(tryParseUnsignedInt("0", out32) && out32 == 0, "tryParseUnsignedInt uint32 '0'");

    // Test uint64_t max
    std::uint64_t out64;
    expect(tryParseUnsignedInt("18446744073709551615", out64) && out64 == std::numeric_limits<std::uint64_t>::max(),
           "tryParseUnsignedInt uint64 max");

    // Test hex parsing
    expect(tryParseUnsignedInt("0xFF", out32) && out32 == 255, "tryParseUnsignedInt '0xFF'");

    // Test with whitespace
    expect(tryParseUnsignedInt(" 100 ", out32) && out32 == 100, "tryParseUnsignedInt ' 100 '");

    // Test negative values should fail
    expect(!tryParseUnsignedInt("-42", out32), "tryParseUnsignedInt '-42'");

    // Test invalid values
    expect(!tryParseUnsignedInt("abc", out32), "tryParseUnsignedInt 'abc'");
    expect(!tryParseUnsignedInt("", out32), "tryParseUnsignedInt ''");
}

void testTryParseFloat() {
    using namespace clasp::detail;

    // Test float parsing
    float outF;
    expect(tryParseFloat("3.14", outF) && outF > 3.13f && outF < 3.15f, "tryParseFloat float '3.14'");
    expect(tryParseFloat("-2.5", outF) && outF > -2.51f && outF < -2.49f, "tryParseFloat float '-2.5'");

    // Test double parsing
    double outD;
    expect(tryParseFloat("3.1415926535", outD) && outD > 3.14 && outD < 3.15, "tryParseFloat double '3.1415926535'");
    expect(tryParseFloat("1e-10", outD) && outD > 0 && outD < 1e-9, "tryParseFloat double '1e-10'");

    // Test with whitespace
    expect(tryParseFloat(" 2.5 ", outF) && outF > 2.49f && outF < 2.51f, "tryParseFloat ' 2.5 '");

    // Test invalid values
    expect(!tryParseFloat("abc", outF), "tryParseFloat 'abc'");
    expect(!tryParseFloat("", outF), "tryParseFloat ''");
}

void testTryParseDuration() {
    using namespace clasp::detail;
    using namespace std::chrono;

    // Test basic units
    milliseconds out;
    expect(tryParseDuration("100ms", out) && out == 100ms, "tryParseDuration '100ms'");
    expect(tryParseDuration("1s", out) && out == 1000ms, "tryParseDuration '1s'");
    expect(tryParseDuration("1m", out) && out == 60000ms, "tryParseDuration '1m'");
    expect(tryParseDuration("1h", out) && out == 3600000ms, "tryParseDuration '1h'");

    // Test small units
    expect(tryParseDuration("100ns", out) && out == 0ms, "tryParseDuration '100ns'");
    expect(tryParseDuration("100us", out) && out == 0ms, "tryParseDuration '100us'");

    // Test microseconds symbol
    expect(tryParseDuration("100µs", out) && out == 0ms, "tryParseDuration '100µs'");

    // Test negative duration
    expect(tryParseDuration("-1s", out) && out == -1000ms, "tryParseDuration '-1s'");
    expect(tryParseDuration("+500ms", out) && out == 500ms, "tryParseDuration '+500ms'");

    // Test compound duration
    expect(tryParseDuration("1h30m", out) && out == 5400000ms, "tryParseDuration '1h30m'");
    expect(tryParseDuration("1s500ms", out) && out == 1500ms, "tryParseDuration '1s500ms'");

    // Test zero
    expect(tryParseDuration("0", out) && out == 0ms, "tryParseDuration '0'");

    // Test decimal values
    expect(tryParseDuration("1.5s", out) && out == 1500ms, "tryParseDuration '1.5s'");
    expect(tryParseDuration("0.5ms", out) && out == 1ms, "tryParseDuration '0.5ms'");

    // Test invalid values
    expect(!tryParseDuration("", out), "tryParseDuration ''");
    expect(!tryParseDuration("invalid", out), "tryParseDuration 'invalid'");
    expect(!tryParseDuration("10", out), "tryParseDuration '10'");
}

void testParseDuration() {
    using namespace clasp::detail;
    using namespace std::chrono;

    // Test with valid input
    auto result1 = parseDuration("1s", milliseconds(0));
    expect(result1 == 1000ms, "parseDuration '1s'");

    // Test with invalid input (returns default)
    auto result2 = parseDuration("invalid", milliseconds(999));
    expect(result2 == 999ms, "parseDuration invalid default");

    // Test with empty input (returns default)
    auto result3 = parseDuration("", milliseconds(500));
    expect(result3 == 500ms, "parseDuration empty default");
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

    if (g_failures == 0) {
        std::cout << "\nok\n";
    }
    return g_failures == 0 ? 0 : 1;
}
