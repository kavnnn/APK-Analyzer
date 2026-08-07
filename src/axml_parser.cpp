#include "axml_parser.h"
#include <cstring>
#include <algorithm>

namespace {

#pragma pack(push, 1)
struct ResChunkHeader {
    uint16_t type;
    uint16_t headerSize;
    uint32_t size;
};

struct ResStringPoolHeader {
    ResChunkHeader header;
    uint32_t stringCount;
    uint32_t styleCount;
    uint32_t flags;
    uint32_t stringsStart;
    uint32_t stylesStart;
};
#pragma pack(pop)

constexpr uint16_t RES_STRING_POOL_TYPE = 0x0001;
constexpr uint32_t UTF8_FLAG = 1 << 8;

} // namespace

namespace axml {

std::vector<std::string> extractStrings(const std::vector<uint8_t>& data) {
    std::vector<std::string> result;
    if (data.size() < sizeof(ResChunkHeader)) return result;

    // The AXML file starts with an XML chunk header, immediately
    // followed by the string pool chunk. Scan for the string pool
    // chunk type rather than assuming a fixed offset, for safety.
    size_t offset = 0;
    while (offset + sizeof(ResChunkHeader) <= data.size()) {
        ResChunkHeader chunk{};
        std::memcpy(&chunk, &data[offset], sizeof(chunk));
        if (chunk.type == RES_STRING_POOL_TYPE) break;
        if (chunk.size == 0) return result;
        offset += chunk.size;
    }
    if (offset + sizeof(ResStringPoolHeader) > data.size()) return result;

    ResStringPoolHeader pool{};
    std::memcpy(&pool, &data[offset], sizeof(pool));

    const bool isUtf8 = (pool.flags & UTF8_FLAG) != 0;
    const size_t indexBase = offset + sizeof(ResStringPoolHeader);
    const size_t stringsBase = offset + pool.stringsStart;

    for (uint32_t i = 0; i < pool.stringCount; ++i) {
        const size_t idxOffset = indexBase + i * sizeof(uint32_t);
        if (idxOffset + sizeof(uint32_t) > data.size()) break;

        uint32_t relOffset;
        std::memcpy(&relOffset, &data[idxOffset], sizeof(relOffset));
        size_t strOffset = stringsBase + relOffset;
        if (strOffset >= data.size()) continue;

        std::string value;
        if (isUtf8) {
            // utf8 strings: [len_16 or skip][len_8][bytes...]
            uint8_t lenByte = data[strOffset];
            size_t pos = strOffset + 1;
            if (lenByte & 0x80) { pos += 1; } // skip extended UTF-16 length byte
            if (pos >= data.size()) continue;
            uint8_t byteLen = data[pos];
            pos += 1;
            if (byteLen & 0x80) {
                if (pos >= data.size()) continue;
                byteLen = data[pos];
                pos += 1;
            }
            if (pos + byteLen <= data.size()) {
                value.assign(reinterpret_cast<const char*>(&data[pos]), byteLen);
            }
        } else {
            // utf16 strings: [len_16 or extended][code units...]
            uint16_t lenUnits;
            std::memcpy(&lenUnits, &data[strOffset], sizeof(uint16_t));
            size_t pos = strOffset + 2;
            if (lenUnits & 0x8000) {
                pos += 2; // extended length, skip second word
                lenUnits &= 0x7fff;
            }
            for (uint32_t c = 0; c < lenUnits; ++c) {
                if (pos + 1 >= data.size()) break;
                uint16_t codeUnit;
                std::memcpy(&codeUnit, &data[pos], sizeof(uint16_t));
                pos += 2;
                // Naive UTF-16 -> UTF-8 for the common BMP case.
                if (codeUnit < 0x80) {
                    value += static_cast<char>(codeUnit);
                } else if (codeUnit < 0x800) {
                    value += static_cast<char>(0xC0 | (codeUnit >> 6));
                    value += static_cast<char>(0x80 | (codeUnit & 0x3F));
                } else {
                    value += static_cast<char>(0xE0 | (codeUnit >> 12));
                    value += static_cast<char>(0x80 | ((codeUnit >> 6) & 0x3F));
                    value += static_cast<char>(0x80 | (codeUnit & 0x3F));
                }
            }
        }
        if (!value.empty()) result.push_back(std::move(value));
    }
    return result;
}

std::vector<std::string> filterPermissions(const std::vector<std::string>& strings) {
    std::vector<std::string> perms;
    for (const auto& s : strings) {
        if (s.find(".permission.") != std::string::npos ||
            s.rfind("android.permission", 0) == 0) {
            perms.push_back(s);
        }
    }
    std::sort(perms.begin(), perms.end());
    perms.erase(std::unique(perms.begin(), perms.end()), perms.end());
    return perms;
}

} // namespace axml
