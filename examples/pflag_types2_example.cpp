#include <algorithm>
#include <cstdint>
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

template <typename T>
static std::string joinMapSorted(const std::unordered_map<std::string, T>& m) {
    std::vector<std::pair<std::string, T>> items(m.begin(), m.end());
    std::sort(items.begin(), items.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    std::vector<std::string> flat;
    flat.reserve(items.size());
    for (const auto& [k, v] : items) flat.push_back(k + "=" + std::to_string(v));
    return join(flat);
}

static std::string joinBytes(const std::vector<unsigned char>& v) {
    std::string out;
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (i) out += "|";
        out += std::to_string(static_cast<int>(v[i]));
    }
    return out;
}

int main(int argc, char** argv) {
    clasp::Command root("app", "extended pflag type helpers example");
    root.withPersistentUint8Flag("--plevel", "", "plevel", "Persistent uint8 flag (inherited by subcommands)", 3);

    clasp::Command show("show", "Print parsed values");
    show.withInt8Flag("--small", "-s", "small", "Int8-like flag", 0)
        .withInt16Flag("--medium", "", "medium", "Int16-like flag", 0)
        .withInt32Flag("--wide", "", "wide", "Int32-like flag", 0)
        .withUint8Flag("--port", "-p", "port", "Uint8-like flag", 0)
        .withUint16Flag("--vlan", "", "vlan", "Uint16-like flag", 0)
        .withUintFlag("--offset", "", "offset", "Uint (platform word) flag", 0)
        .withIPSliceFlag("--ips", "", "ips", "IPSlice-like (canonical IPs, comma-split, repeatable)", std::string(""))
        .withBytesBase64Flag("--blob", "", "blob", "BytesBase64-like (canonical base64 in, raw bytes out)", std::string(""))
        .withFlag("--ratios", "", "ratios", "StringToFloat-like (a=1.5,b=2.5)", std::string(""))
        .withFlag("--ints8", "", "ints8", "Int8Slice-like values (comma-split, repeatable)", std::string(""))
        .withFlag("--uints16", "", "uints16", "Uint16Slice-like values (comma-split, repeatable)", std::string(""));

    show.action([](clasp::Command&, const clasp::Parser& p, const std::vector<std::string>&) {
        std::cout << "small=" << static_cast<int>(p.getInt8("--small")) << "\n";
        std::cout << "medium=" << p.getInt16("--medium") << "\n";
        std::cout << "wide=" << p.getInt32("--wide") << "\n";
        std::cout << "port=" << static_cast<int>(p.getUint8("--port")) << "\n";
        std::cout << "vlan=" << p.getUint16("--vlan") << "\n";
        std::cout << "offset=" << p.getUint("--offset") << "\n";
        std::cout << "plevel=" << static_cast<int>(p.getUint8("--plevel")) << "\n";

        std::cout << "small_slice=" << joinNumeric(p.getInt8Slice("--small")) << "\n";
        std::cout << "port_slice=" << joinNumeric(p.getUint8Slice("--port")) << "\n";
        std::cout << "ints8_slice=" << joinNumeric(p.getInt8Slice("--ints8")) << "\n";
        std::cout << "uints16_slice=" << joinNumeric(p.getUint16Slice("--uints16")) << "\n";
        std::cout << "small_array=" << joinNumeric(p.getInt8Array("--small")) << "\n";
        std::cout << "port_array=" << joinNumeric(p.getUint8Array("--port")) << "\n";
        std::cout << "medium_array=" << joinNumeric(p.getInt16Array("--medium")) << "\n";
        std::cout << "vlan_array=" << joinNumeric(p.getUint16Array("--vlan")) << "\n";

        std::cout << "ips=" << join(p.getIPSlice("--ips")) << "\n";
        std::cout << "blob=" << joinBytes(p.getBytesBase64("--blob")) << "\n";
        std::cout << "ratios=" << joinMapSorted(p.getStringToFloat("--ratios")) << "\n";
        return 0;
    });

    root.addCommand(std::move(show));
    return root.run(argc, argv);
}
