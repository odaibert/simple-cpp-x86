# Task Plan: Replicating the Big-Endian → x86 C++ Modernization

Use this plan to reproduce the full solution from scratch. Each task includes the exact GitHub Copilot prompt to use, the expected outcome, and validation steps. A developer with C++ and Azure experience can follow these tasks sequentially to build the entire repository.

---

## Prerequisites

| Requirement | Minimum Version |
|---|---|
| C++ compiler | GCC 10+, Clang 12+, or MSVC 2019+ |
| C++ standard | C++20 (`-std=c++20`) |
| CMake | 3.16+ |
| Editor | VS Code with GitHub Copilot extension |
| OS | Linux or macOS (x86/x64) |

---

## Task 1: Create the Legacy Big-Endian Source File

**Goal:** Write a C++ program that simulates a POS transaction record read from an IBM Power (OS/400) binary buffer. This file intentionally produces **wrong** output on x86 to demonstrate the endianness bug.

### Copilot Prompt

Open a new file `src/pos_transaction.cpp` in VS Code and use this prompt in Copilot Chat:

```
Write a C++ program that simulates reading a fixed-format binary transaction
record from an IBM Power Systems (OS/400) flat file.

Requirements:
1. Define a struct TxnRecord with these fields in order:
   - uint32_t txnId (Transaction ID)
   - uint32_t amountCents (amount in cents, e.g. 5000 = $50.00)
   - uint16_t storeNumber
   - uint16_t pumpNumber
   - char cardType[4] (e.g. "VISA")
2. Create a simulated Big-Endian binary buffer in main() with:
   txnId=1, amountCents=5000, storeNumber=100, pumpNumber=7, cardType="VISA"
   Write each integer in Big-Endian byte order (most significant byte first).
3. Use std::memcpy to copy the buffer into the struct — do NOT convert byte
   order. This is the legacy OS/400 pattern.
4. Print each field with labels: Txn ID, Amount ($), Store, Pump, Card.
5. Add detailed comments explaining that this code works correctly on
   Big-Endian but produces wrong values on x86 Little-Endian.
6. Use std::cout.write() for the cardType field (it is not null-terminated).
```

### Expected Output (on x86)

```
Txn ID     : 16777216       ← WRONG (should be 1)
Amount ($) : 2.28295e+07    ← WRONG (should be 50)
Store      : 25600          ← WRONG (should be 100)
Pump       : 1792           ← WRONG (should be 7)
Card       : VISA
```

### Validation

```bash
g++ -std=c++17 -o pos_legacy src/pos_transaction.cpp
./pos_legacy
```

Confirm the integer fields are scrambled and `Card` prints `VISA` correctly (char arrays are unaffected by endianness).

---

## Task 2: Refactor for x86 Using Copilot

**Goal:** Create the modernized version that reads the same Big-Endian buffer but produces correct values on x86 by swapping byte order for multi-byte integer fields.

### Copilot Prompt

Open `src/pos_transaction.cpp` in the editor, select the entire file, then use this prompt in Copilot Chat:

```
I am refactoring a C++ application to migrate it from a Big-Endian
architecture (IBM Power / OS/400) to a Little-Endian x86/x64 Linux
environment on Azure.

Analyze the selected code for endian-specific assumptions:
1. Flag any memcpy, memmove, or reinterpret_cast that copies raw bytes
   into structs containing multi-byte integer fields.
2. Refactor using portable C++ (prefer C++20 std::endian and C++23
   std::byteswap; fall back to compiler intrinsics for older standards).
3. Do NOT modify single-byte or character array fields.
4. Add comments explaining each conversion.
5. Keep the function signature and struct layout unchanged.

Additional requirements:
- Create inline helper functions fromBigEndian32() and fromBigEndian16()
  that use if constexpr (std::endian::native == std::endian::big) to
  no-op on Big-Endian hosts, and __builtin_bswap32/__builtin_bswap16
  on GCC/Clang, _byteswap_ulong/_byteswap_ushort on MSVC, with a
  manual fallback.
- Add static_assert(sizeof(TxnRecord) == 16) to catch padding issues.
- Use std::cout.write() for the cardType field.
```

### Post-Generation Steps

1. Save the output as `src/pos_transaction_x86.cpp`.
2. Verify that Copilot:
   - Added `#include <bit>` for `std::endian` (C++20).
   - Created `fromBigEndian32()` and `fromBigEndian16()` with `if constexpr`.
   - Inserted swap calls for `txnId`, `amountCents`, `storeNumber`, `pumpNumber` after `memcpy`.
   - Left `cardType` unchanged.
   - Added `static_assert(sizeof(TxnRecord) == 16)`.

### Expected Output (on x86)

```
Txn ID     : 1
Amount ($) : 50
Store      : 100
Pump       : 7
Card       : VISA
```

### Validation

```bash
g++ -std=c++20 -o pos_modern src/pos_transaction_x86.cpp
./pos_modern
```

Confirm all values match the expected output.

---

## Task 3: Create the CMake Build Configuration

**Goal:** Set up a `CMakeLists.txt` that builds both targets — the legacy version (C++17) and the modernized version (C++20).

### Copilot Prompt

```
Write a CMakeLists.txt for a project with two C++ executables:

1. pos_legacy — compiles src/pos_transaction.cpp with C++17
2. pos_modern — compiles src/pos_transaction_x86.cpp with C++20

Use cmake_minimum_required 3.16. Set the project name to
"BigEndianMigrationDemo". Add compiler warnings (-Wall -Wextra -Wpedantic
for GCC/Clang, /W4 for MSVC).
```

### Validation

```bash
cmake -B build
cmake --build build
./build/pos_legacy   # Should show wrong values
./build/pos_modern   # Should show correct values
```

---

## Task 4: Explain the Legacy Code (Practice Prompt)

**Goal:** Practice using Copilot's `/explain` capability to understand legacy code before refactoring it. This is the recommended first step when working with unfamiliar codebases.

### Copilot Prompt

Select the `processTxn` function in `src/pos_transaction.cpp`, then use:

```
Explain this C++ function. Specifically:
1. Does it assume a particular byte order (endianness)?
2. Are there any direct memory copies (memcpy, memmove) into structs with
   multi-byte integer fields?
3. Will this code produce correct results on an x86 Little-Endian system
   if the source data was written on a Big-Endian IBM Power system?
```

### Expected Copilot Response Should Cover

- The `memcpy` copies raw bytes without conversion.
- On x86, integer fields will be byte-swapped (e.g., `1` becomes `16,777,216`).
- `cardType` is a `char` array and is unaffected.
- A recommendation to add byte-swap utilities.

---

## Task 5: Generate an EBCDIC-to-UTF-8 Utility (Stretch Goal)

**Goal:** For codebases that also mix EBCDIC text with binary data, generate a conversion utility.

### Copilot Prompt

```
Write a C++ utility function that converts an EBCDIC-encoded character buffer
to UTF-8. The function should:
1. Accept a const char* input buffer and a size_t length.
2. Return a std::string containing the UTF-8 equivalent.
3. Handle common EBCDIC code page 037 (US/Canada) characters.
4. Be safe for use in a high-throughput POS transaction pipeline.
```

### Validation

- The generated function should contain a lookup table mapping EBCDIC byte values to ASCII/UTF-8 equivalents.
- Test with known EBCDIC values: `0xC1` → `'A'`, `0xF0` → `'0'`, `0x40` → `' '` (space).

---

## Task 6: Write the Solution Guide

**Goal:** Create the main documentation file that covers endianness theory, architecture differences, the Azure modernization workflow, and assessment checklists.

### Copilot Prompt

Use Copilot Chat (no file selected) or start a new markdown file `docs/modernizing-big-endian-cpp-for-azure.md`:

```
Write a technical solution guide in Microsoft Learn style for migrating a
Big-Endian C++ application from IBM Power Systems (OS/400) to Azure x86.

Structure:
1. Overview (2 paragraphs: problem summary, what this guide covers)
2. The Scenario (bullet list: high-volume POS, mature C++ codebase, OS/400
   APIs, sub-millisecond latency, goal is datacenter exit to Azure)
3. Section 1: Understanding Endianness (define it, show Big vs Little with
   memory diagrams for 0x12345678, explain why it matters in C/C++ with
   pointer casting and binary I/O examples, include the classic endian
   check code snippet)
4. Section 2: Machine Architecture Differences (table comparing IBM Power
   vs Azure x86 on: architecture RISC vs CISC, data storage endianness,
   scaling model vertical vs horizontal, primary OS, text encoding EBCDIC
   vs UTF-8; include an IBM Hardware Family quick reference table for
   IBM i, Power10, zSeries, x86)
5. Section 3: Problem Statement (three "silent killers": binary data
   corruption with numeric example, EBCDIC vs ASCII, pointer arithmetic
   assumptions)
6. Section 4: Modernize C++ Code for Azure (MS Learn architecture style
   with: Architecture diagram showing source→target, 7-step Dataflow,
   Components list with Azure services and links, Deployment options table
   with VM/AKS/Container Apps/App Service, Considerations using Azure
   Well-Architected Framework pillars: Reliability, Security, Cost
   Optimization with VM sizing table, Operational Excellence, Performance
   Efficiency)
7. Section 5: Accelerating Refactoring with GitHub Copilot (the generic
   prompt, before/after code pattern, before/after output)
8. Section 6: Assessment Checklist (checkboxes for Application Logic,
   Build Environment, Integration & Connectivity)
9. Section 7: The 5 Discovery Questions (latency tolerance, endianness
   exposure, OS/400 API surface, build & toolchain, success criteria)
10. Conclusion and Additional Resources (links to Microsoft Learn, GitHub
    Copilot docs, cppreference for std::endian and std::byteswap)

Use horizontal rules (---) between sections. Use proper markdown formatting.
Do not include any customer names or company-specific details.
```

### Validation

- All sections are present with correct numbering.
- Code blocks use proper syntax highlighting (```cpp, ```bash).
- All links to Microsoft Learn and external resources resolve correctly.
- No customer names, no DB2/Oracle references, no Skytap references.

---

## Task 7: Write the Copilot Modernization Guide

**Goal:** Create a step-by-step walkthrough showing how Copilot was used to generate the refactored code, including what changed and gotchas to watch for.

### Copilot Prompt

```
Write a technical guide titled "Using GitHub Copilot to Modernize OS/400 C++
Code for x86" in markdown.

Structure:
1. Problem Statement — why this migration is hard (byte order, memory-mapped
   structs, OS/400 APIs, shrinking talent pool)
2. Why This Migration Is Hard — Power processor specifics, community
   references to byte-swapping challenges
3. The Approach: GitHub Copilot as a Migration Accelerator — 4-step workflow
   (select, prompt, review, compile/test), include the full migration prompt
4. Example: Before and After — show the legacy pos_transaction.cpp code and
   the Copilot-generated pos_transaction_x86.cpp, with output for both
5. What Copilot Did — table with 5 rows showing each change and why
6. What to Watch For — three gotchas: Packed Decimals (COMP-3) with byte
   example, EBCDIC text encoding, struct padding/alignment with
   static_assert
7. Build and Test — compile commands for both versions
8. References — numbered list including Next Platform article on Power chips,
   Reddit cpp_questions thread, Microsoft Learn C++ docs, Azure services
   docs, cppreference for std::endian and std::byteswap

Use std::cout.write() for any char array output (cardType is not null-terminated).
```

### Validation

- Before/after code compiles and matches the source files in `src/`.
- The "What Copilot Did" table has 5 rows covering: `#include <bit>`, swap functions, swap calls after memcpy, skipping cardType, preserving struct layout.
- All three gotchas (COMP-3, EBCDIC, struct padding) are documented.

---

## Task 8: Create the Copilot Prompts Reference

**Goal:** A standalone file with copy-paste-ready prompts for use in other migration projects.

### Copilot Prompt

```
Create a markdown reference file with 4 GitHub Copilot prompts for C++
Big-Endian to Little-Endian migration:

1. Generic Endianness Refactoring — analyze selected code for endian issues,
   refactor with portable byte-swap, optimize for low latency
2. Explain Legacy Code — check if function assumes byte order, find memcpy
   into multi-byte structs, assess x86 correctness
3. Full File Refactoring — migrate entire file from IBM Power/OS/400 to Azure
   x86 Linux, add byte-swap utility (C++20 std::endian / C++23 std::byteswap
   with fallbacks), convert after memcpy, skip char arrays, add comments,
   preserve signatures
4. EBCDIC to UTF-8 Conversion — write a utility function for EBCDIC code page
   037 to UTF-8, accept const char* and size_t, return std::string, be safe
   for high-throughput pipelines

End with "Tips for Effective Copilot Usage" covering: select code first,
use /explain before /fix, iterate on suggestions, validate with tests.

Format each prompt as a fenced code block with context above it.
```

### Validation

- Each prompt is in a fenced code block and is self-contained.
- Tips section has 4 practical items.

---

## Task 9: Set Up the Repository

**Goal:** Initialize the Git repository with proper configuration files.

### Steps (Manual — No Copilot Needed)

1. **Initialize:**
   ```bash
   git init
   ```

2. **Create `.gitignore`:**
   ```
   build/
   *.o
   *.out
   pos_legacy
   pos_modern
   .DS_Store
   .vscode/
   .idea/
   CMakeCache.txt
   CMakeFiles/
   cmake_install.cmake
   Makefile
   ```

3. **Create `LICENSE`:** Use MIT license.

4. **Create `README.md`:**

   ### Copilot Prompt

   ```
   Write a README.md for a GitHub repository called "simple-cpp-x86".

   Contents:
   - Title: "Modernizing OS/400 C++ for x86 with GitHub Copilot"
   - Subtitle: practical guide and working code examples for migrating C++
     from IBM Power (Big-Endian) to Azure x86 (Little-Endian) using Copilot
   - The Problem: 1 paragraph explaining the endianness bug with memcpy
   - Repository Structure: tree diagram showing CMakeLists.txt, LICENSE,
     README, docs/ (3 markdown files), src/ (2 cpp files)
   - Quick Start: prerequisites (GCC 10+, CMake 3.16+), build with CMake,
     build directly with g++, run both targets with expected output
   - Key Concepts table: Byte Order, Text Encoding, Scaling Model, Compiler
     comparing IBM Power vs Azure x86
   - Documentation table: links to the 3 docs files with descriptions
   - References: links to Microsoft Learn C++, AKS, Cloud Adoption Framework,
     Copilot docs, cppreference
   - License: MIT
   ```

5. **Create `_config.yml`** (for GitHub Pages):
   ```yaml
   theme: jekyll-theme-cayman
   title: "Modernizing OS/400 C++ for x86 with GitHub Copilot"
   description: "Migrating C++ from IBM Power Systems (OS/400) to x86 on Azure with GitHub Copilot"
   ```

### Validation

```bash
git add -A
git status
```

Confirm all files are staged: `CMakeLists.txt`, `LICENSE`, `README.md`, `.gitignore`, `_config.yml`, `docs/` (3 files), `src/` (2 files).

---

## Task 10: Build, Test, and Commit

**Goal:** Verify the complete solution compiles and produces correct output, then make the initial commit.

### Steps

```bash
# Build
cmake -B build
cmake --build build

# Test legacy (expect wrong values)
./build/pos_legacy

# Test modern (expect correct values)
./build/pos_modern

# Commit
git add -A
git commit -m "Initial commit: Big-Endian to x86 C++ modernization guide"
```

### Expected Output — Legacy

```
=== Legacy OS/400 Transaction Processing ===

Txn ID     : 16777216
Amount ($) : 2.28295e+07
Store      : 25600
Pump       : 1792
Card       : VISA
```

### Expected Output — Modern

```
=== Modernized x86 Transaction Processing ===

Txn ID     : 1
Amount ($) : 50
Store      : 100
Pump       : 7
Card       : VISA
```

---

## Task 11: Push to GitHub and Enable Pages

**Goal:** Create the public GitHub repository, push, and enable GitHub Pages with the Cayman theme.

### Steps

```bash
# Create repo and push
gh repo create simple-cpp-x86 --public --source=. --push

# Enable GitHub Pages on the main branch
gh api repos/{owner}/simple-cpp-x86/pages \
  -X POST \
  -f "build_type=legacy" \
  -f "source[branch]=main" \
  -f "source[path]=/"
```

### Validation

- Visit `https://{owner}.github.io/simple-cpp-x86/` and confirm the README renders with the Cayman theme.
- Visit `https://{owner}.github.io/simple-cpp-x86/docs/modernizing-big-endian-cpp-for-azure.html` and confirm the solution guide renders.

---

## Summary Checklist

| Task | Deliverable | Key Copilot Prompt Used |
|------|-------------|------------------------|
| 1 | `src/pos_transaction.cpp` | Simulate Big-Endian POS record |
| 2 | `src/pos_transaction_x86.cpp` | Refactor for x86 with byte-swap |
| 3 | `CMakeLists.txt` | Build config for both targets |
| 4 | *(knowledge)* | Explain legacy code |
| 5 | *(utility function)* | EBCDIC to UTF-8 |
| 6 | `docs/modernizing-big-endian-cpp-for-azure.md` | Solution guide |
| 7 | `docs/copilot-modernization-guide.md` | Copilot walkthrough |
| 8 | `docs/copilot-prompts.md` | Prompt reference |
| 9 | `.gitignore`, `LICENSE`, `README.md`, `_config.yml` | README generation |
| 10 | *(verified build)* | — |
| 11 | *(live GitHub Pages site)* | — |

---

## Estimated Time

| Phase | Tasks | Estimated Duration |
|---|---|---|
| Code | 1–3 | 30 minutes |
| Documentation | 4–8 | 60 minutes |
| Repository Setup | 9–11 | 15 minutes |
| **Total** | | **~2 hours** |
