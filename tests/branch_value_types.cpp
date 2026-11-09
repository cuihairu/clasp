#include "branch_support.hpp"

// Narrow integer types: exercise the range checks inside tryParseSignedInt /
// tryParseUnsignedInt through parse<T>.
void testNarrowIntRanges() {
    std::vector<clasp::Flag> flags;
    flags.push_back(mkFlag("--n", "", 0));

    {
        auto p = makeParser({}, flags);
        p.setExternalValues({{"--n", "300"}});
        expect(p.getFlag<std::int8_t>("--n", 7) == 7, "int8 range rejected (high)");
        expect(p.getFlag<std::uint8_t>("--n", 7) == 7, "uint8 range rejected (high)");
    }
    {
        auto p = makeParser({}, flags);
        p.setExternalValues({{"--n", "-300"}});
        expect(p.getFlag<std::int8_t>("--n", 7) == 7, "int8 range rejected (low)");
    }
    {
        auto p = makeParser({}, flags);
        p.setExternalValues({{"--n", "-1"}});
        expect(p.getFlag<std::int16_t>("--n", 7) == -1, "int16 keeps sign check");
    }
    {
        auto p = makeParser({}, flags);
        p.setExternalValues({{"--n", "40000"}});
        expect(p.getFlag<std::int16_t>("--n", 7) == 7, "int16 range rejected");
    }
    {
        auto p = makeParser({}, flags);
        p.setExternalValues({{"--n", "70000"}});
        expect(p.getFlag<std::uint16_t>("--n", 7) == 7, "uint16 range rejected");
    }
    {
        auto p = makeParser({}, flags);
        p.setExternalValues({{"--n", "2147483648"}});
        expect(p.getFlag<std::int32_t>("--n", 7) == 7, "int32 range rejected");
    }
    {
        auto p = makeParser({}, flags);
        p.setExternalValues({{"--n", "42"}});
        expect(p.getFlag<std::int8_t>("--n", 7) == 42, "int8 accepts in-range");
        expect(p.getFlag<std::int16_t>("--n", 7) == 42, "int16 accepts in-range");
        expect(p.getFlag<std::uint8_t>("--n", 7) == 42, "uint8 accepts in-range");
        expect(p.getFlag<std::uint16_t>("--n", 7) == 42, "uint16 accepts in-range");
        expect(p.getFlag<std::int32_t>("--n", 7) == 42, "int32 accepts in-range");
    }
}

// ---------------------------------------------------------------------------
// External source validation (setExternalValuesChecked / ...MultiChecked).
// ---------------------------------------------------------------------------
// Full branch matrix per integer type instantiation.
template <typename T>
void signedMatrix(const char* label) {
    T v{};
    expect(!clasp::detail::tryParseSignedInt<T>("", v), label);
    expect(!clasp::detail::tryParseSignedInt<T>("7x", v), label);
    expect(!clasp::detail::tryParseSignedInt<T>("99999999999999999999", v), label);
    if constexpr (sizeof(T) <= 4) {
        expect(!clasp::detail::tryParseSignedInt<T>("99999999999", v), label);
        expect(!clasp::detail::tryParseSignedInt<T>("-99999999999", v), label);
    }
    expect(clasp::detail::tryParseSignedInt<T>("-5", v) && v == -5, label);
}

template <typename T>
void unsignedMatrix(const char* label) {
    T v{};
    expect(!clasp::detail::tryParseUnsignedInt<T>("", v), label);
    expect(!clasp::detail::tryParseUnsignedInt<T>("-1", v), label);
    expect(!clasp::detail::tryParseUnsignedInt<T>("7x", v), label);
    expect(!clasp::detail::tryParseUnsignedInt<T>("99999999999999999999", v), label);
    if constexpr (sizeof(T) <= 4) {
        expect(!clasp::detail::tryParseUnsignedInt<T>("99999999999", v), label);
    }
    expect(clasp::detail::tryParseUnsignedInt<T>("7", v) && v == 7, label);
}

// ---------------------------------------------------------------------------
// tryParseSignedInt / tryParseUnsignedInt / tryParseFloat / tryParseDuration.
void testValueParseDirect() {
    using namespace clasp::detail;

    std::int8_t s8{};
    std::int16_t s16{};
    int si{};
    expect(tryParseSignedInt<std::int8_t>("-128", s8) && s8 == -128, "signed min in range");
    expect(!tryParseSignedInt<std::int8_t>("", s8), "signed empty rejected");
    expect(!tryParseSignedInt<std::int8_t>("12x", s8), "signed trailing garbage");
    expect(!tryParseSignedInt<std::int8_t>("99999999999999999999", s8), "signed strtoll range");
    expect(!tryParseSignedInt<std::int8_t>("300", s8), "signed T-range high");
    expect(!tryParseSignedInt<std::int8_t>("-300", s8), "signed T-range low");
    expect(!tryParseSignedInt<std::int16_t>("40000", s16), "signed int16 T-range");
    expect(tryParseSignedInt<int>("  42  ", si) && si == 42, "signed trims whitespace");
    expect(tryParseSignedInt<int>("0x10", si) && si == 16, "signed base-0 hex");

    std::uint8_t u8{};
    std::uint64_t u64{};
    expect(tryParseUnsignedInt<std::uint8_t>("255", u8) && u8 == 255, "unsigned max in range");
    expect(!tryParseUnsignedInt<std::uint8_t>("", u8), "unsigned empty rejected");
    expect(!tryParseUnsignedInt<std::uint8_t>("-1", u8), "unsigned minus rejected");
    expect(!tryParseUnsignedInt<std::uint8_t>("5x", u8), "unsigned trailing garbage");
    expect(!tryParseUnsignedInt<std::uint8_t>("99999999999999999999", u8), "unsigned strtoull range");
    expect(!tryParseUnsignedInt<std::uint8_t>("300", u8), "unsigned T-range");
    expect(tryParseUnsignedInt<std::uint64_t>("0x10", u64) && u64 == 16, "unsigned base-0 hex");

    float f{};
    double d{};
    expect(tryParseFloat<float>("1.5", f) && f == 1.5f, "float ok");
    expect(!tryParseFloat<float>("", f), "float empty rejected");
    expect(!tryParseFloat<float>("1.5x", f), "float trailing garbage");
    expect(!tryParseFloat<float>("1e999", f), "float strtod range");
    expect(tryParseFloat<double>("-2.25", d) && d == -2.25, "double ok");
    expect(!tryParseFloat<double>("2.5x", d), "double trailing garbage");
    expect(!tryParseFloat<double>("1e999", d), "double strtod overflow");
    expect(!tryParseFloat<double>("1e-999", d), "double strtod underflow");

    std::chrono::milliseconds ms{};
    expect(tryParseDuration("0", ms) && ms.count() == 0, "duration zero");
    expect(tryParseDuration(" 5ms ", ms) && ms.count() == 5, "duration trims whitespace");
    expect(!tryParseDuration("", ms), "duration empty rejected");
    expect(!tryParseDuration("-", ms), "duration lone minus rejected");
    expect(!tryParseDuration("+", ms), "duration lone plus rejected");
    expect(tryParseDuration("1.5s", ms) && ms.count() == 1500, "duration fractional seconds");
    expect(!tryParseDuration("5x", ms), "duration bad unit");
    expect(!tryParseDuration("ms", ms), "duration missing number");
    expect(tryParseDuration("-2m", ms) && ms.count() == -120000, "duration negative minutes");
    expect(tryParseDuration("1h", ms) && ms.count() == 3600000, "duration hours");
    expect(tryParseDuration("1ns", ms) && ms.count() == 0, "duration nanoseconds round to zero");
    expect(tryParseDuration("1h30m", ms) && ms.count() == 5400000, "duration compound");
    expect(!tryParseDuration("0.1.2s", ms), "duration double dot rejected");

    signedMatrix<std::int8_t>("signed int8 matrix");
    signedMatrix<std::int16_t>("signed int16 matrix");
    signedMatrix<std::int32_t>("signed int32 matrix");
    signedMatrix<std::int64_t>("signed int64 matrix");
    signedMatrix<int>("signed int matrix");
    unsignedMatrix<std::uint8_t>("unsigned uint8 matrix");
    unsignedMatrix<std::uint16_t>("unsigned uint16 matrix");
    unsignedMatrix<std::uint32_t>("unsigned uint32 matrix");
    unsignedMatrix<std::uint64_t>("unsigned uint64 matrix");
}

// ---------------------------------------------------------------------------
// Byte-unit parsing: every unit family, plus rejection edges (tryParseBytes).
void testBytesUnits() {
    const struct {
        const char* in;
        std::uint64_t want;
    } valid[] = {
        {"1b", 1},           {"1k", 1024},      {"1kb", 1024},
        {"1ki", 1024},       {"1kib", 1024},    {"1m", 1024ULL * 1024},
        {"1mb", 1024ULL * 1024},                {"1mi", 1024ULL * 1024},
        {"1mib", 1024ULL * 1024},               {"1g", 1024ULL * 1024 * 1024},
        {"1gb", 1024ULL * 1024 * 1024},         {"1gi", 1024ULL * 1024 * 1024},
        {"1gib", 1024ULL * 1024 * 1024},        {"1t", 1024ULL * 1024 * 1024 * 1024},
        {"1tb", 1024ULL * 1024 * 1024 * 1024},  {"1ti", 1024ULL * 1024 * 1024 * 1024},
        {"1tib", 1024ULL * 1024 * 1024 * 1024},
        {"1p", 1024ULL * 1024 * 1024 * 1024 * 1024},
        {"1pb", 1024ULL * 1024 * 1024 * 1024 * 1024},
        {"1pi", 1024ULL * 1024 * 1024 * 1024 * 1024},
        {"1pib", 1024ULL * 1024 * 1024 * 1024 * 1024},
        {"1e", 1024ULL * 1024 * 1024 * 1024 * 1024 * 1024},
        {"1eb", 1024ULL * 1024 * 1024 * 1024 * 1024 * 1024},
        {"1ei", 1024ULL * 1024 * 1024 * 1024 * 1024 * 1024},
        {"1eib", 1024ULL * 1024 * 1024 * 1024 * 1024 * 1024},
        {"1.5k", 1536},      {"1.5", 2},        {"0x10", 16},
        {"100", 100},        {" 2kb ", 2048},
    };
    for (const auto& c : valid) {
        std::vector<clasp::Flag> flags;
        flags.push_back(annFlag("--size", "bytes", "true", std::uint64_t(0)));
        auto p = makeParser({}, flags);
        const auto err = p.setExternalValuesChecked({{"--size", c.in}});
        expect(!err.has_value() && p.getFlag<std::uint64_t>("--size") == c.want, "bytes accepts value");
    }

    const char* invalid[] = {"", "  ", "nank", "1e999k", "kb", "1e300", "5q", "-1", "1.2.3k"};
    for (const auto* in : invalid) {
        std::vector<clasp::Flag> flags;
        flags.push_back(annFlag("--size", "bytes", "true", std::uint64_t(0)));
        auto p = makeParser({}, flags);
        expect(p.setExternalValuesChecked({{"--size", in}}).has_value(), "bytes rejects value");
    }

    {
        std::vector<clasp::Flag> flags;
        flags.push_back(annFlag("--size", "bytes", "true", std::uint64_t(0)));
        auto p = makeParser({"--size", "2kib"}, flags);
        expect(p.ok() && p.getFlag<std::uint64_t>("--size") == 2048, "bytes via CLI");
    }
    {
        std::vector<clasp::Flag> flags;
        flags.push_back(annFlag("--size", "bytes", "true", std::uint64_t(0)));
        auto p = makeParser({"--size", "zz"}, flags);
        expect(!p.ok(), "bad bytes via CLI rejected");
    }
}

// ---------------------------------------------------------------------------
// IPv4/IPv6 canonicalization through an ip-annotated flag.
void testIPParsing() {
    const struct {
        const char* in;
        const char* canon;
    } valid[] = {
        {"1.2.3.4", "1.2.3.4"},
        {" 1.2.3.4 ", "1.2.3.4"},
        {"2001:db8::1", "2001:db8::1"},
        {"::", "::"},
        {"::1", "::1"},
        {"1::", "1::"},
        {"fe80::", "fe80::"},
        {"::ffff:1.2.3.4", "::ffff:102:304"},
        {"0:0:1:0:0:0:0:1", "0:0:1::1"},
        {"1:0:0:2:0:0:3:4", "1::2:0:0:3:4"},
        {"2001:db8:0:1:2:3:4:5", "2001:db8:0:1:2:3:4:5"},
        {"::9f", "::9f"},
        {"::9F", "::9f"},
    };
    for (const auto& c : valid) {
        std::vector<clasp::Flag> flags;
        flags.push_back(annFlag("--host", "ip", "true", std::string("")));
        auto p = makeParser({}, flags);
        const auto err = p.setExternalValuesChecked({{"--host", c.in}});
        expect(!err.has_value() && p.getFlag<std::string>("--host") == c.canon, "ip canonicalizes");
    }

    const char* invalid[] = {
        "", " ", " : ",       // empty after trim reaches IPv6 entry check
        "1..2.3",             // empty octet
        "1.2.3.4.5",          // too many octets
        "1a.2.3.4",           // non-digit octet
        "1.2.3",              // too few octets
        "1.2.3.256",          // octet overflow
        "12345::1",           // hextet too long
        "::1%",               // zone id
        "::1::2",             // double compression
        ":1:2:3:4:5:6:7",     // empty head part
        "1:2:3:4:5:6:7:",     // empty tail part
        "::ffff:1.2.3.999",   // bad embedded v4
        "::1.2.3.4:16",       // dotted group in non-final position
        "ffff:1.2.3.4:",      // v4-shaped head, wrong total
        "::1:2:3:4:5:6:7:8:9",// compression with too many groups
        "1:2:3:4:5:6:7:8::",  // compression with full groups
        "1:2:3",              // too few groups
        "1.2.3.4:80",         // dotted part inside IPv6 parse
        "1:2:3:4:5:6:7:8:9",  // no compression, too many groups
        "1.-2.3.4",           // octet starting with non-digit
        "::1.2.3.4:1.2.3.4",  // dotted part that is not the v4 tail
        "1::2:",              // empty tail part after compression
        "::1@2",              // char between '9' and 'A' inside a hextet
        "::1-2",              // char below '0' inside a hextet
        "bad_ip",             // plain junk
    };
    for (const auto* in : invalid) {
        std::vector<clasp::Flag> flags;
        flags.push_back(annFlag("--host", "ip", "true", std::string("")));
        auto p = makeParser({}, flags);
        expect(p.setExternalValuesChecked({{"--host", in}}).has_value(), "ip rejects value");
    }

    {
        std::vector<clasp::Flag> flags;
        flags.push_back(annFlag("--host", "ip", "true", std::string("")));
        auto p = makeParser({"--host", "2001:db8::1"}, flags);
        expect(p.ok() && p.getFlag<std::string>("--host") == "2001:db8::1", "ip via CLI");
    }
    {
        std::vector<clasp::Flag> flags;
        flags.push_back(annFlag("--host", "ip", "true", std::string("")));
        auto p = makeParser({"--host=bad_ip"}, flags);
        expect(!p.ok(), "bad ip via =value rejected");
    }
}

// ---------------------------------------------------------------------------
// Netmask and CIDR annotated flags.
void testIPMaskAndCIDR() {
    {
        std::vector<clasp::Flag> flags;
        flags.push_back(annFlag("--mask", "ipmask", "true", std::string("")));
        auto p = makeParser({}, flags);
        expect(!p.setExternalValuesChecked({{"--mask", "255.255.255.0"}}).has_value(), "mask contiguous ok");
        expect(!p.setExternalValuesChecked({{"--mask", "0.0.0.0"}}).has_value(), "mask zero ok");
        expect(!p.setExternalValuesChecked({{"--mask", "255.0.0.0"}}).has_value(), "mask single bit ok");
        expect(p.setExternalValuesChecked({{"--mask", "255.0.255.0"}}).has_value(), "mask non-contiguous");
        expect(p.setExternalValuesChecked({{"--mask", "300.0.0.0"}}).has_value(), "mask bad octet");
        expect(p.setExternalValuesChecked({{"--mask", "1.2.3.4"}}).has_value(), "mask scattered bits");
        expect(p.setExternalValuesChecked({{"--mask", "fe80::1"}}).has_value(), "mask rejects IPv6");
    }
    {
        std::vector<clasp::Flag> flags;
        flags.push_back(annFlag("--mask", "ipmask", "true", std::string("")));
        auto p = makeParser({}, flags);
        expect(!p.setExternalValuesChecked({{"--mask", ""}}).has_value(), "empty mask clears canonical");
        expect(p.getFlag<std::string>("--mask").empty(), "empty mask stays empty");
    }

    const struct {
        const char* in;
        const char* canon;
    } cidrValid[] = {
        {"10.0.0.0/8", "10.0.0.0/8"},
        {"10.0.0.99/24", "10.0.0.0/24"},
        {"10.0.0.99/8", "10.0.0.0/8"},
        {"1.2.3.4/0", "0.0.0.0/0"},
        {"2001:db8::/32", "2001:db8::/32"},
        {"2001:db8:1:2:3:4:5:6/128", "2001:db8:1:2:3:4:5:6/128"},
        {"2001:db8::/0", "::/0"},
        {" 10.0.0.0 / 8 ", "10.0.0.0/8"},
    };
    for (const auto& c : cidrValid) {
        std::vector<clasp::Flag> flags;
        flags.push_back(annFlag("--cidr", "cidr", "true", std::string("")));
        auto p = makeParser({}, flags);
        const auto err = p.setExternalValuesChecked({{"--cidr", c.in}});
        expect(!err.has_value() && p.getFlag<std::string>("--cidr") == c.canon, "cidr canonicalizes");
    }

    const char* cidrInvalid[] = {
        "10.0.0.1",        // no slash
        "/8",              // empty ip part
        "10.0.0.1/",       // empty prefix
        "10.0.0.1/8x",     // non-digit prefix
        "10.0.0.1/999",    // prefix over 128
        "10.0.0.1/33",     // prefix over v4 max
        "10.0.0.1/-8",     // prefix starting with non-digit
        "12345::1/8",      // hextet overflow inside the CIDR IPv6 path
        "2001:db8::/129",  // prefix over v6 max
        "300.0.0.0/8",     // bad IPv4 part
        "zz::1/8",         // bad IPv6 part
        "nope",            // no slash at all
    };
    for (const auto* in : cidrInvalid) {
        std::vector<clasp::Flag> flags;
        flags.push_back(annFlag("--cidr", "cidr", "true", std::string("")));
        auto p = makeParser({}, flags);
        expect(p.setExternalValuesChecked({{"--cidr", in}}).has_value(), "cidr rejects value");
    }

    {
        std::vector<clasp::Flag> flags;
        flags.push_back(annFlag("--net", "ipnet", "true", std::string("")));
        auto p = makeParser({}, flags);
        const auto err = p.setExternalValuesChecked({{"--net", "10.0.0.99/8"}});
        expect(!err.has_value() && p.getFlag<std::string>("--net") == "10.0.0.0/8", "ipnet canonicalizes");
        expect(p.setExternalValuesChecked({{"--net", "nope"}}).has_value(), "ipnet rejects value");
    }
}

// ---------------------------------------------------------------------------
// URL annotated flag canonicalization.
void testURLParsing() {
    const struct {
        const char* in;
        const char* canon;
    } valid[] = {
        {"HTTP://Example.COM:8080/path?a=1#frag", "http://example.com:8080/path?a=1#frag"},
        {"HTTPS://[2001:DB8::1]:443/x", "https://[2001:db8::1]:443/x"},
        {"ftp://user@Host.COM", "ftp://user@host.com"},
        {"http://host:80", "http://host:80"},
        {"http://a:b", "http://a:b"},
        {"http://host:80a", "http://host:80a"},
        {"http+dev.x-1://Host/path", "http+dev.x-1://host/path"},
        {"foo/bar", "foo/bar"},
        {"mailto:x@y.z", "mailto:x@y.z"},
        {"  http://ok.io  ", "http://ok.io"},
        {"HTTP://H:-1/X", "http://h:-1/X"},
    };
    for (const auto& c : valid) {
        std::vector<clasp::Flag> flags;
        flags.push_back(annFlag("--url", "url", "true", std::string("")));
        auto p = makeParser({}, flags);
        const auto err = p.setExternalValuesChecked({{"--url", c.in}});
        expect(!err.has_value() && p.getFlag<std::string>("--url") == c.canon, "url canonicalizes");
    }

    const char* invalid[] = {
        "ht tp://x",     // whitespace inside
        "://x",          // empty scheme
        "1http://x",     // scheme starts with digit
        "sc$heme://x",   // invalid scheme character
        "http://",       // empty authority
        "http://@",      // empty host after userinfo
        "http://[::1",   // unclosed bracket
        "a{://x",        // '{' fails the alpha upper bound inside the scheme
        "a_://x",        // '_' fails the 'Z' upper bound inside the scheme
        "a;://x",        // ';' fails the digit upper bound inside the scheme
    };
    for (const auto* in : invalid) {
        std::vector<clasp::Flag> flags;
        flags.push_back(annFlag("--url", "url", "true", std::string("")));
        auto p = makeParser({}, flags);
        expect(p.setExternalValuesChecked({{"--url", in}}).has_value(), "url rejects value");
    }

    {
        std::vector<clasp::Flag> flags;
        flags.push_back(annFlag("--url", "url", "true", std::string("")));
        auto p = makeParser({}, flags);
        expect(!p.setExternalValuesChecked({{"--url", ""}}).has_value(), "empty url accepted");
        expect(p.getFlag<std::string>("--url").empty(), "empty url clears canonical");
    }
}

// ---------------------------------------------------------------------------
// Annotation-driven key registration (count/bytes/ip/ipmask/cidr/ipnet/url).
void testAnnotationKinds() {
    const char* truthy[] = {"1", "true", "True", "TRUE", "yes", "on"};
    for (const auto* tv : truthy) {
        std::vector<clasp::Flag> flags;
        flags.push_back(annFlag("--size", "bytes", tv, std::uint64_t(0)));
        auto p = makeParser({}, flags);
        const auto err = p.setExternalValuesChecked({{"--size", "1k"}});
        expect(!err.has_value() && p.getFlag<std::uint64_t>("--size") == 1024, "truthy bytes annotation");
    }
    {
        const char* falsy[] = {"0", "false", "False", "FALSE", "no", "off", "bogus"};
        for (const auto* fv : falsy) {
            std::vector<clasp::Flag> flags;
            flags.push_back(annFlag("--cidr", "cidr", fv, std::string("")));
            auto p = makeParser({}, flags);
            const auto err = p.setExternalValuesChecked({{"--cidr", "10.0.0.99/8"}});
            expect(!err.has_value() && p.getFlag<std::string>("--cidr") == "10.0.0.99/8",
                   "falsy annotation skips normalization");
        }
    }
    {
        const char* kinds[] = {"bytes", "ip", "ipmask", "ipnet", "url"};
        for (const auto* kind : kinds) {
            std::vector<clasp::Flag> flags;
            flags.push_back(annFlag("--kv", kind, "bogus", std::string("")));
            auto p = makeParser({}, flags);
            expect(p.ok(), "falsy annotation leaves flag unregistered");
        }
    }
    {
        std::vector<clasp::Flag> flags;
        flags.push_back(annFlag("--host", "ip", "true", std::string("")));
        auto p = makeParser({}, flags);
        expect(!p.setExternalValuesChecked({{"--host", " 1.2.3.4 "}}).has_value() &&
                   p.getFlag<std::string>("--host") == "1.2.3.4",
               "ip annotation active");
    }
    {
        std::vector<clasp::Flag> flags;
        flags.push_back(annFlag("--mask", "ipmask", "1", std::string("")));
        auto p = makeParser({}, flags);
        expect(!p.setExternalValuesChecked({{"--mask", " 255.255.0.0 "}}).has_value() &&
                   p.getFlag<std::string>("--mask") == "255.255.0.0",
               "ipmask annotation active");
    }
    {
        std::vector<clasp::Flag> flags;
        flags.push_back(annFlag("--net", "ipnet", "yes", std::string("")));
        auto p = makeParser({}, flags);
        expect(!p.setExternalValuesChecked({{"--net", "10.0.0.99/8"}}).has_value() &&
                   p.getFlag<std::string>("--net") == "10.0.0.0/8",
               "ipnet annotation active");
    }
    {
        std::vector<clasp::Flag> flags;
        flags.push_back(annFlag("--url", "url", "on", std::string("")));
        auto p = makeParser({}, flags);
        expect(!p.setExternalValuesChecked({{"--url", "HTTP://X.io"}}).has_value() &&
                   p.getFlag<std::string>("--url") == "http://x.io",
               "url annotation active");
    }
    // count annotation: a count flag swallows the next token and emits "1".
    {
        clasp::Flag cnt = mkFlag("--cnt", "-c", 0);
        cnt.setAnnotation("count", "true");
        std::vector<clasp::Flag> flags;
        flags.push_back(cnt);
        auto p = makeParser({"--cnt", "5"}, flags);
        expect(p.ok() && p.getCount("--cnt") == 1 && p.positionals().size() == 1, "count flag ignores next token");
    }
    {
        clasp::Flag cnt = mkFlag("--cnt", "-c", 0);
        cnt.setAnnotation("count", "1");
        std::vector<clasp::Flag> flags;
        flags.push_back(cnt);
        auto p = makeParser({"-cc"}, flags);
        expect(p.ok() && p.getCount("--cnt") == 2, "count annotation '1' registers counting");
    }
    {
        clasp::Flag cnt = mkFlag("--cnt", "-c", 0);
        cnt.setAnnotation("count", "yes");
        std::vector<clasp::Flag> flags;
        flags.push_back(cnt);
        auto p = makeParser({"-c3"}, flags);
        expect(p.ok() && p.getCount("--cnt") == 3, "count annotation 'yes' registers counting");
    }
    {
        clasp::Flag cnt = mkFlag("--cnt", "-c", 0);
        cnt.setAnnotation("count", "false");
        std::vector<clasp::Flag> flags;
        flags.push_back(cnt);
        auto p = makeParser({"--cnt", "5"}, flags);
        expect(p.ok() && p.getCount("--cnt") == 5 && p.positionals().empty(),
               "falsy count annotation keeps plain flag");
    }
    // A flag with an empty long name registers nothing (not even its short).
    {
        std::vector<clasp::Flag> flags;
        flags.push_back(clasp::Flag("", "-z", "D", "v", std::string("")));
        auto p = makeParser({"-z"}, flags, allowUnknown());
        expect(p.ok(), "empty long name flag is not registered");
    }
    {
        std::vector<clasp::Flag> flags;
        flags.push_back(mkFlag("--lonely", "", std::string("")));
        auto p = makeParser({"--lonely", "x"}, flags);
        expect(p.ok() && p.getFlag<std::string>("--lonely") == "x", "long-only flag resolves");
    }
}

// ---------------------------------------------------------------------------
// Short-group parsing edges (unknown shorts, count remainders, value shorts).
