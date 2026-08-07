#include "zip_reader.h"
#include <fstream>
#include <cstring>
#include <zlib.h>

namespace {

#pragma pack(push, 1)
struct EndOfCentralDirectory {
    uint32_t signature;      // 0x06054b50
    uint16_t diskNumber;
    uint16_t diskWithCD;
    uint16_t entriesOnDisk;
    uint16_t totalEntries;
    uint32_t cdSize;
    uint32_t cdOffset;
    uint16_t commentLength;
};

struct CentralDirectoryHeader {
    uint32_t signature;      // 0x02014b50
    uint16_t versionMadeBy;
    uint16_t versionNeeded;
    uint16_t flags;
    uint16_t compressionMethod;
    uint16_t modTime;
    uint16_t modDate;
    uint32_t crc32;
    uint32_t compressedSize;
    uint32_t uncompressedSize;
    uint16_t nameLength;
    uint16_t extraLength;
    uint16_t commentLength;
    uint16_t diskNumberStart;
    uint16_t internalAttrs;
    uint32_t externalAttrs;
    uint32_t localHeaderOffset;
};

struct LocalFileHeader {
    uint32_t signature;      // 0x04034b50
    uint16_t versionNeeded;
    uint16_t flags;
    uint16_t compressionMethod;
    uint16_t modTime;
    uint16_t modDate;
    uint32_t crc32;
    uint32_t compressedSize;
    uint32_t uncompressedSize;
    uint16_t nameLength;
    uint16_t extraLength;
};
#pragma pack(pop)

constexpr uint32_t EOCD_SIG = 0x06054b50;
constexpr uint32_t CD_SIG   = 0x02014b50;
constexpr uint32_t LFH_SIG  = 0x04034b50;

bool inflateRaw(const uint8_t* src, size_t srcLen, size_t dstLen, std::vector<uint8_t>& out) {
    out.resize(dstLen);
    if (dstLen == 0) return true;

    z_stream strm{};
    if (inflateInit2(&strm, -MAX_WBITS) != Z_OK) return false;

    strm.next_in = const_cast<uint8_t*>(src);
    strm.avail_in = static_cast<uInt>(srcLen);
    strm.next_out = out.data();
    strm.avail_out = static_cast<uInt>(dstLen);

    int ret = inflate(&strm, Z_FINISH);
    inflateEnd(&strm);
    return ret == Z_STREAM_END;
}

} // namespace

bool ZipReader::open(const std::string& path) {
    path_ = path;
    entries_.clear();
    index_.clear();

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return false;

    const std::streamsize fileSize = file.tellg();
    if (fileSize < static_cast<std::streamsize>(sizeof(EndOfCentralDirectory))) return false;

    // Search backwards for the End Of Central Directory signature.
    // A zip comment can follow it, so we scan the last 64KB + header size.
    const size_t searchWindow = std::min<size_t>(fileSize, 65536 + sizeof(EndOfCentralDirectory));
    std::vector<uint8_t> tail(searchWindow);
    file.seekg(fileSize - static_cast<std::streamsize>(searchWindow));
    file.read(reinterpret_cast<char*>(tail.data()), searchWindow);

    long eocdPos = -1;
    for (long i = static_cast<long>(searchWindow) - 4; i >= 0; --i) {
        uint32_t sig;
        std::memcpy(&sig, &tail[i], 4);
        if (sig == EOCD_SIG) { eocdPos = i; break; }
    }
    if (eocdPos < 0) return false; // not a zip / corrupted

    EndOfCentralDirectory eocd{};
    std::memcpy(&eocd, &tail[eocdPos], sizeof(EndOfCentralDirectory));

    file.seekg(eocd.cdOffset);
    for (uint16_t i = 0; i < eocd.totalEntries; ++i) {
        CentralDirectoryHeader cdh{};
        file.read(reinterpret_cast<char*>(&cdh), sizeof(cdh));
        if (cdh.signature != CD_SIG) break;

        std::string name(cdh.nameLength, '\0');
        file.read(&name[0], cdh.nameLength);
        file.seekg(cdh.extraLength + cdh.commentLength, std::ios::cur);

        Entry e;
        e.name = name;
        e.compressedSize = cdh.compressedSize;
        e.uncompressedSize = cdh.uncompressedSize;
        e.compressionMethod = cdh.compressionMethod;
        e.localHeaderOffset = cdh.localHeaderOffset;

        index_[name] = entries_.size();
        entries_.push_back(std::move(e));
    }
    return true;
}

bool ZipReader::contains(const std::string& name) const {
    return index_.find(name) != index_.end();
}

bool ZipReader::extract(const std::string& name, std::vector<uint8_t>& out) const {
    auto it = index_.find(name);
    if (it == index_.end()) return false;
    const Entry& e = entries_[it->second];

    std::ifstream file(path_, std::ios::binary);
    if (!file) return false;

    file.seekg(e.localHeaderOffset);
    LocalFileHeader lfh{};
    file.read(reinterpret_cast<char*>(&lfh), sizeof(lfh));
    if (lfh.signature != LFH_SIG) return false;

    file.seekg(lfh.nameLength + lfh.extraLength, std::ios::cur);

    std::vector<uint8_t> compressed(e.compressedSize);
    if (e.compressedSize > 0) {
        file.read(reinterpret_cast<char*>(compressed.data()), e.compressedSize);
    }

    if (e.compressionMethod == 0) { // stored
        out = std::move(compressed);
        return true;
    } else if (e.compressionMethod == 8) { // deflate
        return inflateRaw(compressed.data(), compressed.size(), e.uncompressedSize, out);
    }
    return false; // unsupported compression method
}
