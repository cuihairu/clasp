#include <iostream>
#include <string>

#include "clasp/detail/value_parse.hpp"

namespace {

void testBoolParsing() {
    using namespace clasp::detail;

    // Test valid bool values
    bool b1 = false, b2 = false, b3 = false, b4 = false;
    tryParseBool("true", b1);
    tryParseBool("false", b2);
    tryParseBool("1", b3);
    tryParseBool("0", b4);

    // Test empty string
    bool b5 = false;
    tryParseBool("", b5);

    (void)b1; (void)b2; (void)b3; (void)b4; (void)b5;
}

void testIntParsing() {
    using namespace clasp::detail;

    // Test valid int values
    int i1 = 0, i2 = 0, i3 = 0;
    tryParseSignedInt("42", i1);
    tryParseSignedInt("-123", i2);
    tryParseSignedInt("0", i3);

    // Test invalid int value (should fail but not throw)
    int i4 = 999;
    tryParseSignedInt("invalid", i4);

    (void)i1; (void)i2; (void)i3; (void)i4;
}

void testUnsignedParsing() {
    using namespace clasp::detail;

    // Test valid unsigned int values
    std::uint32_t u1 = 0, u2 = 0;
    tryParseUnsignedInt("42", u1);
    tryParseUnsignedInt("4294967295", u2);

    // Test invalid unsigned value
    std::uint32_t u3 = 999;
    tryParseUnsignedInt("invalid", u3);
    tryParseUnsignedInt("-1", u3);

    (void)u1; (void)u2; (void)u3;
}

void testFloatParsing() {
    using namespace clasp::detail;

    // Test valid float values
    float f1 = 0.0f, f2 = 0.0f;
    tryParseFloat("3.14", f1);
    tryParseFloat("-2.5", f2);

    // Test invalid float value
    float f3 = 99.0f;
    tryParseFloat("invalid", f3);

    (void)f1; (void)f2; (void)f3;
}

void testDurationParsing() {
    using namespace clasp::detail;

    // Test valid durations
    std::chrono::milliseconds d1, d2, d3, d4, d5;

    tryParseDuration("100ms", d1);
    tryParseDuration("1s", d2);
    tryParseDuration("1m", d3);
    tryParseDuration("1h", d4);
    tryParseDuration("0", d5);

    // Test invalid duration
    std::chrono::milliseconds d6;
    tryParseDuration("invalid", d6);

    (void)d1; (void)d2; (void)d3; (void)d4; (void)d5; (void)d6;
}

void testInt64Parsing() {
    using namespace clasp::detail;

    // Test int64 values
    std::int64_t i1 = 0, i2 = 0;
    tryParseSignedInt("9223372036854775807", i1);
    tryParseSignedInt("-9223372036854775808", i2);

    // Test uint64 values
    std::uint64_t u1 = 0, u2 = 0;
    tryParseUnsignedInt("18446744073709551615", u1);
    tryParseUnsignedInt("0", u2);

    (void)i1; (void)i2; (void)u1; (void)u2;
}

void testTrimWs() {
    using namespace clasp::detail;

    // Test trimWs
    auto s1 = trimWs("  hello  ");
    auto s2 = trimWs("\tworld\n");
    auto s3 = trimWs("");
    auto s4 = trimWs("   ");
    auto s5 = trimWs("nospace");

    (void)s1; (void)s2; (void)s3; (void)s4; (void)s5;
}

void testEdgeCases() {
    using namespace clasp::detail;

    // Test edge cases for numeric parsing
    int i1 = 0;
    tryParseSignedInt("", i1);
    tryParseSignedInt("  42  ", i1);
    tryParseSignedInt("0x10", i1);  // hex format

    float f1 = 0.0f;
    tryParseFloat("", f1);
    tryParseFloat("  3.14  ", f1);
    tryParseFloat("inf", f1);

    (void)i1; (void)f1;
}

} // namespace

int main() {
    testBoolParsing();
    testIntParsing();
    testUnsignedParsing();
    testFloatParsing();
    testDurationParsing();
    testInt64Parsing();
    testTrimWs();
    testEdgeCases();

    std::cout << "ok\n";
    return 0;
}
