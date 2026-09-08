#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "clasp/clasp.hpp"

namespace {

int g_failures = 0;
int g_destructions = 0;

void expect(bool cond, const char* label) {
    std::cout << label << ": " << (cond ? "pass" : "fail") << std::endl;
    if (!cond) ++g_failures;
}

struct CountingValue final : clasp::Value {
    int sets = 0;
    std::string value{"initial"};

    ~CountingValue() override { ++g_destructions; }

    std::string type() const override { return "counting"; }

    std::string string() const override { return value; }

    std::optional<std::string> set(std::string_view v) override {
        ++sets;
        if (v.empty()) return std::string("empty value");
        value = std::string(v);
        return std::nullopt;
    }
};

void testPolymorphicUseViaBasePointer() {
    // Use the value exclusively through the clasp::Value interface.
    const int destroyed_before = g_destructions;
    clasp::Value* v = new CountingValue();
    expect(v->type() == "counting", "Value::type via base pointer");
    expect(v->string() == "initial", "Value::string via base pointer");

    expect(!v->set("updated").has_value(), "Value::set success via base pointer");
    expect(v->string() == "updated", "Value::string after set");

    const std::optional<std::string> err = v->set("");
    expect(err.has_value() && *err == "empty value", "Value::set error via base pointer");
    expect(static_cast<CountingValue*>(v)->sets == 2, "Value::set call count via base pointer");

    // Virtual destructor must run through the base pointer.
    delete v;
    expect(g_destructions == destroyed_before + 1, "virtual destructor runs via base pointer");
}

void testPolymorphicUseViaUniquePtr() {
    const int destroyed_before = g_destructions;
    {
        auto v = std::unique_ptr<clasp::Value>(std::make_unique<CountingValue>());
        expect(v->type() == "counting", "Value::type via unique_ptr<Value>");
        expect(!v->set("x").has_value(), "Value::set via unique_ptr<Value>");
        expect(v->string() == "x", "Value::string via unique_ptr<Value>");
    } // Destruction goes through ~Value().
    expect(g_destructions == destroyed_before + 1, "virtual destructor runs via unique_ptr<Value>");
}

void testValueReferenceInCommand() {
    clasp::Command root("app", "Value interface coverage");

    CountingValue level;
    root.withValueFlag("--level", "-l", "Level", level);
    root.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
        return 0;
    });

    const char* argv[] = {"app", "--level", "debug"};
    const int sets_before = level.sets;
    const int rc = root.run(3, const_cast<char**>(argv));
    expect(rc == 0, "run with Value flag succeeds");
    expect(level.sets == sets_before + 1, "Value::set invoked by parser");
    expect(level.string() == "debug", "Value reflects parsed state");
}

} // namespace

int main() {
    std::cout << "=== Testing Value interface polymorphic use ===" << std::endl;
    testPolymorphicUseViaBasePointer();

    std::cout << "\n=== Testing Value via unique_ptr ===" << std::endl;
    testPolymorphicUseViaUniquePtr();

    std::cout << "\n=== Testing Value bound to Command ===" << std::endl;
    testValueReferenceInCommand();

    if (g_failures == 0) {
        std::cout << "\nok\n";
    }
    return g_failures == 0 ? 0 : 1;
}
