// pos_transaction_x86.cpp — Modernized for x86/x64 (Little-Endian)
// Generated with assistance from GitHub Copilot
//
// This is the REFACTORED version of pos_transaction.cpp. It correctly reads
// Big-Endian binary data (from a legacy OS/400 flat file) on an x86 host.
//
// Compile:  g++ -std=c++20 -o pos_modern pos_transaction_x86.cpp
// Run:      ./pos_modern

#include <iostream>
#include <cstring>
#include <cstdint>
#include <bit>       // C++20: std::endian for compile-time byte-order detection

// ---------------------------------------------------------------------------
// Portable byte-swap utilities
//
// These functions convert multi-byte integers from Big-Endian (the source
// data format on OS/400) to the host CPU's native byte order.
//
// On a Little-Endian host (x86), the bytes are reversed.
// On a Big-Endian host, the functions are no-ops (zero overhead).
//
// The `if constexpr` check is resolved at COMPILE TIME — there is no
// runtime branching cost.
// ---------------------------------------------------------------------------

/**
 * fromBigEndian32 — Convert a 32-bit Big-Endian value to host byte order.
 */
inline uint32_t fromBigEndian32(uint32_t v) {
    if constexpr (std::endian::native == std::endian::big)
        return v;  // Already in the correct order

    // Use compiler intrinsics for single-instruction byte reversal
    #if defined(__GNUC__) || defined(__clang__)
        return __builtin_bswap32(v);
    #elif defined(_MSC_VER)
        return _byteswap_ulong(v);
    #else
        // Manual fallback — portable to any C++ compiler
        return ((v >> 24) & 0x000000FF)
             | ((v >>  8) & 0x0000FF00)
             | ((v <<  8) & 0x00FF0000)
             | ((v << 24) & 0xFF000000);
    #endif
}

/**
 * fromBigEndian16 — Convert a 16-bit Big-Endian value to host byte order.
 */
inline uint16_t fromBigEndian16(uint16_t v) {
    if constexpr (std::endian::native == std::endian::big)
        return v;

    #if defined(__GNUC__) || defined(__clang__)
        return __builtin_bswap16(v);
    #elif defined(_MSC_VER)
        return _byteswap_ushort(v);
    #else
        return static_cast<uint16_t>((v >> 8) | (v << 8));
    #endif
}

// ---------------------------------------------------------------------------
// TxnRecord: UNCHANGED struct layout.
//
// The binary format is identical to the OS/400 version. This is critical:
// it means existing data files, network packets, and legacy exports remain
// compatible without any reformatting.
// ---------------------------------------------------------------------------
struct TxnRecord {
    uint32_t txnId;          // 4 bytes — Transaction ID
    uint32_t amountCents;    // 4 bytes — Amount in cents (5000 = $50.00)
    uint16_t storeNumber;    // 2 bytes — Store identifier
    uint16_t pumpNumber;     // 2 bytes — Fuel pump number
    char     cardType[4];    // 4 bytes — Card type ("VISA", "MC", etc.)
};

// Compile-time guard: ensure no unexpected padding was inserted
static_assert(sizeof(TxnRecord) == 16,
    "TxnRecord size mismatch — check struct alignment/padding");

// ---------------------------------------------------------------------------
// processTxn — REFACTORED for x86
//
// Step 1: memcpy raw bytes into the struct (same as legacy).
// Step 2: Convert each multi-byte integer from Big-Endian to host order.
// Step 3: Character arrays (cardType) are NOT converted — endianness does
//         not affect single-byte sequences.
// ---------------------------------------------------------------------------
void processTxn(const char* rawBuffer) {
    TxnRecord txn;

    // Step 1: Raw copy (identical to OS/400 code)
    std::memcpy(&txn, rawBuffer, sizeof(TxnRecord));

    // Step 2: Byte-order conversion — Big-Endian source → host order
    txn.txnId       = fromBigEndian32(txn.txnId);
    txn.amountCents = fromBigEndian32(txn.amountCents);
    txn.storeNumber = fromBigEndian16(txn.storeNumber);
    txn.pumpNumber  = fromBigEndian16(txn.pumpNumber);
    // cardType: char array — no conversion needed

    // Step 3: Process as normal
    std::cout << "Txn ID     : " << txn.txnId       << "\n";
    std::cout << "Amount ($) : " << txn.amountCents / 100.0 << "\n";
    std::cout << "Store      : " << txn.storeNumber  << "\n";
    std::cout << "Pump       : " << txn.pumpNumber   << "\n";
    std::cout << "Card       : ";
    std::cout.write(txn.cardType, sizeof(txn.cardType));
    std::cout << "\n";
}

int main() {
    // Same Big-Endian buffer as the legacy version.
    // The DATA has not changed — only the INTERPRETATION has.
    const char buffer[] = {
        0x00, 0x00, 0x00, 0x01,         // txnId = 1
        0x00, 0x00, 0x13, (char)0x88,   // amountCents = 5000
        0x00, 0x64,                     // storeNumber = 100
        0x00, 0x07,                     // pumpNumber = 7
        'V',  'I',  'S',  'A'          // cardType = "VISA"
    };

    std::cout << "=== Modernized x86 Transaction Processing ===\n\n";
    processTxn(buffer);

    return 0;
}
