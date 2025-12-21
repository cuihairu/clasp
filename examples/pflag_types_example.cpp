#include <algorithm>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "clasp/clasp.hpp"

static std::string join(const std::vector<std::string>& v) {
    std::string out;
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (i) out += "|";
        out += v[i];
    }
    return out;
}

template <typename T>
static std::string joinNumeric(const std::vector<T>& v) {
    std::string out;
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (i) out += "|";
        out += std::to_string(v[i]);
    }
    return out;
}

static std::string joinBool(const std::vector<bool>& v) {
    std::string out;
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (i) out += "|";
        out += (v[i] ? "true" : "false");
    }
    return out;
}

template <typename T>
static std::string joinMapSorted(const std::unordered_map<std::string, T>& m) {
    std::vector<std::pair<std::string, T>> items(m.begin(), m.end());
    std::sort(items.begin(), items.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    std::vector<std::string> flat;
    flat.reserve(items.size());
    for (const auto& [k, v] : items) flat.push_back(k + "=" + std::to_string(v));
    return join(flat);
}

static std::string joinMapSortedBool(const std::unordered_map<std::string, bool>& m) {
    std::vector<std::pair<std::string, bool>> items(m.begin(), m.end());
    std::sort(items.begin(), items.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    std::vector<std::string> flat;
    flat.reserve(items.size());
    for (const auto& [k, v] : items) flat.push_back(k + "=" + (v ? "true" : "false"));
    return join(flat);
}

int main(int argc, char** argv) {
    clasp::Command root("app", "pflag type helpers example");

    clasp::Command show("show", "Print parsed values");
    show.withFlag("--names", "", "names", "StringSlice-like (comma-split, repeatable)", std::string(""));
    show.withFlag("--tags", "", "tags", "StringArray-like (repeatable, no comma split)", std::string(""));
    show.withFlag("--labels", "", "labels", "StringToString-like (a=1,b=2)", std::string(""));
    show.withFlag("--gates", "", "gates", "BoolSlice-like (comma-split, repeatable)", std::string(""));
    show.withFlag("--ports", "", "ports", "IntSlice-like (comma-split, repeatable)", std::string(""));
    show.withFlag("--nums", "", "nums", "IntArray-like (repeatable, no comma split)", std::string(""));
    show.withFlag("--scores", "", "scores", "StringToInt-like (a=1,b=2)", std::string(""));
    show.withFlag("--big", "", "big", "StringToInt64-like (a=1,b=2)", std::string(""));
    show.withFlag("--toggles", "", "toggles", "StringToBool-like (a=true,b=false)", std::string(""));

    show.action([](clasp::Command&, const clasp::Parser& p, const std::vector<std::string>&) {
        const auto names = p.getStringSlice("--names");
        const auto tags = p.getStringArray("--tags");

        const auto m = p.getStringToString("--labels");
        std::vector<std::pair<std::string, std::string>> labels(m.begin(), m.end());
        std::sort(labels.begin(), labels.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
        std::vector<std::string> labelsFlat;
        labelsFlat.reserve(labels.size());
        for (const auto& [k, v] : labels) labelsFlat.push_back(k + "=" + v);

        const auto gates = p.getBoolSlice("--gates");
        const auto ports = p.getIntSlice("--ports");
        const auto nums = p.getIntArray("--nums");
        const auto scores = p.getStringToInt("--scores");
        const auto big = p.getStringToInt64("--big");
        const auto toggles = p.getStringToBool("--toggles");

        std::cout << "names=" << join(names) << "\n";
        std::cout << "tags=" << join(tags) << "\n";
        std::cout << "labels=" << join(labelsFlat) << "\n";
        std::cout << "gates=" << joinBool(gates) << "\n";
        std::cout << "ports=" << joinNumeric(ports) << "\n";
        std::cout << "nums=" << joinNumeric(nums) << "\n";
        std::cout << "scores=" << joinMapSorted(scores) << "\n";
        std::cout << "big=" << joinMapSorted(big) << "\n";
        std::cout << "toggles=" << joinMapSortedBool(toggles) << "\n";
        return 0;
    });

    root.addCommand(std::move(show));
    return root.run(argc, argv);
}
