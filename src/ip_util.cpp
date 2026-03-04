#include "../headers/ip_util.hpp"
#include <cstring>
#include <cstdio>
#include <winsock2.h>
#include <ws2tcpip.h>


// Expand a binary in6_addr to full 8-group zero-padded hex string.
// Example output: "2a01:05a9:01a4:095c:0000:0000:0000:0001"
static std::string expand_ipv6(const struct in6_addr& addr) {
    // in6_addr contains 16 bytes. Read them pairwise as 8 groups.
    unsigned short groups[8]{};
    for (int i = 0; i < 8; ++i) {
        groups[i] = (unsigned short)((addr.u.Byte[i * 2] << 8) | addr.u.Byte[i * 2 + 1]);
    }

    char buf[40]{}; // "xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx" = 39 + null
    std::snprintf(buf, sizeof(buf),
        "%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x",
        groups[0], groups[1], groups[2], groups[3],
        groups[4], groups[5], groups[6], groups[7]);

    return std::string(buf);
}

bool normalize_ip(const std::string& ipType, const std::string& input, std::string& outNormalized) {
    if (ipType == "IPv4") {
        struct in_addr a4{};
        if (inet_pton(AF_INET, input.c_str(), &a4) != 1) return false;

        char buf[INET_ADDRSTRLEN]{};
        if (!inet_ntop(AF_INET, &a4, buf, sizeof(buf))) return false;

        outNormalized = buf;
        return true;
    }

    if (ipType == "IPv6") {
        struct in6_addr a6{};
        if (inet_pton(AF_INET6, input.c_str(), &a6) != 1) return false;
        // Use binary representation directly to produce fully expanded form
        outNormalized = expand_ipv6(a6);
        return true;
    }

    return false;
}
