#pragma once
#include <string>
#include <vector>
#include <cstdint>

// Reads the global string pool out of a binary AndroidManifest.xml
// (the AXML format). This does not build a full attribute/element
// tree, but the string pool alone is enough to recover useful
// information: package name, permissions, activity/service class
// names, etc.
namespace axml {

// Returns every UTF-8/UTF-16 string stored in the manifest's string pool.
std::vector<std::string> extractStrings(const std::vector<uint8_t>& manifestBytes);

// Convenience filter: strings that look like "android.permission.*"
// or custom "<package>.permission.*" entries.
std::vector<std::string> filterPermissions(const std::vector<std::string>& strings);

} // namespace axml
