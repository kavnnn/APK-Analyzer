# APK Analyzer

A small, dependency-light C++ CLI tool that inspects Android APK files
without needing the Android SDK, `aapt`, or `apktool`.

It reads the APK's ZIP structure directly and reports:

- Archive summary (entry count, total uncompressed size)
- Native libraries (`lib/<abi>/*.so`)
- Signing / `META-INF` certificate files
- Strings and permissions pulled from the binary `AndroidManifest.xml`
- `classes.dex` header info (string/type/method/class counts)

## Build

Requires a C++17 compiler and zlib development headers.

```bash
# Debian/Ubuntu
sudo apt install build-essential zlib1g-dev

# macOS
brew install zlib
```

Build with CMake:

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

Or compile directly with g++:

```bash
g++ -std=c++17 -Iinclude -O2 src/*.cpp -lz -o apk-analyzer
```

## Usage

```bash
./apk-analyzer path/to/app.apk
```

## Preview

```
> Kavn: apk-analyzer myAPK.apk
APK Analyzer
File: myAPK.apk

== Archive Contents ==
  Entries        : 1061
  Uncompressed   : 73.5 MB

== Native Libraries ==
  lib/arm64-v8a/libAML.so (1.12 MB)
  lib/arm64-v8a/libGameEngine.so (8.50 MB)
  lib/arm64-v8a/libMultiplayer.so (3.93 MB)
  lib/arm64-v8a/libOpenAL64.so (1.65 MB)
  lib/arm64-v8a/libSCAnd.so (3.16 MB)
  lib/arm64-v8a/libbass.so (317.2 KB)
  lib/arm64-v8a/libbass_ssl.so (1.64 MB)
  lib/arm64-v8a/libcrashlytics-common.so (844.4 KB)
  lib/arm64-v8a/libcrashlytics-handler.so (216.9 KB)
  lib/arm64-v8a/libcrashlytics-trampoline.so (4.63 KB)
  lib/arm64-v8a/libcrashlytics.so (226.5 KB)
  lib/arm64-v8a/libdatastore_shared_counter.so (6.95 KB)
  lib/arm64-v8a/libluajit-5.1.so (948.9 KB)
  lib/arm64-v8a/libmonetloader.so (6.70 MB)
  lib/arm64-v8a/libpl_droidsonroids_gif.so (41.4 KB)
  lib/arm64-v8a/libshadowhook.so (72.8 KB)

== Signing / META-INF ==
  (no v1/META-INF signature files found - may use APK Signature Scheme v2/v3)

== AndroidManifest.xml ==
  Strings found  : 0
  Permissions    : 0

== classes.dex ==
  Magic          : dex\n038
  File size      : 8906096 bytes
  String IDs     : 60226
  Type IDs       : 9306
  Method IDs     : 65459
  Class defs     : 7381

Done.
```

## Project Structure

```
APK Analyzer/
├── src/
│   ├── main.cpp          # CLI entry point / report output
│   ├── zip_reader.cpp     # Minimal ZIP central directory reader
│   └── axml_parser.cpp    # Binary AndroidManifest.xml string pool parser
├── include/
│   ├── zip_reader.h
│   └── axml_parser.h
└── CMakeLists.txt
```

## How it works

- **ZIP reading**: APKs are just ZIP files. `zip_reader.cpp` parses the
  End of Central Directory record and Central Directory headers to list
  entries, then extracts individual files (stored or deflate) using zlib.
- **Manifest parsing**: `AndroidManifest.xml` inside an APK is stored in
  Android's binary XML (AXML) format, not plain text. `axml_parser.cpp`
  reads the global string pool chunk, which is enough to recover
  permissions, class names, and other useful strings without building a
  full XML tree.
- **DEX inspection**: `classes.dex` starts with a fixed-size header
  containing counts of strings, types, methods, and class definitions -
  read directly as a struct.

## Releases

Tagged pushes (`vX.Y.Z`) are automatically built for Linux, Windows, and
macOS via GitHub Actions, with binaries attached to the GitHub Release.

## License

[MIT License](https://raw.githubusercontent.com/kavnnn/APK-Analyzer/refs/heads/main/LICENSE)
