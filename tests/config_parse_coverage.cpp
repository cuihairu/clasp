// Coverage tests for config-file parsing internals (JSON/TOML/INI/YAML/env).
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "clasp/command.hpp"

namespace {

int g_failures = 0;
std::string g_capture;

// MSVC has no setenv/unsetenv (POSIX); _putenv_s with an empty value deletes.
void setEnvVar(const char* name, const char* value) {
#ifdef _WIN32
    _putenv_s(name, value);
#else
    ::setenv(name, value, 1);
#endif
}

void unsetEnvVar(const char* name) {
#ifdef _WIN32
    _putenv_s(name, "");
#else
    ::unsetenv(name);
#endif
}

void expect(bool cond, const char* label) {
    std::cout << label << ": " << (cond ? "ok" : "fail") << std::endl;
    if (!cond) ++g_failures;
}

void writeTextFile(const std::string& path, const std::string& content) {
    std::ofstream f(path, std::ios::binary);
    f << content;
}

int runArgs(clasp::Command& cmd, const std::vector<std::string>& args) {
    std::vector<char*> ptrs;
    ptrs.push_back(const_cast<char*>(cmd.name().c_str()));
    for (const auto& a : args) ptrs.push_back(const_cast<char*>(a.c_str()));
    return cmd.run(static_cast<int>(ptrs.size()), ptrs.data());
}

// Root command wired for config-file driven tests. When withKeyFlags is set,
// additional flags are registered so parsed keys (including section/dotted
// paths and oddly-named flags) have mapping targets inside keyToLong.
clasp::Command makeCfgRoot(std::ostringstream& out, std::ostringstream& err, bool withKeyFlags) {
    clasp::Command root("app", "cfg parse coverage");
    root.setOut(out);
    root.setErr(err);
    root.withFlag("--config", "", "cfg", "Config file", std::string(""));
    root.withFlag("--sv", "", "sv", "String value", std::string(""));
    root.withFlag("--items", "", "items", "Items", std::string(""));
    root.withFlag("--num", "", "num", "Number", 0);
    if (withKeyFlags) {
        root.withFlag("", "", "emptylong", "Flag with empty long name", std::string(""));
        root.withFlag("-x", "", "xdash", "Flag whose long name is single-dash", std::string(""));
        root.withFlag("--top", "", "top", "YAML top-level list", std::string(""));
        root.withFlag("--nested-inner", "", "nestedinner", "YAML nested list", std::string(""));
        root.withFlag("--sec-sv", "", "secsv", "Section-prefixed key", std::string(""));
        root.withFlag("--outer-sv", "", "outersv", "JSON nested key", std::string(""));
    }
    root.configFileFlag("--config");
    root.action([](clasp::Command&, const clasp::Parser& p, const std::vector<std::string>&) {
        g_capture = "sv=[" + p.getFlag<std::string>("--sv", "") + "]";
        g_capture += " num=" + std::to_string(p.getFlag<int>("--num", 0));
        g_capture += " items=";
        for (const auto& i : p.getStringArray("--items")) g_capture += "|" + i;
        g_capture += " top=";
        for (const auto& i : p.getStringArray("--top")) g_capture += "|" + i;
        g_capture += " ni=";
        for (const auto& i : p.getStringArray("--nested-inner")) g_capture += "|" + i;
        g_capture += " secsv=[" + p.getFlag<std::string>("--sec-sv", "") + "]";
        g_capture += " outer=[" + p.getFlag<std::string>("--outer-sv", "") + "]";
        return 0;
    });
    return root;
}

// One config load against a throwaway root; returns exit code.
int loadConfigContent(const std::string& file, const std::string& content, std::string* errText = nullptr) {
    std::ostringstream out, err;
    auto root = makeCfgRoot(out, err, true);
    writeTextFile(file, content);
    g_capture.clear();
    const int rc = runArgs(root, {"--config", file});
    std::remove(file.c_str());
    if (errText != nullptr) *errText = err.str();
    return rc;
}

// --- .env-like files ---------------------------------------------------------

void testEnvLikeParsing() {
    std::string errText;
    // Comments, key-less lines, quoted/mismatched/short values.
    const int rc = loadConfigContent(
        "/tmp/cpc_env.env",
        "# full line comment\n"
        "justtext\n"
        "s=x\n"
        "sv=\"double quoted\"\n"
        "sq='single'\n"
        "mx=\"mismatch'\n"
        "mx2='mismatch\"\n",
        &errText);
    expect(rc == 0, "env: comment/nokey/quoted/mismatched values accepted");
    expect(g_capture.find("sv=[double quoted]") != std::string::npos, "env: double-quoted value unquoted");

    // Unquoted value arrives trimmed.
    const int rc2 = loadConfigContent("/tmp/cpc_env2.env", "sv= spaced out \n");
    expect(rc2 == 0, "env: unquoted value accepted");
    expect(g_capture.find("sv=[spaced out]") != std::string::npos, "env: unquoted value trimmed");

    // Extension-less path (no '.' anywhere): dispatches via the find('.')==npos
    // side; the path is also shorter than every suffix so endsWith() takes its
    // early-out on the ".json" probe.
    writeTextFile("e", "sv=nodot\n");
    std::ostringstream out, err;
    auto root = makeCfgRoot(out, err, true);
    g_capture.clear();
    const int rc3 = runArgs(root, {"--config", "e"});
    std::remove("e");
    expect(rc3 == 0, "env: extension-less path dispatched to env parser");
    expect(g_capture.find("sv=[nodot]") != std::string::npos, "env: extension-less value applied");
}

// --- JSON parser -------------------------------------------------------------

void testJsonBranchMatrix() {
    struct Case {
        const char* content;
        bool ok;
        const char* label;
    };
    const std::vector<Case> cases = {
        {"{\r\n\t \"sv\" : \"ok\" \r\n}\r\n", true, "json: space/tab/nl/cr whitespace skipped"},
        {"{\"sv\": +5}", true, "json: leading plus sign accepted"},
        {"{\"items\": [1.5, 2e+3]}", true, "json: fraction and exponent multi values"},
        {"{\"alongtoplevelkeyname123456\": 1}", true, "json: long top-level key"},
        {"{\"averylongprefixname\": {\"alongkeyname123\": 1}}", true, "json: long nested key"},
        {"{\"outer\": {\"sv\": \"nested\"}}", true, "json: nested object flattens"},
        {"{\"items\": []}", true, "json: empty array yields no multi values"},
        {"{\"items\": [[], [1, 2]]}", true, "json: nested arrays skipped cleanly"},
        {"{\"items\": [{}, {\"a\": {}}, 1]}", true, "json: objects inside arrays skipped"},
        {"{\"a\":", false, "json: eof after colon rejected"},
        {"{\"a\": 1,", false, "json: eof after comma rejected"},
        {"{5: 1}", false, "json: non-string key rejected"},
        {"{\"items\": [1.", false, "json: eof after fraction dot rejected"},
        {"{\"items\": [1.x]}", false, "json: non-digit after fraction dot rejected"},
        {"{\"sv\": 1e", false, "json: eof after exponent marker rejected"},
        {"{\"sv\": 1E}", false, "json: exponent without digits rejected"},
        {"{\"items\": [{5: 1}]}", false, "json: bad key inside skipped object rejected"},
        {"{\"items\": [{\"a\" 1}]}", false, "json: missing colon inside skipped object rejected"},
        {"{\"items\": [{\"a\": }]}", false, "json: missing value inside skipped object rejected"},
        {"{\"items\": [{\"a\": 1 \"b\": 2}]}", false, "json: missing comma inside skipped object rejected"},
        {"{\"a\": {\"b\": }}", false, "json: nested object with missing value rejected"},
        {"{\"a\": 1 \"b\": 2}", false, "json: missing separator between pairs rejected"},
    };
    for (const auto& c : cases) {
        std::string errText;
        const int rc = loadConfigContent("/tmp/cpc_json.json", c.content, &errText);
        expect((rc == 0) == c.ok, c.label);
        if (!c.ok) {
            expect(errText.find("failed to parse json config file") != std::string::npos,
                   (std::string(c.label) + " [error text]").c_str());
        }
    }
    // Verify flattened nested key reached the flag via the dotted mapping.
    {
        std::ostringstream out, err;
        auto root = makeCfgRoot(out, err, true);
        writeTextFile("/tmp/cpc_json2.json", "{\"outer\": {\"sv\": \"nested\"}}");
        g_capture.clear();
        const int rc = runArgs(root, {"--config", "/tmp/cpc_json2.json"});
        std::remove("/tmp/cpc_json2.json");
        expect(rc == 0, "json: nested key load ok");
        expect(g_capture.find("outer=[nested]") != std::string::npos, "json: nested scalar value applied");
    }
}

// --- TOML parser -------------------------------------------------------------

void testTomlBranchMatrix() {
    std::string errText;
    const int rc = loadConfigContent(
        "/tmp/cpc_toml.toml",
        "# whole line comment\n"
        "justtext\n"
        "= 5\n"
        "sv =\n"
        "sv = 5 # trailing comment\n"
        "sq = 'x' # c\n"
        "empty = \"\"\n"
        "esc = \"a\\tb\"\n"
        "mismatch = \"abc'\n"
        "items = [\"a\" , \"b\"]\n"
        "items2 = [1,,2]\n"
        "n1 = [\n"
        "n2 = [1\n"
        "items3 = ['a\"b', \"c'd\", \"e\\\"f\"]\n"
        "averylongtoplevelkey12345 = 2\n"
        "[]\n"
        "cleared = 1\n"
        "[averylongtablenamexyz]\n"
        "averylongkeyname123 = 1\n"
        "[sec]\n"
        "sv = \"insec\"\n"
        "inline = {a = 1}\n",
        &errText);
    expect(rc == 0, "toml: combined branch matrix accepted");
    expect(g_capture.find("sv=[insec]") != std::string::npos, "toml: section-prefixed key applied");
    expect(g_capture.find("items=|a|b") != std::string::npos, "toml: array with padded commas parsed");
    expect(errText.empty(), "toml: no error text");

    // Unclosed table header fails the whole file.
    const int rc2 = loadConfigContent("/tmp/cpc_toml_bad.toml", "[no-close\nsv = 1\n");
    expect(rc2 != 0, "toml: unclosed table header rejected");

    std::ostringstream out, err;
    auto root = makeCfgRoot(out, err, true);
    writeTextFile("/tmp/cpc_toml_arr.toml", "items = [\"x\", 'y']\n");
    g_capture.clear();
    const int rc3 = runArgs(root, {"--config", "/tmp/cpc_toml_arr.toml"});
    std::remove("/tmp/cpc_toml_arr.toml");
    expect(rc3 == 0, "toml: quoted array load ok");
    expect(g_capture.find("items=|x|y") != std::string::npos, "toml: array values bound as multi");
}

// --- INI parser --------------------------------------------------------------

void testIniBranchMatrix() {
    std::string errText;
    const int rc = loadConfigContent(
        "/tmp/cpc_a.ini",
        "justtext\n"
        "= nokey\n"
        "; semi comment\n"
        "# hash comment\n"
        "[unclosed\n"
        "s=x\n"
        "k1 = v1\n"
        "k2 : v2\n"
        "k3=\"dq\"\n"
        "k4='sq'\n"
        "k5=\"mix'\n"
        "k6='mix\"\n"
        "[averylongsectionname]\n"
        "averylongkeyname123 = v\n"
        "[sec]\n"
        "sv = insec\n",
        &errText);
    expect(rc == 0, "ini: comment/section/quote matrix accepted");
    expect(g_capture.find("sv=[insec]") != std::string::npos, "ini: section-prefixed key applied");
    expect(g_capture.find("sv=[dq]") == std::string::npos, "ini: unrelated keys do not touch --sv");
}

// --- YAML parser -------------------------------------------------------------

void testYamlBranchMatrix() {
    std::string errText;
    const int rc = loadConfigContent(
        "/tmp/cpc_a.yml",
        "sv: plain\n"
        "sq: 'single'\n"
        "dq: \"double\"\n"
        "mx1: \"mismatch'\n"
        "mx2: 'mismatch\"\n"
        "esc1: \"a\\\"b\" # escaped quote keeps hash\n"
        "esc2: 'a # b'\n"
        "esc3: \"a'b\"\n"
        "esc4: 'a\"b'\n"
        "\"qk\": 1 # quoted key comment\n"
        "justtext\n"
        ": nokey\n"
        "\n"
        "   \n"
        "   # indented comment\n"
        "- orphan\n"
        "\ttv: tabbed\n"
        "top:\n"
        "  - one\n"
        "  - \"two\"\n"
        "nested:\n"
        "  inner:\n"
        "    - deep\n"
        "top2:\n"
        "  sub: x\n"
        "after:\n"
        "sv: popped\n",
        &errText);
    expect(rc == 0, "yaml: comment/quote/indent/list matrix accepted");
    expect(g_capture.find("sv=[popped]") != std::string::npos, "yaml: scalar after stack pop applied");
    expect(g_capture.find("top=|one|two") != std::string::npos, "yaml: list items bound as multi");
    expect(g_capture.find("ni=|deep") != std::string::npos, "yaml: nested list path bound");
    expect(errText.empty(), "yaml: no error text");

    // .yaml extension dispatches too.
    const int rc2 = loadConfigContent("/tmp/cpc_b.yaml", "sv: yamlfile\n");
    expect(rc2 == 0, "yaml: .yaml extension dispatches");
    expect(g_capture.find("sv=[yamlfile]") != std::string::npos, "yaml: .yaml value applied");
}

// --- Extension dispatch and path resolution ----------------------------------

void testExtensionDispatchAndPaths() {
    // .cfg and .env extensions route to their parsers.
    const int rcCfg = loadConfigContent("/tmp/cpc_b.cfg", "sv=cfgval\n");
    expect(rcCfg == 0, "dispatch: .cfg extension routes to ini parser");
    expect(g_capture.find("sv=[cfgval]") != std::string::npos, "dispatch: .cfg value applied");

    const int rcEnv = loadConfigContent("/tmp/cpc_b.env", "sv=envval\n");
    expect(rcEnv == 0, "dispatch: .env extension routes to env parser");
    expect(g_capture.find("sv=[envval]") != std::string::npos, "dispatch: .env value applied");

    // Unsupported extension and unreadable file produce errors.
    std::string errText;
    const int rcTxt = loadConfigContent("/tmp/cpc_b.txt", "sv=x\n", &errText);
    expect(rcTxt != 0, "dispatch: unsupported extension rejected");
    expect(errText.find("unsupported config file format") != std::string::npos,
           "dispatch: unsupported extension error text");

    std::ostringstream out, err;
    auto root = makeCfgRoot(out, err, true);
    const int rcMissing = runArgs(root, {"--config", "/tmp/cpc_definitely_missing.json"});
    expect(rcMissing != 0, "dispatch: missing config file rejected");

    // Fixed path (no flag): configPathDefault branch.
    writeTextFile("/tmp/cpc_fixed.json", "{\"sv\": \"fixed\"}");
    clasp::Command root2("app", "fixed path");
    root2.setOut(out);
    root2.setErr(err);
    root2.withFlag("--sv", "", "sv", "S", std::string(""));
    root2.configFile("/tmp/cpc_fixed.json");
    root2.action([](clasp::Command&, const clasp::Parser& p, const std::vector<std::string>&) {
        g_capture = "sv=[" + p.getFlag<std::string>("--sv", "") + "]";
        return 0;
    });
    g_capture.clear();
    const int rcFixed = runArgs(root2, {});
    expect(rcFixed == 0, "dispatch: fixed config path loads without flag");
    expect(g_capture.find("sv=[fixed]") != std::string::npos, "dispatch: fixed path value applied");
    std::remove("/tmp/cpc_fixed.json");

    // Child overrides parent default path; both loop-guard sides exercised.
    writeTextFile("/tmp/cpc_parent.json", "{\"sv\": \"parent\"}");
    writeTextFile("/tmp/cpc_child.json", "{\"sv\": \"child\"}");
    clasp::Command root3("app", "parent");
    root3.setOut(out);
    root3.setErr(err);
    root3.configFile("/tmp/cpc_parent.json");
    clasp::Command sub("sub", "sub");
    sub.setOut(out);
    sub.setErr(err);
    sub.withFlag("--config", "", "cfg", "Config file", std::string(""));
    sub.withFlag("--sv", "", "sv", "S", std::string(""));
    sub.configFile("/tmp/cpc_child.json");
    sub.configFileFlag("--config");
    sub.action([](clasp::Command&, const clasp::Parser& p, const std::vector<std::string>&) {
        g_capture = "sv=[" + p.getFlag<std::string>("--sv", "") + "]";
        return 0;
    });
    root3.addCommand(std::move(sub));
    g_capture.clear();
    const int rcSub = runArgs(root3, {"sub", "--config", "/tmp/cpc_child.json"});
    expect(rcSub == 0, "dispatch: subcommand config load ok");
    expect(g_capture.find("sv=[child]") != std::string::npos, "dispatch: config flag beats fixed path");
}

// --- externalMulti mapping and env overrides ---------------------------------

void testMultiMappingAndEnvOverrides() {
    // Unknown multi key is dropped; known multi key with invalid scalar fails.
    const int rcUnknown = loadConfigContent("/tmp/cpc_multi1.json", "{\"items\": [1, 2], \"unknownkey\": [1]}");
    expect(rcUnknown == 0, "multi: unknown key dropped without error");

    std::string errText;
    const int rcBad = loadConfigContent("/tmp/cpc_multi2.json", "{\"num\": [1, \"abc\"]}", &errText);
    expect(rcBad != 0, "multi: invalid scalar value rejected");
    expect(!errText.empty(), "multi: rejection carries error text");

    const int rcInts = loadConfigContent("/tmp/cpc_multi3.json", "{\"num\": [1, 2], \"items\": [\"a\"]}");
    expect(rcInts == 0, "multi: valid integer array accepted");

    // Env bindings override config scalars and erase config multi values.
    setEnvVar("CPC_SV", "fromenv");
    setEnvVar("CPC_EMPTY", "");
    setEnvVar("CPC_ITEMS", "x");
    std::ostringstream out, err;
    auto root = makeCfgRoot(out, err, true);
    root.bindEnv("--sv", "CPC_SV");
    root.bindEnv("--num", "CPC_EMPTY");
    root.bindEnv("--noenvname", "");
    root.bindEnv("--items", "CPC_ITEMS");
    writeTextFile("/tmp/cpc_envov.json", "{\"items\": [\"a\", \"b\"], \"sv\": \"cfg\", \"num\": 7}");
    g_capture.clear();
    const int rc = runArgs(root, {"--config", "/tmp/cpc_envov.json"});
    std::remove("/tmp/cpc_envov.json");
    expect(rc == 0, "env: override matrix load ok");
    expect(g_capture.find("sv=[fromenv]") != std::string::npos, "env: env value beats config value");
    expect(g_capture.find("items=|x") != std::string::npos && g_capture.find("|a") == std::string::npos,
           "env: env binding erases config multi");
    unsetEnvVar("CPC_SV");
    unsetEnvVar("CPC_EMPTY");
    unsetEnvVar("CPC_ITEMS");
}

// --- Remaining TOML value-shape branches -------------------------------------
// Every quoting/escaping/array shape the inline unquote and parseTomlArray
// helpers can meet, including the trailing-backslash escape and the
// unknown-escape fallback, plus values nested under a [table].

void testTomlValueShapes() {
    std::string errText;
    // double-quoted escapes incl. one past-the-end backslash and unknown \g;
    // single-quoted raw; bare word; plain double-quoted (no escapes).
    const int rc = loadConfigContent(
        "/tmp/cpc_shapes.toml",
        "esc = \"a\\nb\\tc\\rd\\\\e\\\"f\\g\"\n"
        "escend = \"tail\\\"\n"
        "plain = \"simple\"\n"
        "estr = \"\"\n"
        "raw = 'literal'\n"
        "eraw = ''\n"
        "bare = xyz\n"
        "num = 5\n",
        &errText);
    expect(rc == 0, "toml shapes: quoting matrix accepted");
    expect(errText.empty(), "toml shapes: no error text");

    // Arrays: padded spaces, trailing comma, empty array, single element,
    // quoted mix, and the same shapes nested under a table header.
    const int rc2 = loadConfigContent(
        "/tmp/cpc_shapes2.toml",
        "items = [ 1 , 2 ]\n"
        "items = [4, ]\n"
        "items = []\n"
        "items = [7]\n"
        "[sec]\n"
        "secsv = [\"a\", 'b']\n"
        "secsv = [ 3 ,  ]\n"
        "secsv = []\n"
        "secsv = tail\n",
        &errText);
    expect(rc2 == 0, "toml shapes: array matrix accepted");
    expect(errText.empty(), "toml shapes: array matrix no error");
}

// --- Top-level (no section) INI and YAML keys --------------------------------

void testIniYamlTopLevelKeys() {
    std::string errText;
    // INI keys before any [section] header: sectionPrefix stays empty.
    const int rc = loadConfigContent("/tmp/cpc_top.ini", "sv = topINI\n", &errText);
    expect(rc == 0, "ini top-level: load ok");
    expect(g_capture.find("sv=[topINI]") != std::string::npos, "ini top-level: key applied");

    // YAML nested mapping: fullKey accumulates the stack frames (dotted).
    const int rc2 = loadConfigContent(
        "/tmp/cpc_nested.yml",
        "sec:\n"
        "  sv: fromsec\n"
        "sv: plain\n",
        &errText);
    expect(rc2 == 0, "yaml nested: load ok");
    expect(g_capture.find("secsv=[fromsec]") != std::string::npos, "yaml nested: section key applied");
    expect(g_capture.find("sv=[plain]") != std::string::npos, "yaml nested: top-level key applied");
}

} // namespace

int main() {
    // Keep the short relative fixture used by the extension tests inside /tmp.
    std::filesystem::current_path("/tmp");

    testEnvLikeParsing();
    testJsonBranchMatrix();
    testTomlBranchMatrix();
    testIniBranchMatrix();
    testYamlBranchMatrix();
    testExtensionDispatchAndPaths();
    testMultiMappingAndEnvOverrides();
    testTomlValueShapes();
    testIniYamlTopLevelKeys();

    if (g_failures == 0) {
        std::cout << "ALL OK" << std::endl;
        return 0;
    }
    std::cout << g_failures << " failures" << std::endl;
    return 1;
}
