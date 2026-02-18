// pos_transaction.cpp — Legacy OS/400 version
// Originally compiled with IBM XL C/C++ on Power (Big-Endian)
//
// DEMONSTRATES THE BUG:
//   This code produces CORRECT results on a Big-Endian system (IBM Power / OS/400)
//   but WRONG results on a Little-Endian system (x86 / Azure VM).
//
// Compile:  g++ -std=c++17 -o pos_legacy pos_transaction.cpp
// Run:      ./pos_legacy

#include <iostream>
#include <cstring>
#include <cstdint>

// ---------------------------------------------------------------------------
// TxnRecord: Fixed-format binary record from a DB2 Physical File.
//
// On the OS/400, this struct is overlaid directly onto raw record buffers
// using memcpy. The byte order of the buffer (Big-Endian) matches the CPU's
// native integer representation, so no conversion is needed.
//
// On x86, this assumption breaks. The bytes are in the wrong order.
// ---------------------------------------------------------------------------
struct TxnRecord {
    uint32_t txnId;          // 4 bytes — Transaction ID
    uint32_t amountCents;    // 4 bytes — Amount in cents (5000 = $50.00)
    uint16_t storeNumber;    // 2 bytes — Store identifier
    uint16_t pumpNumber;     // 2 bytes — Fuel pump number
    char     cardType[4];    // 4 bytes — Card type ("VISA", "MC", etc.)
};

// ---------------------------------------------------------------------------
// processTxn: Read a raw binary buffer into TxnRecord and display the fields.
//
// BUG: On x86, the integer fields will contain garbage values because the
//      Big-Endian bytes are interpreted in Little-Endian order.
//
//      Example: txnId bytes 00 00 00 01
//        Big-Endian reads:    0x00000001 = 1           (correct)
//        Little-Endian reads: 0x01000000 = 16,777,216  (wrong)
// ---------------------------------------------------------------------------
void processTxn(const char* rawBuffer) {
    TxnRecord txn;

    // Direct memory copy — NO byte-order conversion.
    // This is the standard OS/400 pattern for reading DB2 record buffers.
    std::memcpy(&txn, rawBuffer, sizeof(TxnRecord));

    std::cout << "Txn ID     : " << txn.txnId       << "\n";
    std::cout << "Amount ($) : " << txn.amountCents / 100.0 << "\n";
    std::cout << "Store      : " << txn.storeNumber  << "\n";
    std::cout << "Pump       : " << txn.pumpNumber   << "\n";
    std::cout << "Card       : ";
    std::cout.write(txn.cardType, sizeof(txn.cardType));
    std::cout << "\n";
}

int main() {
    // Simulated Big-Endian binary buffer, as it would arrive from an
    // iSeries DB2 Physical File or Data Queue export:
    //
    //   txnId       =    1  →  00 00 00 01
    //   amountCents = 5000  →  00 00 13 88
    //   storeNumber =  100  →  00 64
    //   pumpNumber  =    7  →  00 07
    //   cardType    = VISA  →  56 49 53 41
    const char buffer[] = {
        0x00, 0x00, 0x00, 0x01,         // txnId = 1
        0x00, 0x00, 0x13, (char)0x88,   // amountCents = 5000
        0x00, 0x64,                     // storeNumber = 100
        0x00, 0x07,                     // pumpNumber = 7
        'V',  'I',  'S',  'A'          // cardType = "VISA"
    };

    std::cout << "=== Legacy OS/400 Transaction Processing ===\n\n";
    processTxn(buffer);

    return 0;
}
