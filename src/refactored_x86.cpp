/**
 * refactored_x86.cpp
 *
 * This file is the REFACTORED version of legacy_bigendian.cpp, produced
 * using GitHub Copilot-assisted migration guidance. It correctly handles
 * Big-Endian binary data on a Little-Endian (x86/x64) host.
 *
 * KEY CHANGES:
 *   1. Added a portable byte-swap utility using compile-time endianness
 *      detection (C++20 <bit> header).
 *   2. All multi-byte integer fields are explicitly converted from
 *      Big-Endian (network byte order) to the host's native byte order
 *      after the raw memcpy.
 *   3. Character data (status) is left untouched — endianness does not
 *      affect single-byte sequences.
 *
 * COMPILE:
 *   g++ -std=c++23 -o refactored_x86 refactored_x86.cpp
 *
 *   If C++23 std::byteswap is unavailable, fall back to:
 *   g++ -std=c++17 -o refactored_x86 refactored_x86.cpp
 *   (The code auto-detects and uses __builtin_bswap32 as a fallback.)
 *
 * CORRECT OUTPUT on Little-Endian (x86 / Azure VM):
 *   Transaction ID : 1
 *   Amount (cents) : 5000
 *   Terminal ID    : 42
 *   Status         : OK
 *   System is Little-Endian
 */

#include <iostream>
#include <cstring>
#include <cstdint>

// ---------------------------------------------------------------------------
// Portable byte-swap utility
//
// Strategy:
//   - C++20/23: Use std::endian for compile-time detection and
//               std::byteswap for the actual swap.
//   - C++17 fallback: Use GCC/Clang __builtin_bswap32.
//   - Manual fallback: Bitwise shift operations for maximum portability.
// ---------------------------------------------------------------------------
#if __cplusplus >= 202002L
    #include <bit>
    #define HAS_STD_ENDIAN 1
#else
    #define HAS_STD_ENDIAN 0
#endif

#if __cplusplus >= 202302L
    #define HAS_STD_BYTESWAP 1
#else
    #define HAS_STD_BYTESWAP 0
#endif

/**
 * swap32 - Convert a 32-bit integer from Big-Endian to the host's native
 *          byte order. If the host is already Big-Endian, this is a no-op.
 */
inline uint32_t swap32(uint32_t value) {
#if HAS_STD_ENDIAN
    if constexpr (std::endian::native == std::endian::big) {
        return value; // No swap needed — host is Big-Endian
    }
#endif

#if HAS_STD_BYTESWAP
    return std::byteswap(value);
#elif defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap32(value);
#elif defined(_MSC_VER)
    return _byteswap_ulong(value);
#else
    // Manual fallback — works everywhere
    return ((value >> 24) & 0x000000FF) |
           ((value >>  8) & 0x0000FF00) |
           ((value <<  8) & 0x00FF0000) |
           ((value << 24) & 0xFF000000);
#endif
}

/**
 * swap16 - Convert a 16-bit integer from Big-Endian to the host's native
 *          byte order.
 */
inline uint16_t swap16(uint16_t value) {
#if HAS_STD_ENDIAN
    if constexpr (std::endian::native == std::endian::big) {
        return value;
    }
#endif

#if HAS_STD_BYTESWAP
    return std::byteswap(value);
#elif defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap16(value);
#elif defined(_MSC_VER)
    return _byteswap_ushort(value);
#else
    return (value >> 8) | (value << 8);
#endif
}

// ---------------------------------------------------------------------------
// Struct: LegacyHeader
// Same layout as the original. The struct definition does not change —
// only the processing logic does.
// ---------------------------------------------------------------------------
struct LegacyHeader {
    uint32_t transactionId;   // 4 bytes — stored as Big-Endian in source data
    uint32_t amountCents;     // 4 bytes — stored as Big-Endian in source data
    uint32_t terminalId;      // 4 bytes — stored as Big-Endian in source data
    char     status[4];       // 4 bytes — character data (no swap needed)
};

// ---------------------------------------------------------------------------
// Function: processHeader (REFACTORED)
// After the raw memcpy, each multi-byte integer field is explicitly
// converted from Big-Endian to the host's native byte order.
// ---------------------------------------------------------------------------
void processHeader(const char* buffer) {
    LegacyHeader header;

    // Step 1: Copy raw bytes into the struct (same as legacy code).
    std::memcpy(&header, buffer, sizeof(LegacyHeader));

    // Step 2: Convert each integer field from Big-Endian to host order.
    // On a Little-Endian host (x86), this reverses the byte order.
    // On a Big-Endian host, swap32() is a no-op — zero overhead.
    header.transactionId = swap32(header.transactionId);
    header.amountCents   = swap32(header.amountCents);
    header.terminalId    = swap32(header.terminalId);
    // Note: header.status is character data — no conversion needed.

    std::cout << "Transaction ID : " << header.transactionId << std::endl;
    std::cout << "Amount (cents) : " << header.amountCents    << std::endl;
    std::cout << "Terminal ID    : " << header.terminalId      << std::endl;
    std::cout << "Status         : " << header.status          << std::endl;
}

// ---------------------------------------------------------------------------
// Function: checkEndianness
// Reports the host system's byte order using compile-time detection
// (C++20) with a runtime fallback for older standards.
// ---------------------------------------------------------------------------
void checkEndianness() {
#if HAS_STD_ENDIAN
    if constexpr (std::endian::native == std::endian::little) {
        std::cout << "System is Little-Endian (detected at compile time)" << std::endl;
    } else if constexpr (std::endian::native == std::endian::big) {
        std::cout << "System is Big-Endian (detected at compile time)" << std::endl;
    } else {
        std::cout << "System has mixed endianness" << std::endl;
    }
#else
    // Runtime fallback for C++17
    unsigned int i = 1;
    char *c = reinterpret_cast<char*>(&i);
    if (*c) {
        std::cout << "System is Little-Endian (detected at runtime)" << std::endl;
    } else {
        std::cout << "System is Big-Endian (detected at runtime)" << std::endl;
    }
#endif
}

// ---------------------------------------------------------------------------
// main
// Uses the SAME simulated Big-Endian buffer as the legacy code.
// This time, the output is CORRECT on x86.
// ---------------------------------------------------------------------------
int main() {
    // Simulated Big-Endian binary data (identical to legacy_bigendian.cpp):
    //   transactionId = 1       -> 0x00 0x00 0x00 0x01
    //   amountCents   = 5000    -> 0x00 0x00 0x13 0x88
    //   terminalId    = 42      -> 0x00 0x00 0x00 0x2A
    //   status        = "OK\0\0"
    const char rawData[] = {
        0x00, 0x00, 0x00, 0x01,         // transactionId = 1   (Big-Endian)
        0x00, 0x00, 0x13, (char)0x88,   // amountCents = 5000  (Big-Endian)
        0x00, 0x00, 0x00, 0x2A,         // terminalId = 42     (Big-Endian)
        'O',  'K',  '\0', '\0'          // status = "OK"
    };

    std::cout << "=== Refactored x86 Data Processing ===" << std::endl;
    std::cout << std::endl;

    processHeader(rawData);

    std::cout << std::endl;
    checkEndianness();

    return 0;
}
