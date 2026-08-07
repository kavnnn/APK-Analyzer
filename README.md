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

MIT
