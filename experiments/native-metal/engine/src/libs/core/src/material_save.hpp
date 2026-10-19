#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <zlib.h>

// SAVE transport only: live attributes and CHM geometry retain their original
// bytes. Each envelope is bounded by the material writer's 32768-byte chunks.
namespace storm::material_save
{
inline constexpr char Prefix[] = "!CMZ2:";
inline constexpr char HexPrefix[] = "!CMZ1:";
inline constexpr size_t MaxValue = 1024 * 1024;
inline constexpr uint32_t MaxStateSize = 0x8000000;
inline bool validStateSize(size_t size) { return size && size <= MaxStateSize; }

inline std::string base64(const unsigned char *bytes, size_t size)
{
    constexpr char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result; result.reserve((size + 2) / 3 * 4);
    for (size_t n = 0; n < size; n += 3) {
        const uint32_t v = uint32_t(bytes[n]) << 16 | (n + 1 < size ? uint32_t(bytes[n + 1]) << 8 : 0) | (n + 2 < size ? bytes[n + 2] : 0);
        result += alphabet[v >> 18]; result += alphabet[(v >> 12) & 63];
        result += n + 1 < size ? alphabet[(v >> 6) & 63] : '=';
        result += n + 2 < size ? alphabet[v & 63] : '=';
    }
    return result;
}

inline bool unbase64(const char *text, size_t size, std::vector<unsigned char> &bytes)
{
    if (!size || size % 4) return false;
    const auto digit = [](char c) { return c >= 'A' && c <= 'Z' ? c - 'A' : c >= 'a' && c <= 'z' ? c - 'a' + 26 : c >= '0' && c <= '9' ? c - '0' + 52 : c == '+' ? 62 : c == '/' ? 63 : -1; };
    bytes.clear(); bytes.reserve(size / 4 * 3);
    for (size_t n = 0; n < size; n += 4) {
        const int a = digit(text[n]), b = digit(text[n + 1]);
        const bool pad2 = text[n + 2] == '=', pad3 = text[n + 3] == '=';
        const int c = pad2 ? 0 : digit(text[n + 2]), d = pad3 ? 0 : digit(text[n + 3]);
        if (a < 0 || b < 0 || c < 0 || d < 0 || (pad2 && !pad3) || ((pad2 || pad3) && n + 4 != size) || (pad2 && (b & 15)) || (!pad2 && pad3 && (c & 3))) return false;
        const uint32_t v = uint32_t(a) << 18 | uint32_t(b) << 12 | uint32_t(c) << 6 | uint32_t(d);
        bytes.push_back(uint8_t(v >> 16)); if (!pad2) bytes.push_back(uint8_t(v >> 8)); if (!pad3) bytes.push_back(uint8_t(v));
    }
    return true;
}

inline bool isPacked(const char *value)
{
    return value && (!std::strncmp(value, Prefix, sizeof(Prefix) - 1) || !std::strncmp(value, HexPrefix, sizeof(HexPrefix) - 1));
}

inline std::string pack(const char *value)
{
    if (!value) return {};
    const size_t size = std::strlen(value);
    if (size < 1024 || size > MaxValue) return value;
    uLongf length = compressBound(size);
    std::vector<unsigned char> compressed(length);
    if (compress2(compressed.data(), &length, reinterpret_cast<const Bytef *>(value), size, Z_BEST_SPEED) != Z_OK)
        return value;
    if (15 + (length + 2) / 3 * 4 >= size) return value;
    constexpr char digits[] = "0123456789abcdef";
    std::string result = Prefix;
    for (int shift = 28; shift >= 0; shift -= 4) result += digits[(size >> shift) & 15];
    result += ':';
    result += base64(compressed.data(), length);
    return result;
}

inline bool unpack(const char *value, std::string &result)
{
    if (!isPacked(value)) { result = value ? value : ""; return true; }
    const size_t size = std::strlen(value);
    if (size < 17 || value[14] != ':') return false;
    const auto digit = [](char c) { return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1; };
    size_t rawSize = 0;
    for (size_t n = 6; n < 14; ++n) { const int d = digit(value[n]); if (d < 0) return false; rawSize = (rawSize << 4) | unsigned(d); }
    if (!rawSize || rawSize > MaxValue) return false;
    std::vector<unsigned char> compressed;
    if (!std::strncmp(value, HexPrefix, sizeof(HexPrefix) - 1)) {
        if ((size - 15) % 2) return false;
        compressed.resize((size - 15) / 2);
        for (size_t n = 0; n < compressed.size(); ++n) { const int a = digit(value[15 + n * 2]), b = digit(value[16 + n * 2]); if (a < 0 || b < 0) return false; compressed[n] = uint8_t(a * 16 + b); }
    } else if (!unbase64(value + 15, size - 15, compressed)) return false;
    result.resize(rawSize);
    uLongf length = rawSize;
    uLong consumed = compressed.size();
    return uncompress2(reinterpret_cast<Bytef *>(result.data()), &length, compressed.data(), &consumed) == Z_OK &&
        length == rawSize && consumed == compressed.size() && result.find('\0') == std::string::npos;
}

inline bool equalName(const char *name, const char *expected)
{
    if (!name) return false;
    for (; *name && *expected; ++name, ++expected) {
        const auto lower = [](char c) { return c >= 'A' && c <= 'Z' ? char(c + 'a' - 'A') : c; };
        if (lower(*name) != lower(*expected)) return false;
    }
    return !*name && !*expected;
}

inline bool rootName(const char *name) { return equalName(name, "hullmaterial") || equalName(name, "phonkworldmaterial"); }

template<class Attr> uint64_t revision(Attr *node)
{
    const char *value = node ? node->GetAttribute("revision") : nullptr;
    if (!value || !*value) return 0;
    uint64_t result = 0;
    for (; *value; ++value) { if (*value < '0' || *value > '9' || result > (UINT64_MAX - unsigned(*value - '0')) / 10) return 0; result = result * 10 + unsigned(*value - '0'); }
    return result;
}

template<class Attr> Attr *selected(Attr *parent, const char *a, const char *b, uint64_t committed)
{
    Attr *best = nullptr;
    uint64_t latest = 0;
    for (const auto *name : {a, b}) if (auto *slot = parent->GetAttributeClass(name)) {
        const auto r = revision(slot); if (r && r <= committed && r > latest) { best = slot; latest = r; }
    }
    return best;
}

// In memory two slots make publication atomic. On disk only the committed slot
// is reachable; retaining its predecessor doubles every geometry checkpoint.
template<class Attr> std::vector<Attr *> children(Attr *parent)
{
    std::vector<Attr *> result;
    auto *state = rootName(parent->GetThisName()) ? parent : parent->GetParent();
    const auto committed = state && rootName(state->GetThisName()) && state->GetAttributeAsDword("version") == 1 && state->GetAttribute("model") ? revision(state) : 0;
    Attr *slot = nullptr;
    const char *blob = nullptr;
    if (committed && parent != state) {
        const char *name = parent->GetThisName();
        bool numbered = name && name[0] == 'n' && name[1];
        if (numbered) for (const char *p = name + 1; *p; ++p) numbered &= *p >= '0' && *p <= '9';
        if (numbered) slot = selected(parent, "a", "b", committed);
        if (slot) blob = slot->GetAttribute("blob");
        // Do not prune a malformed checkpoint; leave validation to its reader.
        if (blob) {
            auto *data = parent->GetAttributeClass(blob);
            const char *payload = slot->GetAttribute("payload"), *id = data ? data->GetAttribute("id") : nullptr;
            if (!data || (!equalName(blob, "dataA") && !equalName(blob, "dataB")) || !payload || !id || std::strcmp(payload, id)) slot = nullptr;
        }
    }
    auto *meta = committed && parent == state ? selected(parent, "metaA", "metaB", committed) : nullptr;
    for (uint32_t n = 0; n < parent->GetAttributesNum(); ++n) {
        auto *child = parent->GetAttributeClass(n);
        const char *name = child->GetThisName();
        if (meta && (equalName(name, "metaA") || equalName(name, "metaB")) && child != meta) continue;
        if (slot && (equalName(name, "a") || equalName(name, "b") || equalName(name, "dataA") || equalName(name, "dataB")) && child != slot && (!blob || !equalName(name, blob))) continue;
        result.push_back(child);
    }
    return result;
}
}
