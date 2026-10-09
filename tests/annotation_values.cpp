// Coverage for annotation-value truthiness: help type names (flagTypeName)
// and parser registration (registerFlag) must accept "1"/"yes" like the
// helper-set "true", and must ignore non-truthy values such as "no".
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "clasp/command.hpp"

namespace {

int g_failures = 0;

void expect(bool cond, const char* label) {
    std::cout << label << ": " << (cond ? "ok" : "fail") << std::endl;
    if (!cond) ++g_failures;
}

// Registers the eight annotation-backed helper flags and forces every type
// annotation to `value` (the helpers set "true"; mark*Annotation overwrites).
void configure(clasp::Command& root, const std::string& value) {
    root.withInt8Flag("--small", "", "small", "Int8 flag", 0)
        .withInt16Flag("--medium", "", "medium", "Int16 flag", 0)
        .withInt32Flag("--wide", "", "wide", "Int32 flag", 0)
        .withUint8Flag("--port", "", "port", "Uint8 flag", 0)
        .withUint16Flag("--vlan", "", "vlan", "Uint16 flag", 0)
        .withUintFlag("--offset", "", "offset", "Uint flag", 0)
        .withIPSliceFlag("--ips", "", "ips", "IPSlice flag", std::string(""))
        .withBytesBase64Flag("--blob", "", "blob", "BytesBase64 flag", std::string(""));
    root.markFlagAnnotation("--small", "int8", value);
    root.markFlagAnnotation("--medium", "int16", value);
    root.markFlagAnnotation("--wide", "int32", value);
    root.markFlagAnnotation("--port", "uint8", value);
    root.markFlagAnnotation("--vlan", "uint16", value);
    root.markFlagAnnotation("--offset", "uint", value);
    root.markFlagAnnotation("--ips", "ipslice", value);
    root.markFlagAnnotation("--blob", "bytesbase64", value);
}

int runArgs(clasp::Command& cmd, const std::vector<std::string>& args) {
    std::vector<char*> ptrs;
    ptrs.push_back(const_cast<char*>(cmd.name().c_str()));
    for (const auto& a : args) ptrs.push_back(const_cast<char*>(a.c_str()));
    return cmd.run(static_cast<int>(ptrs.size()), ptrs.data());
}

std::string renderHelp(clasp::Command& cmd) {
    std::ostringstream os;
    cmd.setOut(os);
    cmd.setErr(os);
    cmd.printHelp();
    return os.str();
}

// Truthy alternates ("1", "yes") must render the annotated type names, and
// the variant fallbacks must not appear.
void testTruthyValueShowsAnnotatedTypes() {
    for (const std::string_view v : {"1", "yes"}) {
        clasp::Command root("app", "annotation values");
        configure(root, std::string(v));
        root.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
        const std::string help = renderHelp(root);
        const bool ok = help.find("--small int8") != std::string::npos &&
                        help.find("--medium int16") != std::string::npos &&
                        help.find("--wide int32") != std::string::npos &&
                        help.find("--port uint8") != std::string::npos &&
                        help.find("--vlan uint16") != std::string::npos &&
                        help.find("--offset uint64") == std::string::npos &&
                        help.find("--ips ipSlice") != std::string::npos &&
                        help.find("--blob bytesBase64") != std::string::npos;
        expect(ok, ("annotation value \"" + std::string(v) + "\" shows annotated types").c_str());
    }
}

// A non-truthy value must disable the annotation: flagTypeName falls back to
// the variant type name for every helper flag.
void testNonTruthyValueFallsBackToVariantTypes() {
    clasp::Command root("app", "annotation values");
    configure(root, "no");
    root.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
    const std::string help = renderHelp(root);
    const bool ok = help.find("--small int8") == std::string::npos &&
                    help.find("--port uint8") == std::string::npos &&
                    help.find("--ips ipSlice") == std::string::npos &&
                    help.find("--blob bytesBase64") == std::string::npos &&
                    help.find("--blob string") != std::string::npos &&
                    help.find("--ips string") != std::string::npos &&
                    help.find("--offset uint64") != std::string::npos;
    expect(ok, "non-truthy annotation falls back to variant type names");
}

// Registration follows the same truthiness: "1"/"yes" keep narrow-width
// range validation active; "no" registers nothing, so 999 parses as a plain int.
void testRegisterFlagTruthiness() {
    for (const std::string_view v : {"1", "yes"}) {
        clasp::Command root("app", "annotation values");
        configure(root, std::string(v));
        root.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
        std::ostringstream os;
        root.setOut(os);
        root.setErr(os);
        const int rc = runArgs(root, {"--small", "999"});
        expect(rc != 0, ("annotation value \"" + std::string(v) + "\" keeps int8 validation").c_str());
    }
    {
        clasp::Command root("app", "annotation values");
        configure(root, "no");
        root.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) { return 0; });
        std::ostringstream os;
        root.setOut(os);
        root.setErr(os);
        const int rc = runArgs(root, {"--small", "999"});
        expect(rc == 0, "non-truthy annotation skips int8 validation");
    }
}

} // namespace

int main() {
    testTruthyValueShowsAnnotatedTypes();
    testNonTruthyValueFallsBackToVariantTypes();
    testRegisterFlagTruthiness();
    std::cout << (g_failures == 0 ? "ok" : "fail") << std::endl;
    return g_failures == 0 ? 0 : 1;
}
