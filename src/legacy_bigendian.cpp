/**
 * legacy_bigendian.cpp
 *
 * This file simulates legacy C++ code originally written for an IBM Power
 * Systems environment (Big-Endian, OS/400). It demonstrates common patterns
 * that WILL FAIL when recompiled for an x86/x64 (Little-Endian) target
 * without proper byte-order handling.
 *
 * PURPOSE: Use this file as the "before" example when demonstrating
 *          GitHub Copilot-assisted refactoring.
 *
 * COMPILE (on x86, to see the bug):
 *   g++ -std=c++17 -o legacy_bigendian legacy_bigendian.cpp
 *
 * EXPECTED OUTPUT on Big-Endian (P10):
 *   Transaction ID : 1
 *   Amount (cents) : 5000
 *   Terminal ID    : 42
 *   Status         : OK
 *   System is Big-Endian
 *
 * ACTUAL OUTPUT on Little-Endian (x86 / Azure VM):
 *   Transaction ID : 16777216      <-- WRONG (0x01000000 instead of 0x00000001)
 *   Amount (cents) : 2283798528    <-- WRONG
 *   Terminal ID    : 704643072     <-- WRONG
 *   Status         : OK            <-- Text is unaffected
 *   System is Little-Endian
 */

#include <iostream>
#include <cstring>
#include <cstdint>

// ---------------------------------------------------------------------------
// Struct: LegacyHeader
// Represents a fixed-format binary record header, as it would appear in a
// DB2 flat-file export or a raw network packet from the iSeries.
// On the Big-Endian source system, multi-byte integers are stored with the
// most significant byte first.
// ---------------------------------------------------------------------------
struct LegacyHeader {
    uint32_t transactionId;   // 4 bytes — Big-Endian on source
    uint32_t amountCents;     // 4 bytes — Big-Endian on source
    uint32_t terminalId;      // 4 bytes — Big-Endian on source
    char     status[4];       // 4 bytes — character data (unaffected by endianness)
};

// ---------------------------------------------------------------------------
// Function: processHeader
// Reads raw bytes into the struct using a direct memcpy. This is the
// standard pattern on the iSeries where the byte order matches the struct
// layout. On x86, this produces INCORRECT integer values.
// ---------------------------------------------------------------------------
void processHeader(const char* buffer) {
    LegacyHeader header;

    // BUG ON x86: Direct memory copy assumes the buffer's byte order matches
    // the host CPU's byte order. On Little-Endian x86, it does NOT.
    std::memcpy(&header, buffer, sizeof(LegacyHeader));

    std::cout << "Transaction ID : " << header.transactionId << std::endl;
    std::cout << "Amount (cents) : " << header.amountCents    << std::endl;
    std::cout << "Terminal ID    : " << header.terminalId      << std::endl;
    std::cout << "Status         : " << header.status          << std::endl;
}

// ---------------------------------------------------------------------------
// Function: checkEndianness
// A classic technique to detect the host system's byte order at runtime.
// ---------------------------------------------------------------------------
void checkEndianness() {
    unsigned int i = 1;
    char *c = (char*)&i;

    if (*c) {
        std::cout << "System is Little-Endian" << std::endl;
    } else {
        std::cout << "System is Big-Endian" << std::endl;
    }
}

// ---------------------------------------------------------------------------
// main
// Constructs a simulated Big-Endian binary buffer (as if read from an iSeries
// flat file) and processes it.
// ---------------------------------------------------------------------------
int main() {
    // Simulated Big-Endian binary data:
    //   transactionId = 1       -> 0x00 0x00 0x00 0x01
    //   amountCents   = 5000    -> 0x00 0x00 0x13 0x88
    //   terminalId    = 42      -> 0x00 0x00 0x00 0x2A
    //   status        = "OK\0\0"
    const char rawData[] = {
        0x00, 0x00, 0x00, 0x01,   // transactionId = 1   (Big-Endian)
        0x00, 0x00, 0x13, (char)0x88,   // amountCents = 5000  (Big-Endian)
        0x00, 0x00, 0x00, 0x2A,   // terminalId = 42     (Big-Endian)
        'O',  'K',  '\0', '\0'    // status = "OK"
    };

    std::cout << "=== Legacy Big-Endian Data Processing ===" << std::endl;
    std::cout << std::endl;

    processHeader(rawData);

    std::cout << std::endl;
    checkEndianness();

    return 0;
}
