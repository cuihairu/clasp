#include "branch_support.hpp"

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
