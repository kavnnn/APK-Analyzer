#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <map>

// Minimal ZIP (APK) reader. Supports STORED and DEFLATE entries.
// Enough to list files and extract specific entries like
// AndroidManifest.xml and classes.dex without any external
// dependency other than zlib.
class ZipReader {
public:
    struct Entry {
        std::string name;
        uint32_t compressedSize = 0;
        uint32_t uncompressedSize = 0;
        uint16_t compressionMethod = 0; // 0 = stored, 8 = deflate
        uint32_t localHeaderOffset = 0;
    };

    // Opens the archive and parses its central directory.
    // Returns false if the file could not be opened or is not a valid zip.
    bool open(const std::string& path);

    // All entries found in the central directory, in archive order.
    const std::vector<Entry>& entries() const { return entries_; }

    // Extracts a single entry's uncompressed bytes by exact name.
    // Returns true on success.
    bool extract(const std::string& name, std::vector<uint8_t>& out) const;

    bool contains(const std::string& name) const;

private:
    std::string path_;
    std::vector<Entry> entries_;
    std::map<std::string, size_t> index_;
};
