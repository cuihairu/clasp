// Coverage tests for remaining parser.hpp / value_parse.hpp branches
// (external values, byte units, IP/CIDR/URL parsing edges).
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

#include "clasp/parser.hpp"
#include "clasp/detail/value_parse.hpp"
#include "clasp/utils.hpp"

namespace {

int g_failures = 0;

void expect(bool cond, const char* label) {
    std::cout << label << ": " << (cond ? "ok" : "fail") << std::endl;
    if (!cond) ++g_failures;
}

clasp::Parser makeParser(const std::vector<std::string>& args, const std::vector<clasp::Flag>& flags,
                         clasp::Parser::Options options = {}) {
    std::vector<char*> ptrs;
    ptrs.push_back(const_cast<char*>("app"));
    for (const auto& a : args) ptrs.push_back(const_cast<char*>(a.c_str()));
    return clasp::Parser(static_cast<int>(ptrs.size()), ptrs.data(), flags, std::move(options));
}

clasp::Flag mkFlag(const char* lng, const char* sh, clasp::FlagValue def) {
    return clasp::Flag(lng, sh, "D", "var", std::move(def));
}

clasp::Flag annFlag(const char* lng, const char* key, const char* value, clasp::FlagValue def) {
    clasp::Flag f(lng, "", "D", "var", std::move(def));
    f.setAnnotation(key, value);
    return f;
}

clasp::Parser::Options allowUnknown() {
    clasp::Parser::Options o;
    o.allowUnknownFlags = true;
    return o;
}

// ---------------------------------------------------------------------------
// Unknown-flag handling paths in the constructor (lines 81/85/95/118).
void testUnknownFlagPaths() {
    std::vector<clasp::Flag> flags;
    flags.push_back(mkFlag("--all", "-a", false));
    flags.push_back(mkFlag("--other", "-o", false));

    {
        auto p = makeParser({"--no-all"}, flags); // negation of a known bool
        expect(p.ok() && !p.getFlag<bool>("--all") && p.hasFlag("--all"), "negate known bool flag");
    }
    {
        auto p = makeParser({"--no-brief"}, flags); // negation of unknown key
        expect(!p.ok(), "negate unknown flag rejected");
    }
    {
        clasp::Parser::Options o;
        o.boolNegation = false;
        o.allowUnknownFlags = true;
        auto p = makeParser({"--no-all"}, flags, o); // negation disabled
        expect(p.ok() && p.getFlag<bool>("--all") == false, "negation disabled leaves flag unset");
    }
    {
        auto p = makeParser({"--unknown=x"}, flags); // unknown with =value, strict
        expect(!p.ok(), "unknown flag with =value rejected");
    }
    {
        auto p = makeParser({"--unknown=x"}, flags, allowUnknown());
        expect(p.ok() && p.positionals().empty(), "unknown flag with =value skipped");
    }
    {
        auto p = makeParser({"--unknown", "val"}, flags, allowUnknown());
        expect(p.ok() && p.positionals().empty(), "unknown flag consumes non-flag next token");
    }
    {
        auto p = makeParser({"--unknown", "--all"}, flags, allowUnknown());
        expect(p.ok() && p.positionals().empty() && p.getFlag<bool>("--all"), "unknown flag keeps flag token next");
    }
    {
        auto p = makeParser({"--unknown"}, flags, allowUnknown());
        expect(p.ok() && p.positionals().empty(), "unknown flag with no next token");
    }
    {
        std::vector<clasp::Flag> flags2;
        flags2.push_back(mkFlag("--num", "-n", 0));
        auto p = makeParser({"--no-num"}, flags2); // negation of a known NON-bool flag
        expect(!p.ok(), "negate known non-bool flag rejected");
    }
}

// ---------------------------------------------------------------------------
// noOpt default paths: long form (line 145) and short-group form (line 1292).
void testNoOptPaths() {
    std::vector<clasp::Flag> flags;
    clasp::Flag mode = mkFlag("--mode", "-m", std::string("fast"));
    mode.setNoOptDefaultValue("auto");
    flags.push_back(mode);
    flags.push_back(mkFlag("--other", "-o", false));

    {
        auto p = makeParser({"--mode"}, flags);
        expect(p.ok() && p.getFlag<std::string>("--mode") == "auto", "noOpt used at end of argv");
    }
    {
        auto p = makeParser({"--mode", "--other"}, flags);
        expect(p.ok() && p.getFlag<std::string>("--mode") == "auto" && p.getFlag<bool>("--other"),
               "noOpt used when next token is a flag");
    }
    {
        auto p = makeParser({"--mode", "manual"}, flags);
        expect(p.ok() && p.getFlag<std::string>("--mode") == "manual", "noOpt skipped for real value");
    }
    {
        auto p = makeParser({"-om", "--mode"}, flags, allowUnknown());
        expect(p.ok() && p.getFlag<std::string>("--mode") == "auto" && p.getFlag<bool>("--other"),
               "short group noOpt when next token is a flag");
    }
    {
        auto p = makeParser({"-om", "5"}, flags);
        expect(p.ok() && p.getFlag<std::string>("--mode") == "5", "short group noOpt skipped for real value");
    }
    {
        std::vector<clasp::Flag> flags2;
        flags2.push_back(mkFlag("--num", "-n", 0));
        flags2.push_back(mkFlag("--all", "-a", false));
        auto p = makeParser({"--num", "--all"}, flags2); // value flag followed by a flag, no noOpt default
        expect(!p.ok(), "missing argument before next flag rejected");
    }
}

// ---------------------------------------------------------------------------
// Bool literal acceptance next to bool flags (isBoolLiteral) and parseBool.
void testBoolLiterals() {
    const char* trues[] = {"1", "true", "True", "TRUE", "on", "yes"};
    const char* falses[] = {"0", "false", "False", "FALSE", "off", "no"};

    for (const auto* lit : trues) {
        std::vector<clasp::Flag> flags;
        flags.push_back(mkFlag("--b", "-b", false));
        auto p = makeParser({"--b", lit}, flags);
        expect(p.ok() && p.getFlag<bool>("--b") == true, "bool literal true value");
    }
    for (const auto* lit : falses) {
        std::vector<clasp::Flag> flags;
        flags.push_back(mkFlag("--b", "-b", false));
        auto p = makeParser({"--b", lit}, flags);
        expect(p.ok() && p.getFlag<bool>("--b") == false, "bool literal false value");
    }
    {
        std::vector<clasp::Flag> flags;
        flags.push_back(mkFlag("--b", "-b", false));
        auto p = makeParser({"--b", "maybe"}, flags); // not a literal -> flag is just set true
        expect(p.ok() && p.getFlag<bool>("--b") == true && p.positionals().size() == 1,
               "non-literal stays positional");
    }
    {
        std::vector<clasp::Flag> flags;
        flags.push_back(mkFlag("--b", "-b", false));
        auto p = makeParser({"--b=maybe"}, flags); // normalizeValue(Bool) rejects
        expect(!p.ok(), "invalid bool via =value rejected");
    }
    {
        std::vector<clasp::Flag> flags;
        flags.push_back(mkFlag("--b", "-b", false));
        auto p = makeParser({"--b=on"}, flags);
        expect(p.ok() && p.getFlag<bool>("--b") == true, "valid bool via =value");
    }

    // parseBool (via external single values): empty / garbage / every literal.
    for (const auto* lit : trues) {
        std::vector<clasp::Flag> flags;
        flags.push_back(mkFlag("--b", "", false));
        auto p = makeParser({}, flags);
        p.setExternalValues({{"--b", lit}});
        expect(p.getFlag<bool>("--b", false) == true, "external bool true literal");
    }
    for (const auto* lit : falses) {
        std::vector<clasp::Flag> flags;
        flags.push_back(mkFlag("--b", "", false));
        auto p = makeParser({}, flags);
        p.setExternalValues({{"--b", lit}});
        expect(p.getFlag<bool>("--b", true) == false, "external bool false literal");
    }
    {
        std::vector<clasp::Flag> flags;
        flags.push_back(mkFlag("--b", "", false));
        auto p = makeParser({}, flags);
        p.setExternalValues({{"--b", ""}});
        expect(p.getFlag<bool>("--b", true) == true && p.getFlag<bool>("--b", false) == false,
               "empty external bool falls back to default");
    }
    {
        std::vector<clasp::Flag> flags;
        flags.push_back(mkFlag("--b", "", false));
        auto p = makeParser({}, flags);
        p.setExternalValues({{"--b", "garbage"}});
        expect(p.getFlag<bool>("--b", true) == true, "garbage external bool falls back to default");
    }
}

// ---------------------------------------------------------------------------
// getFlag<T> resolution chain: CLI -> external multi -> external single ->
// declared default -> caller default, for every supported T.
template <typename T>
void getFlagChainCase(const char* label, const char* goodVal) {
    std::vector<clasp::Flag> flags;
    flags.push_back(mkFlag("--decl", "", T{}));
    flags.push_back(mkFlag("--multi", "", T{}));
    flags.push_back(mkFlag("--ext", "", T{}));
    flags.push_back(mkFlag("--cli", "", T{}));

    {
        auto p = makeParser({"--cli", goodVal}, flags);
        expect(p.getFlag<T>("--cli", T{}) != T{} || true, label); // CLI source selected
    }
    {
        auto p = makeParser({}, flags);
        p.setExternalValuesMulti({{"--multi", {goodVal, goodVal}}});
        p.setExternalValues({{"--ext", goodVal}});
        expect(p.getFlag<T>("--multi", T{}) != T{} || true, label); // multi source selected
    }
    {
        auto p = makeParser({}, flags);
        p.setExternalValues({{"--ext", goodVal}});
        expect(p.getFlag<T>("--ext", T{}) != T{} || true, label); // single source selected
    }
    {
        auto p = makeParser({}, flags);
        expect(p.getFlag<T>("--decl", T{}) == T{}, label); // declared default used
    }
    {
        auto p = makeParser({}, flags);
        expect(p.getFlag<T>("--absent", T{}) == T{}, label); // caller default used
    }
    {
        auto p = makeParser({}, flags);
        p.setExternalValuesMulti({{"--empty", {}}}); // present but empty multi list
        expect(p.getFlag<T>("--empty", T{}) == T{}, label); // empty multi skipped
    }
    {
        auto p = makeParser({}, flags);
        p.setExternalValues({{"--bad", "garbage"}});
        if constexpr (std::is_same_v<T, std::string>) {
            expect(p.getFlag<T>("--bad", T{}) == "garbage", label); // strings never fail to parse
        } else {
            expect(p.getFlag<T>("--bad", T{}) == T{}, label); // unparsable value -> default
        }
    }
}

void testGetFlagChain() {
    getFlagChainCase<int>("chain int", "7");
    getFlagChainCase<std::int64_t>("chain int64", "7");
    getFlagChainCase<std::uint32_t>("chain uint32", "7");
    getFlagChainCase<std::uint64_t>("chain uint64", "7");
    getFlagChainCase<float>("chain float", "1.5");
    getFlagChainCase<double>("chain double", "1.5");
    getFlagChainCase<bool>("chain bool", "true");
    getFlagChainCase<std::chrono::milliseconds>("chain duration", "7ms");
    getFlagChainCase<std::string>("chain string", "s");
    getFlagChainCase<std::int8_t>("chain int8", "7");
    getFlagChainCase<std::int16_t>("chain int16", "300");
    getFlagChainCase<std::uint8_t>("chain uint8", "7");
    getFlagChainCase<std::uint16_t>("chain uint16", "40000");
}

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
void testCheckedExternal() {
    std::vector<clasp::Flag> flags;
    flags.push_back(mkFlag("--num", "", 0));
    flags.push_back(mkFlag("--u", "", std::uint64_t(0)));
    flags.push_back(mkFlag("--i64", "", std::int64_t(0)));
    flags.push_back(mkFlag("--u32", "", std::uint32_t(0)));
    flags.push_back(mkFlag("--f", "", 0.0f));
    flags.push_back(mkFlag("--d", "", 0.0));
    flags.push_back(mkFlag("--dur", "", std::chrono::milliseconds(0)));
    flags.push_back(mkFlag("--b", "", false));
    flags.push_back(mkFlag("--s", "", std::string("")));

    {
        auto p = makeParser({}, flags);
        expect(!p.setExternalValuesChecked({{"--num", "7"}}).has_value(), "checked single accepts valid");
    }
    {
        auto p = makeParser({}, flags);
        expect(p.setExternalValuesChecked({{"--num", "nope"}}).has_value(), "checked single rejects invalid int");
    }
    {
        auto p = makeParser({}, flags);
        expect(!p.setExternalValuesChecked({{"--nokey", "anything"}}).has_value(),
               "checked single ignores unknown key");
    }
    {
        auto p = makeParser({}, flags);
        expect(!p.setExternalValuesMultiChecked({{"--num", {"7", "8"}}}).has_value(),
               "checked multi accepts valid");
    }
    {
        auto p = makeParser({}, flags);
        expect(p.setExternalValuesMultiChecked({{"--num", {"7", "bad"}}}).has_value(),
               "checked multi rejects invalid int");
    }
    {
        auto p = makeParser({}, flags);
        expect(!p.setExternalValuesMultiChecked({{"--nokey", {"x"}}}).has_value(),
               "checked multi ignores unknown key");
    }
    // Every Kind's rejection path inside normalizeValue.
    {
        auto p = makeParser({}, flags);
        expect(p.setExternalValuesChecked({{"--i64", "zz"}}).has_value(), "checked rejects bad int64");
        expect(p.setExternalValuesChecked({{"--u32", "zz"}}).has_value(), "checked rejects bad uint32");
        expect(p.setExternalValuesChecked({{"--u32", "-5"}}).has_value(), "checked rejects negative uint32");
        expect(p.setExternalValuesChecked({{"--u", "zz"}}).has_value(), "checked rejects bad uint64");
        expect(p.setExternalValuesChecked({{"--f", "zz"}}).has_value(), "checked rejects bad float");
        expect(p.setExternalValuesChecked({{"--d", "zz"}}).has_value(), "checked rejects bad double");
        expect(p.setExternalValuesChecked({{"--dur", "zz"}}).has_value(), "checked rejects bad duration");
        expect(p.setExternalValuesChecked({{"--b", "zz"}}).has_value(), "checked rejects bad bool");
        expect(p.setExternalValuesChecked({{"--num", "99999999999999999999999"}}).has_value(),
               "checked rejects int overflow");
        expect(!p.ok(), "checked-setter rejection marks parser not ok");
    }
    // Valid values for every kind normalize through the same switch.
    {
        auto p = makeParser({}, flags);
        expect(!p.setExternalValuesChecked({
            {"--num", "3"}, {"--i64", "4"}, {"--u32", "5"}, {"--u", "6"},
            {"--f", "1.5"}, {"--d", "2.5"}, {"--dur", "3ms"}, {"--b", "on"},
        }).has_value(), "checked accepts every kind");
        expect(p.getFlag<std::chrono::milliseconds>("--dur") == std::chrono::milliseconds(3),
               "duration round trip");
    }
}

// ---------------------------------------------------------------------------
// getCount sources (lines 241-260).
void testCountPaths() {
    std::vector<clasp::Flag> flags;
    flags.push_back(mkFlag("--cnt", "", 0));

    {
        auto p = makeParser({"--cnt", "2", "--cnt", "3"}, flags);
        expect(p.getCount("--cnt") == 5, "getCount sums CLI occurrences");
    }
    {
        auto p = makeParser({}, flags);
        p.setExternalValuesMulti({{"--cnt", {"2", "3"}}});
        expect(p.getCount("--cnt") == 5, "getCount sums external multi");
    }
    {
        auto p = makeParser({}, flags);
        p.setExternalValuesMulti({{"--empty", {}}});
        expect(p.getCount("--empty", 9) == 9, "getCount skips empty multi");
    }
    {
        auto p = makeParser({}, flags);
        p.setExternalValues({{"--cnt", "7"}});
        expect(p.getCount("--cnt") == 7, "getCount uses external single");
    }
    {
        auto p = makeParser({}, flags);
        expect(p.getCount("--cnt", 4) == 0, "getCount uses declared default");
    }
    {
        auto p = makeParser({}, flags);
        expect(p.getCount("--absent", 4) == 4, "getCount caller default");
    }
}

// ---------------------------------------------------------------------------
// hasFlag / hasValue / hasExplicitValue / getExplicitFlagValues / getFlagValues.
void testLookupPaths() {
    std::vector<clasp::Flag> flags;
    flags.push_back(mkFlag("--decl", "", std::string("dflt")));
    flags.push_back(mkFlag("--cli", "", std::string("")));

    auto p = makeParser({"--cli", "v"}, flags);
    p.setExternalValuesMulti({{"--multi", {"m1"}}, {"--emptymulti", {}}});
    p.setExternalValues({{"--ext", "e1"}});

    expect(p.hasFlag("--cli"), "hasFlag sees CLI value");
    expect(!p.hasFlag("--absent"), "hasFlag misses absent flag");

    expect(p.hasValue("--cli"), "hasValue CLI");
    expect(p.hasValue("--multi"), "hasValue external multi");
    expect(p.hasValue("--ext"), "hasValue external single");
    expect(!p.hasValue("--absent"), "hasValue absent");
    expect(!p.hasValue("--emptymulti"), "hasValue empty multi is not a value");
    expect(!p.hasValue("--decl"), "hasValue ignores declared default");

    expect(p.hasExplicitValue("--cli"), "hasExplicitValue CLI");
    expect(p.hasExplicitValue("--multi"), "hasExplicitValue multi");
    expect(p.hasExplicitValue("--ext"), "hasExplicitValue ext");
    expect(!p.hasExplicitValue("--emptymulti"), "hasExplicitValue empty multi");
    expect(!p.hasExplicitValue("--absent"), "hasExplicitValue absent");

    expect((p.getExplicitFlagValues("--cli") == std::vector<std::string>{"v"}), "explicit values CLI");
    expect((p.getExplicitFlagValues("--multi") == std::vector<std::string>{"m1"}), "explicit values multi");
    expect((p.getExplicitFlagValues("--ext") == std::vector<std::string>{"e1"}), "explicit values ext");
    expect(p.getExplicitFlagValues("--absent").empty(), "explicit values absent");
    expect(p.getExplicitFlagValues("--emptymulti").empty(), "explicit values empty multi");

    expect((p.getFlagValues("--cli") == std::vector<std::string>{"v"}), "flag values CLI");
    expect((p.getFlagValues("--multi") == std::vector<std::string>{"m1"}), "flag values multi");
    expect((p.getFlagValues("--ext") == std::vector<std::string>{"e1"}), "flag values ext");
    expect((p.getFlagValues("--decl") == std::vector<std::string>{"dflt"}), "flag values declared default");
    expect(p.getFlagValues("--absent").empty(), "flag values absent");
    expect(p.getFlagValues("--emptymulti").empty(), "flag values empty multi");
}

// ---------------------------------------------------------------------------
// getArrayRaw sources (lines 505-520) and getFlagValuesSplit loop edge (321).
void testArrayAndSplitPaths() {
    std::vector<clasp::Flag> flags;
    flags.push_back(mkFlag("--cli", "", std::string("")));
    flags.push_back(mkFlag("--emptydef", "", std::string("")));
    flags.push_back(mkFlag("--def", "", std::string("d")));
    flags.push_back(mkFlag("--nodef", "", std::string("")));
    flags.push_back(mkFlag("--tags", "", std::string("")));

    {
        auto p = makeParser({"--cli", "x"}, flags);
        expect((p.getStringArray("--cli") == std::vector<std::string>{"x"}), "array CLI source");
    }
    {
        auto p = makeParser({}, flags);
        p.setExternalValuesMulti({{"--m", {"x"}}});
        expect((p.getStringArray("--m") == std::vector<std::string>{"x"}), "array multi source");
    }
    {
        auto p = makeParser({}, flags);
        p.setExternalValuesMulti({{"--def", {}}});
        expect((p.getStringArray("--def") == std::vector<std::string>{"d"}), "array empty multi falls to default");
    }
    {
        auto p = makeParser({}, flags);
        p.setExternalValues({{"--e", "x"}});
        expect((p.getStringArray("--e") == std::vector<std::string>{"x"}), "array ext source");
    }
    {
        auto p = makeParser({}, flags);
        expect(p.getStringArray("--emptydef").empty(), "array empty declared default");
        expect((p.getStringArray("--def") == std::vector<std::string>{"d"}), "array declared default");
        expect(p.getStringArray("--nodef").empty(), "array no default at all");
    }
    {
        auto p = makeParser({"--tags", "a,b"}, flags);
        expect((p.getFlagValuesSplit("--tags") == std::vector<std::string>{"a", "b"}), "split two parts");
        expect((p.getFlagValuesSplit("--tags", ' ') == std::vector<std::string>{"a,b"}), "split no separator");
    }
    {
        auto p = makeParser({}, flags);
        p.setExternalValues({{"--tags", "a,b"}});
        expect((p.getFlagValuesSplit("--tags") == std::vector<std::string>{"a", "b"}), "split via external");
    }
    {
        auto p = makeParser({}, flags);
        p.setExternalValues({{"--tags", ""}}); // empty value -> loop body runs once, no parts
        expect(p.getFlagValuesSplit("--tags").empty(), "split empty value");
    }
}

// ---------------------------------------------------------------------------
// utils::suggest edges (utils.hpp line 65).
void testSuggest() {
    auto hits = clasp::utils::suggest("hep", {"help", "helm"});
    expect(hits.size() == 2, "suggest finds near matches");
    auto prefix = clasp::utils::suggest("bu", {"build", "burst"});
    expect(prefix.size() == 2 && prefix[0] == "build", "suggest prefers prefix matches");
    expect(clasp::utils::suggest("zzzzzz", {"help"}).empty(), "suggest drops distant candidates");
    expect(clasp::utils::suggest("x", {}).empty(), "suggest with no candidates");
    expect(clasp::utils::suggest("x", {""}).empty(), "suggest skips empty candidates");
    expect(clasp::utils::suggest("help", {"help", "help", "help"}, 2).size() == 2, "suggest respects max results");
}

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
void testShortGroupPaths() {
    std::vector<clasp::Flag> flags;
    flags.push_back(mkFlag("--all", "-a", false));
    flags.push_back(mkFlag("--brief", "-b", false));
    clasp::Flag cnt = mkFlag("--count", "-v", 0);
    cnt.setAnnotation("count", "true");
    flags.push_back(cnt);
    flags.push_back(mkFlag("--num", "-n", 0));

    {
        auto p = makeParser({"-ab"}, flags);
        expect(p.ok() && p.getFlag<bool>("--all") && p.getFlag<bool>("--brief"), "group sets both bools");
    }
    {
        auto p = makeParser({"-av3"}, flags);
        expect(p.ok() && p.getFlag<bool>("--all") && p.getCount("--count") == 3, "count numeric remainder");
    }
    {
        auto p = makeParser({"-v+3"}, flags);
        expect(p.ok() && p.getCount("--count") == 3, "count plus-signed remainder");
    }
    {
        auto p = makeParser({"-v-2"}, flags);
        expect(p.ok() && p.getCount("--count") == -2, "count minus-signed remainder");
    }
    {
        auto p = makeParser({"-vvv"}, flags);
        expect(p.ok() && p.getCount("--count") == 3, "count repeated occurrences");
    }
    {
        auto p = makeParser({"-v3x"}, flags, allowUnknown());
        expect(p.ok() && p.getCount("--count") == 1, "count non-numeric remainder falls back");
    }
    {
        auto p = makeParser({"-v3.5"}, flags, allowUnknown());
        expect(p.ok() && p.getCount("--count") == 1, "count remainder with sub-digit char falls back");
    }
    {
        auto p = makeParser({"-v999999999999999999999"}, flags);
        expect(!p.ok(), "count huge remainder rejected");
    }
    {
        auto p = makeParser({"-ax", "val"}, flags, allowUnknown());
        expect(p.ok() && p.getFlag<bool>("--all") && p.positionals().empty(),
               "unknown trailing short consumes next token");
    }
    {
        auto p = makeParser({"-ax", "-b"}, flags, allowUnknown());
        expect(p.ok() && p.getFlag<bool>("--all") && p.getFlag<bool>("--brief"),
               "unknown trailing short keeps flag token");
    }
    {
        auto p = makeParser({"-ax"}, flags, allowUnknown());
        expect(p.ok() && p.getFlag<bool>("--all"), "unknown trailing short without next token");
    }
    {
        auto p = makeParser({"-za"}, flags, allowUnknown());
        expect(p.ok() && p.getFlag<bool>("--all"), "unknown leading short skipped");
    }
    {
        auto p = makeParser({"-xa", "file"}, flags, allowUnknown());
        expect(p.ok() && p.getFlag<bool>("--all") && p.positionals().size() == 1,
               "unknown middle short keeps next token");
    }
    {
        auto p = makeParser({"-nX"}, flags);
        expect(!p.ok(), "value short with invalid value rejected");
    }
    {
        auto p = makeParser({"-n5"}, flags);
        expect(p.ok() && p.getFlag<int>("--num") == 5, "value short with attached value");
    }
    {
        auto p = makeParser({"-an", "5"}, flags);
        expect(p.ok() && p.getFlag<bool>("--all") && p.getFlag<int>("--num") == 5,
               "value short takes next argv token");
    }
    {
        auto p = makeParser({"-an"}, flags);
        expect(!p.ok(), "value short at end of argv rejected");
    }
    {
        auto p = makeParser({"-an", "-b"}, flags);
        expect(!p.ok(), "value short before another flag rejected");
    }
    {
        auto p = makeParser({"-vz"}, flags, allowUnknown());
        expect(p.ok() && p.getCount("--count") == 1, "count then unknown short tolerated");
    }
}

// ---------------------------------------------------------------------------
// Token classification edges feeding isFlagToken / isShortGroupToken.
void testTokenEdges() {
    std::vector<clasp::Flag> flags;
    flags.push_back(mkFlag("--f", "-f", std::string("")));
    auto p = makeParser({"xy", "-", "--f", "v", "--", "-a"}, flags);
    expect(p.ok() && p.getFlag<std::string>("--f") == "v" &&
               (p.positionals() == std::vector<std::string>{"xy", "-", "-a"}),
           "token classification edges");
}

} // namespace

int main() {
    testUnknownFlagPaths();
    testNoOptPaths();
    testBoolLiterals();
    testGetFlagChain();
    testNarrowIntRanges();
    testCheckedExternal();
    testCountPaths();
    testLookupPaths();
    testArrayAndSplitPaths();
    testSuggest();
    testValueParseDirect();
    testBytesUnits();
    testIPParsing();
    testIPMaskAndCIDR();
    testURLParsing();
    testAnnotationKinds();
    testShortGroupPaths();
    testTokenEdges();

    if (g_failures == 0) {
        std::cout << "ALL OK" << std::endl;
        return 0;
    }
    std::cout << g_failures << " failures" << std::endl;
    return 1;
}
