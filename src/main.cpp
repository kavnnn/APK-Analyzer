#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <algorithm>
#include "zip_reader.h"
#include "axml_parser.h"

namespace {

void printHeader(const std::string& title) {
    std::cout << "\n== " << title << " ==\n";
}

struct DexHeader {
    char magic[8];        // "dex\n035\0"
    uint32_t checksum;
    uint8_t signature[20];
    uint32_t fileSize;
    uint32_t headerSize;
    uint32_t endianTag;
    uint32_t linkSize;
    uint32_t linkOff;
    uint32_t mapOff;
    uint32_t stringIdsSize;
    uint32_t stringIdsOff;
    uint32_t typeIdsSize;
    uint32_t typeIdsOff;
    uint32_t protoIdsSize;
    uint32_t protoIdsOff;
    uint32_t fieldIdsSize;
    uint32_t fieldIdsOff;
    uint32_t methodIdsSize;
    uint32_t methodIdsOff;
    uint32_t classDefsSize;
    uint32_t classDefsOff;
    uint32_t dataSize;
    uint32_t dataOff;
};

void analyzeDex(const std::vector<uint8_t>& bytes) {
    if (bytes.size() < sizeof(DexHeader)) {
        std::cout << "  (too small to be a valid classes.dex)\n";
        return;
    }
    DexHeader hdr{};
    std::memcpy(&hdr, bytes.data(), sizeof(DexHeader));
    std::cout << "  Magic          : dex\\n" << (bytes[4] - '0') << (bytes[5] - '0') << (bytes[6] - '0') << "\n";
    std::cout << "  File size      : " << hdr.fileSize << " bytes\n";
    std::cout << "  String IDs     : " << hdr.stringIdsSize << "\n";
    std::cout << "  Type IDs       : " << hdr.typeIdsSize << "\n";
    std::cout << "  Method IDs     : " << hdr.methodIdsSize << "\n";
    std::cout << "  Class defs     : " << hdr.classDefsSize << "\n";
}

std::string humanSize(uint32_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB"};
    double size = bytes;
    int unit = 0;
    while (size >= 1024.0 && unit < 3) { size /= 1024.0; unit++; }
    std::ostringstream out;
    out << std::fixed << std::setprecision(size < 10 ? 2 : 1) << size << " " << units[unit];
    return out.str();
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: apk-analyzer <path-to-apk>\n";
        return 1;
    }

    const std::string apkPath = argv[1];

    ZipReader zip;
    if (!zip.open(apkPath)) {
        std::cerr << "Error: could not open '" << apkPath << "' as an APK/ZIP file.\n";
        return 1;
    }

    std::cout << "APK Analyzer\n";
    std::cout << "File: " << apkPath << "\n";

    // --- File listing summary -------------------------------------------
    printHeader("Archive Contents");
    uint64_t totalUncompressed = 0;
    for (const auto& e : zip.entries()) totalUncompressed += e.uncompressedSize;
    std::cout << "  Entries        : " << zip.entries().size() << "\n";
    std::cout << "  Uncompressed   : " << humanSize(static_cast<uint32_t>(totalUncompressed)) << "\n";

    // --- Native libraries --------------------------------------------------
    printHeader("Native Libraries");
    bool foundNative = false;
    for (const auto& e : zip.entries()) {
        if (e.name.rfind("lib/", 0) == 0 && e.name.size() > 4) {
            std::cout << "  " << e.name << " (" << humanSize(e.uncompressedSize) << ")\n";
            foundNative = true;
        }
    }
    if (!foundNative) std::cout << "  (none found)\n";

    // --- Signing info --------------------------------------------------
    printHeader("Signing / META-INF");
    bool foundSigning = false;
    for (const auto& e : zip.entries()) {
        if (e.name.rfind("META-INF/", 0) == 0 &&
            (e.name.size() > 4) &&
            (e.name.find(".RSA") != std::string::npos ||
             e.name.find(".DSA") != std::string::npos ||
             e.name.find(".EC") != std::string::npos ||
             e.name.find(".SF") != std::string::npos)) {
            std::cout << "  " << e.name << "\n";
            foundSigning = true;
        }
    }
    if (!foundSigning) std::cout << "  (no v1/META-INF signature files found - may use APK Signature Scheme v2/v3)\n";

    // --- AndroidManifest.xml --------------------------------------------
    printHeader("AndroidManifest.xml");
    if (zip.contains("AndroidManifest.xml")) {
        std::vector<uint8_t> manifestBytes;
        if (zip.extract("AndroidManifest.xml", manifestBytes)) {
            auto strings = axml::extractStrings(manifestBytes);
            auto perms = axml::filterPermissions(strings);

            std::cout << "  Strings found  : " << strings.size() << "\n";
            std::cout << "  Permissions    : " << perms.size() << "\n";
            for (const auto& p : perms) {
                std::cout << "    - " << p << "\n";
            }
        } else {
            std::cout << "  (failed to extract manifest bytes)\n";
        }
    } else {
        std::cout << "  (not found in archive)\n";
    }

    // --- classes.dex -----------------------------------------------------
    printHeader("classes.dex");
    if (zip.contains("classes.dex")) {
        std::vector<uint8_t> dexBytes;
        if (zip.extract("classes.dex", dexBytes)) {
            analyzeDex(dexBytes);
        } else {
            std::cout << "  (failed to extract classes.dex)\n";
        }
    } else {
        std::cout << "  (not found - could be a multi-dex or bundle-only APK)\n";
    }

    std::cout << "\nDone.\n";
    return 0;
}
