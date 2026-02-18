# Using GitHub Copilot to Modernize OS/400 C++ Code for x86

## Problem Statement

Organizations running C++ applications on IBM Power Systems (OS/400, IBM i)
face a growing challenge: the hardware is specialized, the talent pool is
shrinking, and the architecture is fundamentally incompatible with modern
cloud platforms.

The core issue is **not** the programming language. C++ compiles and runs on
virtually every platform. The issue is **how the code was written** — decades
of development on a Big-Endian, vertically scaled, single-system architecture
produced code that makes deep assumptions about byte order, memory layout, and
OS-level services that simply do not exist on x86 Linux or Windows.

Manually auditing and refactoring every `memcpy`, every binary file read, and
every pointer cast across a large codebase is slow, expensive, and error-prone.

**GitHub Copilot changes the economics of this migration.** It can identify
architecture-specific patterns, generate portable replacements, and explain
legacy logic — accelerating what would otherwise be months of manual work.

---

## Why This Migration Is Hard

IBM Power processors (including the Power10) are not just "different CPUs."
They are specialized engines with tightly integrated memory subsystems and
hardware accelerators that blur the traditional boundaries between processor,
memory, and I/O [[1]](#references).

When C++ code is written for this environment, developers often exploit these
characteristics directly:

- **Byte order:** Power is Big-Endian. x86 is Little-Endian. A 4-byte integer
  `0x00000001` (value `1`) is stored as bytes `00 00 00 01` on Power but
  `01 00 00 00` on x86. Any code that reads binary data into a struct with
  `memcpy` and then uses the integer fields will produce garbage on x86.

- **Memory-mapped structures:** OS/400 programs frequently overlay structs
  directly onto raw buffers from record-level file access or Data Queues.
  These patterns assume the CPU's native byte order matches the data.

- **System APIs:** OS/400 provides unique IPC mechanisms (Message Queues,
  Data Areas, User Spaces) that have no equivalent on Linux or Windows.

Community discussions on porting large C++ codebases from Big-Endian to
Little-Endian confirm that **byte-swapping is the single biggest source of
bugs**, and that automated tooling is essential for any codebase beyond
trivial size [[2]](#references).

---

## The Approach: GitHub Copilot as a Migration Accelerator

The workflow is straightforward:

1. **Select** the legacy function or struct in VS Code.
2. **Prompt** Copilot with the migration context.
3. **Review** the generated code — Copilot handles the byte-swap insertion,
   modern C++ idioms, and inline documentation.
4. **Compile and test** on the x86 target.

### The Prompt

Use this prompt in GitHub Copilot Chat. It is intentionally generic so it
works on any function, not just the example below.

```text
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
```

---

## Example: Before and After

### Source Code — OS/400 (Big-Endian)

This is a simplified but realistic pattern from a POS transaction processing
module. It reads a fixed-format binary record into a struct and processes
the fields directly.

```cpp
// pos_transaction.cpp — Legacy OS/400 version
// Compiled with IBM XL C/C++ on Power (Big-Endian)

#include <iostream>
#include <cstring>
#include <cstdint>

// Fixed-format binary record from a legacy flat file
struct TxnRecord {
    uint32_t txnId;          // Transaction ID
    uint32_t amountCents;    // Amount in cents (e.g., 5000 = $50.00)
    uint16_t storeNumber;    // Store identifier
    uint16_t pumpNumber;     // Fuel pump number
    char     cardType[4];    // "VISA", "MC", "AMEX", etc.
};

void processTxn(const char* rawBuffer) {
    TxnRecord txn;

    // Direct copy — works on Big-Endian because the buffer byte order
    // matches the CPU's native integer representation.
    std::memcpy(&txn, rawBuffer, sizeof(TxnRecord));

    std::cout << "Txn ID     : " << txn.txnId       << "\n";
    std::cout << "Amount ($) : " << txn.amountCents / 100.0 << "\n";
    std::cout << "Store      : " << txn.storeNumber  << "\n";
    std::cout << "Pump       : " << txn.pumpNumber   << "\n";
    std::cout << "Card       : " << txn.cardType     << "\n";
}

int main() {
    // Simulated Big-Endian buffer:
    //   txnId       = 1       → 00 00 00 01
    //   amountCents = 5000    → 00 00 13 88
    //   storeNumber = 100     → 00 64
    //   pumpNumber  = 7       → 00 07
    //   cardType    = "VISA"
    const char buffer[] = {
        0x00, 0x00, 0x00, 0x01,         // txnId = 1
        0x00, 0x00, 0x13, (char)0x88,   // amountCents = 5000
        0x00, 0x64,                     // storeNumber = 100
        0x00, 0x07,                     // pumpNumber = 7
        'V', 'I', 'S', 'A'             // cardType
    };

    processTxn(buffer);
    return 0;
}
```

**Output on OS/400 (Big-Endian) — CORRECT:**

```
Txn ID     : 1
Amount ($) : 50
Store      : 100
Pump       : 7
Card       : VISA
```

**Output on x86 (Little-Endian) — WRONG:**

```
Txn ID     : 16777216
Amount ($) : 2.28395e+07
Store      : 25600
Pump       : 1792
Card       : VISA
```

Every integer field is scrambled. The `char` array (`cardType`) is unaffected
because endianness only applies to multi-byte values.

---

### Copilot-Generated Code — x86 (Little-Endian)

After selecting `processTxn` and the struct, then pasting the prompt above
into Copilot Chat, the following code is generated:

```cpp
// pos_transaction_x86.cpp — Modernized for x86/x64 (Little-Endian)
// Compiled with GCC or Clang: g++ -std=c++20 -o pos_txn pos_transaction_x86.cpp

#include <iostream>
#include <cstring>
#include <cstdint>
#include <bit>       // C++20: std::endian

// ---------------------------------------------------------------------------
// Portable byte-swap utilities
// Uses compiler intrinsics for zero-overhead conversion.
// ---------------------------------------------------------------------------
inline uint32_t fromBigEndian32(uint32_t v) {
    if constexpr (std::endian::native == std::endian::big)
        return v;  // No-op on Big-Endian hosts

    #if defined(__GNUC__) || defined(__clang__)
        return __builtin_bswap32(v);
    #elif defined(_MSC_VER)
        return _byteswap_ulong(v);
    #else
        return ((v >> 24) & 0xFF)
             | ((v >>  8) & 0xFF00)
             | ((v <<  8) & 0xFF0000)
             | ((v << 24) & 0xFF000000);
    #endif
}

inline uint16_t fromBigEndian16(uint16_t v) {
    if constexpr (std::endian::native == std::endian::big)
        return v;

    #if defined(__GNUC__) || defined(__clang__)
        return __builtin_bswap16(v);
    #elif defined(_MSC_VER)
        return _byteswap_ushort(v);
    #else
        return (v >> 8) | (v << 8);
    #endif
}

// ---------------------------------------------------------------------------
// Struct layout is UNCHANGED — same binary format as the OS/400 original.
// ---------------------------------------------------------------------------
struct TxnRecord {
    uint32_t txnId;
    uint32_t amountCents;
    uint16_t storeNumber;
    uint16_t pumpNumber;
    char     cardType[4];
};

// ---------------------------------------------------------------------------
// processTxn — Refactored for x86
// After the raw memcpy, each multi-byte field is converted from Big-Endian
// (the source data format) to the host's native byte order.
// Character arrays are NOT swapped — endianness does not affect them.
// ---------------------------------------------------------------------------
void processTxn(const char* rawBuffer) {
    TxnRecord txn;
    std::memcpy(&txn, rawBuffer, sizeof(TxnRecord));

    // Convert Big-Endian source data → host byte order
    txn.txnId       = fromBigEndian32(txn.txnId);
    txn.amountCents = fromBigEndian32(txn.amountCents);
    txn.storeNumber = fromBigEndian16(txn.storeNumber);
    txn.pumpNumber  = fromBigEndian16(txn.pumpNumber);
    // txn.cardType — no conversion (char array)

    std::cout << "Txn ID     : " << txn.txnId       << "\n";
    std::cout << "Amount ($) : " << txn.amountCents / 100.0 << "\n";
    std::cout << "Store      : " << txn.storeNumber  << "\n";
    std::cout << "Pump       : " << txn.pumpNumber   << "\n";
    std::cout << "Card       : " << txn.cardType     << "\n";
}

int main() {
    // Same Big-Endian buffer — data has not changed, only interpretation has.
    const char buffer[] = {
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x13, (char)0x88,
        0x00, 0x64,
        0x00, 0x07,
        'V', 'I', 'S', 'A'
    };

    processTxn(buffer);
    return 0;
}
```

**Output on x86 (Little-Endian) — CORRECT:**

```
Txn ID     : 1
Amount ($) : 50
Store      : 100
Pump       : 7
Card       : VISA
```

---

## What Copilot Did

| Step | What Changed | Why |
|------|-------------|-----|
| 1 | Added `#include <bit>` | Enables `std::endian` for compile-time detection (C++20). |
| 2 | Created `fromBigEndian32()` and `fromBigEndian16()` | Portable byte-swap with compiler intrinsics — zero overhead. |
| 3 | Added swap calls after `memcpy` for each integer field | Converts source data from Big-Endian to host order. |
| 4 | Skipped `cardType` | Single-byte sequences are unaffected by endianness. |
| 5 | Preserved struct layout and function signature | No changes to the binary interface — existing data files still work. |

The `if constexpr` check on `std::endian::native` is resolved **at compile
time**. On an x86 build, the compiler unconditionally emits the swap
instructions. On a Big-Endian build, the swaps are eliminated entirely. There
is **zero runtime cost**.

---

## What to Watch For

### 1. Packed Decimals (COMP-3)

IBM systems store decimal numbers in a packed format where each byte holds
two digits, with the last nibble as the sign. This is **not** an endianness
issue — it is a proprietary encoding. You need a dedicated parser:

```cpp
// Packed decimal: value 12345 with positive sign
// Stored as: 0x01 0x23 0x45 0x0C
//            1  2  3  4  5  +
```

Copilot can generate a packed-decimal parser if you describe the format in
the prompt.

### 2. EBCDIC Text

OS/400 uses EBCDIC encoding for character data. The letter `A` is `0xC1` in
EBCDIC but `0x41` in ASCII. If your binary records mix integers with text,
you need both byte-swapping (for integers) and character-set conversion (for
text) in the same processing pipeline.

### 3. Struct Padding and Alignment

IBM XL C++ and GCC may pad structs differently. Always verify with
`static_assert(sizeof(TxnRecord) == 16)` to catch alignment mismatches
at compile time.

---

## Build and Test

```bash
# Compile the legacy version (shows the bug on x86)
g++ -std=c++17 -o pos_legacy src/pos_transaction.cpp
./pos_legacy
# → Txn ID: 16777216  (WRONG)

# Compile the modernized version (correct on x86)
g++ -std=c++20 -o pos_modern src/pos_transaction_x86.cpp
./pos_modern
# → Txn ID: 1  (CORRECT)
```

---

## References

1. **Timothy Prickett Morgan.** "IBM Power Chips Blur The Lines To Memory
   And Accelerators." *The Next Platform*, August 28, 2018.
   [https://www.nextplatform.com/2018/08/28/ibm-power-chips-blur-the-lines-to-memory-and-accelerators/](https://www.nextplatform.com/2018/08/28/ibm-power-chips-blur-the-lines-to-memory-and-accelerators/)

2. **r/cpp_questions.** "Question on approach converting C++ codebase from
   Big-Endian to Little-Endian." Reddit, 2024.
   [https://www.reddit.com/r/cpp_questions/comments/1r31shp/question_on_approach_converting_c_codebase_from/](https://www.reddit.com/r/cpp_questions/comments/1r31shp/question_on_approach_converting_c_codebase_from/)

3. **Microsoft Learn.** "C++ language documentation."
   [https://learn.microsoft.com/cpp/cpp/](https://learn.microsoft.com/cpp/cpp/)

4. **Microsoft Learn.** "C and C++ in Visual Studio."
   [https://learn.microsoft.com/cpp/overview/visual-cpp-in-visual-studio](https://learn.microsoft.com/cpp/overview/visual-cpp-in-visual-studio)

5. **Microsoft Learn.** "Azure Kubernetes Service (AKS) documentation."
   [https://learn.microsoft.com/azure/aks/](https://learn.microsoft.com/azure/aks/)

6. **Microsoft Learn.** "Cloud Adoption Framework — Modernize."
   [https://learn.microsoft.com/azure/cloud-adoption-framework/modernize/](https://learn.microsoft.com/azure/cloud-adoption-framework/modernize/)

7. **Microsoft Learn.** "Porting and upgrading C++ code."
   [https://learn.microsoft.com/cpp/porting/visual-cpp-porting-and-upgrading-guide](https://learn.microsoft.com/cpp/porting/visual-cpp-porting-and-upgrading-guide)

8. **GitHub.** "GitHub Copilot documentation."
   [https://docs.github.com/copilot](https://docs.github.com/copilot)

9. **Microsoft Learn.** "Azure Virtual Machines documentation."
   [https://learn.microsoft.com/azure/virtual-machines/](https://learn.microsoft.com/azure/virtual-machines/)

10. **cppreference.com.** "`std::endian` (C++20)."
    [https://en.cppreference.com/w/cpp/types/endian](https://en.cppreference.com/w/cpp/types/endian)

11. **cppreference.com.** "`std::byteswap` (C++23)."
    [https://en.cppreference.com/w/cpp/numeric/byteswap](https://en.cppreference.com/w/cpp/numeric/byteswap)
