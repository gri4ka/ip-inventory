#pragma once
#include <string>

// Validates and normalize an IP address.
// For IPv4: standard dotted notation (e.g. "95.44.73.19")
// For IPv6: fully expanded 8-group zero-padded form (e.g. "2a01:05a9:01a4:095c:0000:0000:0000:0001")
bool normalize_ip(const std::string& ipType, const std::string& input, std::string& outCanonical);
