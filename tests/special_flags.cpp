#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "clasp/clasp.hpp"

namespace {

void testBytesFlag() {
    clasp::Command root("app", "Test bytes flag");
    root.withBytesFlag("--size", "-s", "size", "Size", std::uint64_t(0));
    root.withPersistentBytesFlag("--limit", "-L", "limit", "Limit", std::uint64_t(0));

    bool actionCalled = false;
    root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        auto size = parser.getFlag<std::uint64_t>("--size", 0);
        auto limit = parser.getFlag<std::uint64_t>("--limit", 0);
        (void)size; (void)limit;
        actionCalled = true;
        return 0;
    });

    const char* argv[] = {"app", "--size", "1KB", "--limit", "5MB"};
    root.run(5, const_cast<char**>(argv));
}

void testIPFlag() {
    // Test IPv4
    {
        clasp::Command root("app", "Test IP flag");
        root.withIPFlag("--host", "-H", "host", "Host", std::string(""));

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto host = parser.getFlag<std::string>("--host", "");
            (void)host;
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app", "--host", "192.168.1.1"};
        root.run(3, const_cast<char**>(argv));
    }

    // Test IPv6
    {
        clasp::Command root("app2", "Test IP flag");
        root.withIPFlag("--host", "-H", "host", "Host", std::string(""));

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto host = parser.getFlag<std::string>("--host", "");
            (void)host;
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app2", "--host", "::1"};
        root.run(3, const_cast<char**>(argv));
    }

    // Test IPv6 full
    {
        clasp::Command root("app3", "Test IP flag");
        root.withIPFlag("--host", "-H", "host", "Host", std::string(""));

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto host = parser.getFlag<std::string>("--host", "");
            (void)host;
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app3", "--host", "2001:db8::1"};
        root.run(3, const_cast<char**>(argv));
    }
}

void testIPMaskFlag() {
    // Test valid netmask
    {
        clasp::Command root("app", "Test IPMask flag");
        root.withIPMaskFlag("--mask", "-M", "mask", "Mask", std::string(""));

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto mask = parser.getFlag<std::string>("--mask", "");
            (void)mask;
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app", "--mask", "255.255.255.0"};
        root.run(3, const_cast<char**>(argv));
    }

    // Test another netmask
    {
        clasp::Command root("app2", "Test IPMask flag");
        root.withIPMaskFlag("--mask", "-M", "mask", "Mask", std::string(""));

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto mask = parser.getFlag<std::string>("--mask", "");
            (void)mask;
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app2", "--mask", "255.255.0.0"};
        root.run(3, const_cast<char**>(argv));
    }

    // Test empty (valid for IPMask)
    {
        clasp::Command root("app3", "Test IPMask flag");
        root.withPersistentIPMaskFlag("--netmask", "", "netmask", "Netmask", std::string(""));

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto netmask = parser.getFlag<std::string>("--netmask", "");
            (void)netmask;
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app3", "--netmask", ""};
        root.run(3, const_cast<char**>(argv));
    }
}

void testCIDRFlag() {
    // Test IPv4 CIDR
    {
        clasp::Command root("app", "Test CIDR flag");
        root.withCIDRFlag("--network", "-N", "network", "Network", std::string(""));

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto network = parser.getFlag<std::string>("--network", "");
            (void)network;
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app", "--network", "192.168.1.0/24"};
        root.run(3, const_cast<char**>(argv));
    }

    // Test IPv6 CIDR
    {
        clasp::Command root("app2", "Test CIDR flag");
        root.withPersistentCIDRFlag("--cidr", "", "cidr", "CIDR", std::string(""));

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto cidr = parser.getFlag<std::string>("--cidr", "");
            (void)cidr;
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app2", "--cidr", "2001:db8::/32"};
        root.run(3, const_cast<char**>(argv));
    }
}

void testIPNetFlag() {
    clasp::Command root("app", "Test IPNet flag");
    root.withIPNetFlag("--ipnet", "-I", "ipnet", "IPNet", std::string(""));
    root.withPersistentIPNetFlag("--network", "", "network", "Network", std::string(""));

    bool actionCalled = false;
    root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        auto ipnet = parser.getFlag<std::string>("--ipnet", "");
        (void)ipnet;
        actionCalled = true;
        return 0;
    });

    // Test IPv4 CIDR (IPNet is same as CIDR)
    const char* argv[] = {"app", "--ipnet", "10.0.0.0/8"};
    root.run(3, const_cast<char**>(argv));
}

void testURLFlag() {
    // Test valid URL
    {
        clasp::Command root("app", "Test URL flag");
        root.withURLFlag("--url", "-U", "url", "URL", std::string(""));

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto url = parser.getFlag<std::string>("--url", "");
            (void)url;
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app", "--url", "http://example.com/path"};
        root.run(3, const_cast<char**>(argv));
    }

    // Test complex URL
    {
        clasp::Command root("app2", "Test URL flag");
        root.withPersistentURLFlag("--endpoint", "", "endpoint", "Endpoint", std::string(""));

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto endpoint = parser.getFlag<std::string>("--endpoint", "");
            (void)endpoint;
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app2", "--endpoint", "https://user:pass@host:8080/path?query=value#fragment"};
        root.run(3, const_cast<char**>(argv));
    }

    // Test empty (valid for URL)
    {
        clasp::Command root("app3", "Test URL flag");
        root.withURLFlag("--url", "-U", "url", "URL", std::string(""));

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto url = parser.getFlag<std::string>("--url", "");
            (void)url;
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app3", "--url", ""};
        root.run(3, const_cast<char**>(argv));
    }

    // Test relative URL (valid)
    {
        clasp::Command root("app4", "Test URL flag");
        root.withURLFlag("--url", "-U", "url", "URL", std::string(""));

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto url = parser.getFlag<std::string>("--url", "");
            (void)url;
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app4", "--url", "relative/path"};
        root.run(3, const_cast<char**>(argv));
    }
}

void testBytesFlagUnits() {
    clasp::Command root("app", "Test bytes flag units");
    root.withBytesFlag("--size", "-s", "size", "Size", std::uint64_t(0));

    bool actionCalled = false;
    root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        auto size = parser.getFlag<std::uint64_t>("--size", 0);
        (void)size;
        actionCalled = true;
        return 0;
    });

    // Test decimal units
    const char* argv[] = {"app", "--size", "1.5GB"};
    root.run(3, const_cast<char**>(argv));
}

void testCIDRFlagPrefixValidation() {
    // Test /32 prefix
    {
        clasp::Command root("app", "Test CIDR prefix validation");
        root.withCIDRFlag("--network", "-N", "network", "Network", std::string(""));

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto network = parser.getFlag<std::string>("--network", "");
            (void)network;
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app", "--network", "192.168.1.0/32"};
        root.run(3, const_cast<char**>(argv));
    }

    // Test IPv6 /128 prefix
    {
        clasp::Command root("app2", "Test CIDR prefix validation");
        root.withCIDRFlag("--network", "-N", "network", "Network", std::string(""));

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto network = parser.getFlag<std::string>("--network", "");
            (void)network;
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app2", "--network", "2001:db8::/128"};
        root.run(3, const_cast<char**>(argv));
    }

    // Test /0 prefix
    {
        clasp::Command root("app3", "Test CIDR prefix validation");
        root.withCIDRFlag("--network", "-N", "network", "Network", std::string(""));

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto network = parser.getFlag<std::string>("--network", "");
            (void)network;
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app3", "--network", "0.0.0.0/0"};
        root.run(3, const_cast<char**>(argv));
    }
}

void testURLFlagSchemes() {
    // Test ftp
    {
        clasp::Command root("app", "Test URL flag schemes");
        root.withURLFlag("--url", "-U", "url", "URL", std::string(""));

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto url = parser.getFlag<std::string>("--url", "");
            (void)url;
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app", "--url", "ftp://ftp.example.com"};
        root.run(3, const_cast<char**>(argv));
    }

    // Test http
    {
        clasp::Command root("app2", "Test URL flag schemes");
        root.withURLFlag("--url", "-U", "url", "URL", std::string(""));

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto url = parser.getFlag<std::string>("--url", "");
            (void)url;
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app2", "--url", "http://example.com"};
        root.run(3, const_cast<char**>(argv));
    }

    // Test https with port
    {
        clasp::Command root("app3", "Test URL flag schemes");
        root.withURLFlag("--url", "-U", "url", "URL", std::string(""));

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto url = parser.getFlag<std::string>("--url", "");
            (void)url;
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app3", "--url", "https://example.com:8443"};
        root.run(3, const_cast<char**>(argv));
    }
}

void testIPFlagWithIPv4MappedIPv6() {
    clasp::Command root("app", "Test IP flag with IPv4-mapped IPv6");
    root.withIPFlag("--host", "-H", "host", "Host", std::string(""));

    bool actionCalled = false;
    root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        auto host = parser.getFlag<std::string>("--host", "");
        (void)host;
        actionCalled = true;
        return 0;
    });

    // Test IPv4-mapped IPv6 address
    const char* argv[] = {"app", "--host", "::ffff:192.0.2.128"};
    root.run(3, const_cast<char**>(argv));
}

void testAllSpecialFlagsTogether() {
    clasp::Command root("app", "Test all special flags together");
    root.withBytesFlag("--quota", "-q", "quota", "Quota", std::uint64_t(0));
    root.withIPFlag("--server", "-S", "server", "Server", std::string(""));
    root.withIPMaskFlag("--mask", "-M", "mask", "Mask", std::string(""));
    root.withCIDRFlag("--subnet", "", "subnet", "Subnet", std::string(""));
    root.withIPNetFlag("--network", "", "network", "Network", std::string(""));
    root.withURLFlag("--source", "", "source", "Source", std::string(""));

    bool actionCalled = false;
    root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        auto quota = parser.getFlag<std::uint64_t>("--quota", 0);
        auto server = parser.getFlag<std::string>("--server", "");
        auto mask = parser.getFlag<std::string>("--mask", "");
        auto subnet = parser.getFlag<std::string>("--subnet", "");
        auto network = parser.getFlag<std::string>("--network", "");
        auto source = parser.getFlag<std::string>("--source", "");

        (void)quota; (void)server; (void)mask; (void)subnet; (void)network; (void)source;
        actionCalled = true;
        return 0;
    });

    const char* argv[] = {"app",
        "--quota", "100MB",
        "--server", "192.168.1.1",
        "--mask", "255.255.255.0",
        "--subnet", "10.0.0.0/24",
        "--network", "172.16.0.0/16",
        "--source", "https://example.com/api"
    };
    root.run(13, const_cast<char**>(argv));
}

void testBytesFlagLargeValues() {
    // Test terabyte
    {
        clasp::Command root("app", "Test large byte values");
        root.withBytesFlag("--big", "-b", "big", "Big", std::uint64_t(0));

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto big = parser.getFlag<std::uint64_t>("--big", 0);
            (void)big;
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app", "--big", "1TB"};
        root.run(3, const_cast<char**>(argv));
    }

    // Test petabyte
    {
        clasp::Command root("app2", "Test large byte values");
        root.withBytesFlag("--big", "-b", "big", "Big", std::uint64_t(0));

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto big = parser.getFlag<std::uint64_t>("--big", 0);
            (void)big;
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app2", "--big", "10PB"};
        root.run(3, const_cast<char**>(argv));
    }
}

void testIPv6FlagVariousFormats() {
    // Test compressed IPv6
    {
        clasp::Command root("app", "Test IPv6 various formats");
        root.withIPFlag("--addr6", "-6", "addr6", "Addr6", std::string(""));

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto addr6 = parser.getFlag<std::string>("--addr6", "");
            (void)addr6;
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app", "--addr6", "2001:db8::"};
        root.run(3, const_cast<char**>(argv));
    }

    // Test full IPv6
    {
        clasp::Command root("app2", "Test IPv6 various formats");
        root.withIPFlag("--addr6", "-6", "addr6", "Addr6", std::string(""));

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto addr6 = parser.getFlag<std::string>("--addr6", "");
            (void)addr6;
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app2", "--addr6", "2001:0db8:0000:0000:0000:0000:0000:0001"};
        root.run(3, const_cast<char**>(argv));
    }

    // Test IPv6 loopback
    {
        clasp::Command root("app3", "Test IPv6 various formats");
        root.withIPFlag("--addr6", "-6", "addr6", "Addr6", std::string(""));

        bool actionCalled = false;
        root.action([&](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
            auto addr6 = parser.getFlag<std::string>("--addr6", "");
            (void)addr6;
            actionCalled = true;
            return 0;
        });

        const char* argv[] = {"app3", "--addr6", "::1"};
        root.run(3, const_cast<char**>(argv));
    }
}

} // namespace

int main() {
    testBytesFlag();
    testIPFlag();
    testIPMaskFlag();
    testCIDRFlag();
    testIPNetFlag();
    testURLFlag();
    testBytesFlagUnits();
    testCIDRFlagPrefixValidation();
    testURLFlagSchemes();
    testIPFlagWithIPv4MappedIPv6();
    testAllSpecialFlagsTogether();
    testBytesFlagLargeValues();
    testIPv6FlagVariousFormats();

    std::cout << "ok\n";
    return 0;
}
