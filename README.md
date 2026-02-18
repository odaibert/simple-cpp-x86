# Simple C++ x86: Big-Endian to Little-Endian Migration Demo

Companion code for the blog post **"Modernizing Big-Endian C++ Workloads for Azure"**.

This repository contains a before-and-after demonstration of how legacy C++ code written for IBM Power Systems (Big-Endian) breaks on x86 (Little-Endian) — and how to fix it using modern C++ and GitHub Copilot.

## Repository Structure

```
simple-cpp-x86/
├── CMakeLists.txt                  # Build configuration
├── README.md
├── docs/
│   ├── blog-post.md                # Full solution guide (Microsoft Learn style)
│   └── copilot-prompts.md          # Reusable GitHub Copilot prompts
└── src/
    ├── legacy_bigendian.cpp        # BEFORE: Buggy on x86 (assumes Big-Endian)
    └── refactored_x86.cpp          # AFTER:  Correct on all platforms
```

## Quick Start

### Build

```bash
cmake -B build
cmake --build build
```

### Run the "Before" (observe incorrect values on x86)

```bash
./build/legacy_bigendian
```

Expected output on an x86 Mac or Azure VM:

```
=== Legacy Big-Endian Data Processing ===

Transaction ID : 16777216      <-- WRONG (should be 1)
Amount (cents) : 2283798528    <-- WRONG (should be 5000)
Terminal ID    : 704643072     <-- WRONG (should be 42)
Status         : OK
```

### Run the "After" (observe correct values)

```bash
./build/refactored_x86
```

Expected output:

```
=== Refactored x86 Data Processing ===

Transaction ID : 1
Amount (cents) : 5000
Terminal ID    : 42
Status         : OK
```

## Key Concepts

| Concept | Source (IBM Power) | Target (Azure x86) |
|---|---|---|
| Byte Order | Big-Endian | Little-Endian |
| Text Encoding | EBCDIC | ASCII / UTF-8 |
| Scaling Model | Vertical | Horizontal |
| Compiler | IBM XL C/C++ | GCC / Clang / MSVC |

## GitHub Copilot Prompts

See [docs/copilot-prompts.md](docs/copilot-prompts.md) for ready-to-use prompts that automate endianness refactoring.

## License

This project is provided as reusable intellectual property content for educational and architectural assessment purposes.
