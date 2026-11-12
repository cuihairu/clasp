#include "branch_support.hpp"

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
        expect(p.ok() && p.getFlag<std::string>("--mode") == "auto", "noOpt used for real value, token not consumed");
        expect(p.positionals().size() == 1 && p.positionals()[0] == "manual", "real value stays a positional");
    }
    {
        auto p = makeParser({"-om", "--mode"}, flags, allowUnknown());
        expect(p.ok() && p.getFlag<std::string>("--mode") == "auto" && p.getFlag<bool>("--other"),
               "short group noOpt when next token is a flag");
    }
    {
        auto p = makeParser({"-om", "5"}, flags);
        expect(p.ok() && p.getFlag<std::string>("--mode") == "auto", "short group noOpt used, token not consumed");
        expect(p.positionals().size() == 1 && p.positionals()[0] == "5", "short group real value stays positional");
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
        expect(!p.ok(), "count negative remainder rejected");
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
