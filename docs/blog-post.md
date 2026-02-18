# Solution Guide: Modernizing Big-Endian C++ Workloads for Azure

## Overview

Migrating mission-critical applications from legacy midrange or mainframe systems to the cloud is rarely a simple "lift and shift" operation. It involves a fundamental change in underlying infrastructure, operating paradigms, and—most critically—how the computer interprets raw data.

This guide walks through the architectural considerations and practical strategies for migrating a high-volume, latency-sensitive C++ application from an IBM Power Systems environment to Microsoft Azure. It is written to be reusable across similar migration engagements.

---

## The Scenario

An organization relies on a custom-built C++ application running on IBM Power hardware (with an operating system such as IBM i / OS/400). The system:

- Handles **high-volume, real-time transactions** (e.g., Point-of-Sale).
- Uses **DB2 stored procedures** for approximately 30% of its business logic.
- Has dependencies on **on-premises Oracle databases**.
- Must maintain **sub-millisecond latency** for customer-facing operations.

The goal is to **retire the physical datacenter** while maintaining performance and unlocking modern cloud capabilities such as AI and analytics.

---

## Section 1: Understanding Endianness

### What is Endianness?

Endianness is the "reading direction" a computer uses to store multi-byte data (like an integer) in memory. When a piece of data occupies more than one byte, the system must decide which end of the number to write first.

### The Two Main Types

Consider the hexadecimal number `0x12345678`. It has a "big end" (`12`) and a "little end" (`78`).

#### Big-Endian (The "Human" Way)

The most significant byte is stored at the lowest memory address. It reads the way we write numbers.

```
Memory: | 12 | 34 | 56 | 78 |
```

Used by: **IBM Power Systems, IBM Mainframes (zSeries), Network protocols (TCP/IP).**

#### Little-Endian (The "Standard" Way)

The least significant byte is stored first.

```
Memory: | 78 | 56 | 34 | 12 |
```

Used by: **Intel/AMD x86/x64, ARM (default mode), Azure Virtual Machines.**

### Why Does This Matter in C and C++?

C and C++ provide low-level access to memory. Endianness becomes critical in two scenarios:

1.  **Pointer Casting:** If you take a 4-byte integer and use a `char*` pointer to inspect the first byte, a Little-Endian system returns `0x78`, while a Big-Endian system returns `0x12`.

2.  **Network Communication & Binary I/O:** The Internet uses Big-Endian ("network byte order"). If a C++ application sends a raw binary struct from a Little-Endian PC without converting it, the receiver will interpret the numbers incorrectly.

A classic check for system endianness:

```cpp
unsigned int i = 1;
char *c = (char*)&i;

if (*c) {
    printf("Little Endian\n");
} else {
    printf("Big Endian\n");
}
```

---

## Section 2: Understanding Machine Architecture Differences

A successful migration requires recognizing the distinct characteristics of the source and destination hardware.

| Feature | IBM Power Systems (Source) | Azure x86/x64 (Target) | Migration Implication |
|---|---|---|---|
| **Architecture** | RISC (Reduced Instruction Set) | CISC (Complex Instruction Set) | Code must be recompiled for the new instruction set. |
| **Data Storage** | Big-Endian | Little-Endian | **Critical:** Binary data and memory operations must be refactored or byte-swapped. |
| **Scaling Model** | Vertical: Add more power to a single, massive server. | Horizontal: Distribute workloads across many smaller VMs or containers. | Application architecture may need adjustment for distributed cloud patterns. |
| **Primary OS** | IBM i (OS/400) | Windows Server, Linux | System-level APIs and IPC mechanisms will change. |
| **Text Encoding** | EBCDIC | ASCII / UTF-8 | Character data requires a translation layer during migration. |

### Quick Reference: IBM Hardware Family

| Technology | IBM i / iSeries | IBM Power10 (P10) | IBM Mainframe (zSeries) | x86 (Intel/AMD) |
|---|---|---|---|---|
| **Category** | The Platform (OS + DB + Runtime) | The Processor (Latest Gen) | The "Big Iron" (Enterprise Scale) | Commodity Hardware (Cloud Standard) |
| **Endianness** | Big-Endian | Big-Endian | Big-Endian | Little-Endian |
| **Architecture** | RISC (Power-based) | RISC (Power-based) | CISC (Proprietary) | CISC (Universal) |
| **Best For** | Integrated business logic & DB2 | High-performance enterprise workloads | Massive transaction volume (10B+/day) | General purpose, Cloud, and Web apps |
| **Scaling** | Vertical | Vertical | Vertical (near-infinite redundancy) | Horizontal |

---

## Section 3: The Problem Statement

### The "Silent Killers" of a Cross-Architecture Migration

When moving a C++ application from a Big-Endian system to Azure, three issues surface:

### 1. Binary Data Corruption

A value that was supposed to be `1` on the source system (stored as `00 00 00 01`) will be read as `16,777,216` on a Little-Endian system (interpreted as `01 00 00 00`).

### 2. EBCDIC vs. ASCII

Source systems typically use EBCDIC text encoding, while the target uses ASCII/UTF-8. When binary numbers and EBCDIC text are mixed in a single flat file or data stream, both the bytes and the characters need to be translated.

### 3. Pointer Arithmetic Assumptions

Legacy code often assumes the first byte of an integer is the most significant. If that code is recompiled for x86 without modification, conditional logic based on byte inspection will break.

---

## Section 4: Migration Strategies on Azure

There are several approaches, each balancing speed against long-term value.

### Option 1: Rehost via Hybrid Bridge

> **TL;DR:** Keep the application running on native IBM Power infrastructure while connecting it to Azure services through a private network.

- **How:** Migrate the IBM Power workloads to **IBM Power Virtual Server (PowerVS)** on IBM Cloud, then establish an **Azure ExpressRoute** connection to link PowerVS directly into your Azure virtual network. Alternatively, evaluate ISV partner solutions available through the **Azure Modernization Center** that can emulate or host legacy Power workloads on Azure infrastructure.
- **Pros:** No code changes. The application continues to run in a Big-Endian environment. Azure services (AI, analytics, storage) are accessible over a private, low-latency backbone.
- **Cons:** Higher operational cost (maintaining two cloud fabrics). The application remains in a legacy architecture with limited horizontal scaling.
- **Best For:** Meeting a hard datacenter exit deadline while buying time for a full modernization.

### Option 2: Replatform ("Lift, Tinker, and Shift")

> **TL;DR:** Move to a managed service (PaaS) with minimal code changes.

- **How:** Swap the underlying platform (e.g., move DB2 to Azure SQL Managed Instance) while keeping the application logic largely intact.
- **Pros:** Reduced operational overhead (no more OS patching). Lower cost than rehosting.
- **Cons:** Requires database conversion effort.
- **Best For:** Reducing operational burden without a full application rewrite.

### Option 3: Modernize / Refactor

> **TL;DR:** Modify the application's code to leverage cloud-native features.

- **How:** Recompile C++ for Linux containers. Deploy to Azure Kubernetes Service (AKS). Use GitHub Copilot to handle Endianness refactoring.
- **Pros:** Unlocks horizontal scaling, AI integration, and modern DevOps pipelines.
- **Cons:** Requires significant engineering effort and testing.
- **Best For:** Long-term strategic value, performance optimization, and AI readiness.

### Option 4: Rearchitect / Rebuild

> **TL;DR:** Build a new, cloud-native solution from scratch.

- **How:** Redesign the application using serverless (Azure Functions) or microservices.
- **Pros:** Maximum cloud-native benefit.
- **Cons:** Highest cost and timeline. Impractical for large, mature codebases.
- **Best For:** Specific sub-services or APIs rather than the entire legacy stack.

### Recommended Approach: The Phased Hybrid

For most engagements of this type, a **two-phase approach** is recommended:

1.  **Phase 1 — Bridge:** Move the IBM Power workloads to IBM PowerVS and connect them to Azure via ExpressRoute. This retires the physical datacenter while keeping the application logic intact. Simultaneously, begin recompiling non-critical C++ modules for Linux x86 on Azure Virtual Machines.
2.  **Phase 2 — Modernize:** Systematically refactor critical APIs and business logic into AKS using GitHub Copilot, addressing Endianness and unlocking AI and analytics capabilities. As each service is validated on Azure, decommission its PowerVS counterpart until the bridge is fully retired.

---

## Section 5: Accelerating Refactoring with GitHub Copilot

Manual refactoring of legacy C++ code for Endianness is time-consuming and error-prone. GitHub Copilot can significantly accelerate this process.

### The Generic Copilot Prompt

Use this prompt in GitHub Copilot Chat to convert any Big-Endian C++ code for an x86 target:

```
I am refactoring a C++ application to migrate it from a Big-Endian architecture
to a Little-Endian (x86/x64) cloud environment.

Please analyze the following code for endian-specific logic, specifically where
binary data is read into structs or where multi-byte integers are manipulated
directly.

1. Identify areas where data corruption will occur due to byte-order differences.
2. Refactor the code to use portable, standard C++ methods (such as byte-swapping
   utilities or network-to-host functions) to ensure data integrity.
3. Ensure the solution is optimized for low-latency performance.
```

### Example: Before and After

**Legacy Code Pattern (Big-Endian Assumption):**

```cpp
#include <cstring>
#include <cstdint>

struct DataPacket {
    uint32_t id;
    uint32_t value;
};

void processBuffer(char* rawInput) {
    DataPacket packet;
    // Direct memory copy — assumes Big-Endian byte order.
    // This will result in scrambled data on Little-Endian x86.
    std::memcpy(&packet, rawInput, sizeof(DataPacket));
    // ... process packet ...
}
```

**Refactored Code (Portable, Cross-Platform):**

```cpp
#include <cstring>
#include <cstdint>
#include <bit> // Requires C++20/23

struct DataPacket {
    uint32_t id;
    uint32_t value;
};

void processBuffer(char* rawInput) {
    DataPacket packet;
    std::memcpy(&packet, rawInput, sizeof(DataPacket));

    // Conditionally swap bytes only when running on a Little-Endian host.
    // std::byteswap converts Big-Endian source data to native format.
    if constexpr (std::endian::native == std::endian::little) {
        packet.id    = std::byteswap(packet.id);
        packet.value = std::byteswap(packet.value);
    }
    // ... process packet ...
}
```

> **Key Insight:** The `if constexpr` check is evaluated at compile time, so there is **zero runtime overhead** on Little-Endian systems. The compiler simply includes the swap instructions unconditionally.

---

## Section 6: Database and Connectivity Considerations

### Converting Stored Procedures

Business logic in DB2 stored procedures should be assessed for migration to **Azure SQL Managed Instance**. The recommended tooling:

-   **SQL Server Migration Assistant (SSMA) for DB2** automates schema and simpler procedural logic conversion to T-SQL.
-   Complex stored procedures (especially those calling external programs like RPG or COBOL) may require manual refactoring or moving the logic into the C++ application tier.

### Key Risk: Packed Decimals (COMP-3)

IBM systems use a specific format called **Packed Decimal** where two digits are stored in a single byte. This is neither Big-Endian nor Little-Endian—it is a proprietary encoding that requires a dedicated parser during migration.

### Managing Latency for Hybrid Dependencies

If the application maintains connectivity to external on-premises databases (e.g., Oracle), network latency can degrade performance.

-   **Azure ExpressRoute** provides a private, dedicated network connection between on-premises infrastructure and Azure.
-   **Oracle Database@Azure** co-locates Oracle hardware directly within Azure regions, providing sub-2ms latency between compute and data.

### Azure VM Performance Mapping

When sizing Azure infrastructure to match Power Systems performance:

| Power Systems Feature | Recommended Azure VM Series | Rationale |
|---|---|---|
| High Memory per Core | **E-Series (Ev5)** | Optimized for memory-intensive workloads like large caches and database operations. |
| High Compute Speed | **F-Series (Fsv2)** | High clock speeds for CPU-bound transaction processing logic. |
| General Purpose | **D-Series (Dv5)** | Balanced CPU/Memory for API handlers and web-tier services. |
| Hardware Encryption | **DC-Series** | Confidential Computing with Intel SGX or AMD SEV-SNP for secure payment processing. |

---

## Section 7: Assessment Checklist

Before beginning any migration, conduct a technical discovery. The following checklist should be completed by the application owner.

### I. Application Logic & Portability

- [ ] Does the code perform direct binary reads/writes or pointer arithmetic on multi-byte integers?
- [ ] Are there compiler-specific pragmas or extensions (e.g., IBM XL C++) that do not exist in GCC or Clang?
- [ ] Does the application use native OS/400 IPC mechanisms (Message Queues, Data Queues)?
- [ ] List all third-party or IBM-native libraries used for encryption, networking, or hardware interfacing.

### II. Database & Stored Procedure Analysis

- [ ] Are stored procedures written in SQL PL, or do they call external programs (RPG/COBOL)?
- [ ] Are there GRAPHIC, VARGRAPHIC, or BLOB data types requiring specific mapping?
- [ ] What is the peak transaction rate (TPS) the database must sustain?
- [ ] Does the logic rely on specific DB2 isolation levels (e.g., Repeatable Read)?

### III. Integration & Connectivity

- [ ] Is the external database (e.g., Oracle) used solely by this application, or is it shared?
- [ ] How does the application interface with physical peripherals (Serial-over-IP, IoT Gateways)?
- [ ] How is user identity managed (Local OS/400 profiles, LDAP, Active Directory)?

---

## Section 8: The 5 Discovery Questions

At the start of any assessment meeting, ask these five questions to surface hidden risks early:

1.  **Latency Tolerance:** "For real-time transactions, what is the maximum round-trip latency the application can tolerate before it timeouts or degrades the end-user experience?"

2.  **Endianness Exposure:** "How heavily does the C++ codebase rely on raw memory manipulation, binary file I/O, or pointer arithmetic that assumes a Big-Endian architecture?"

3.  **Stored Procedure Weight:** "Of the logic residing in stored procedures, how much is purely data retrieval versus complex business logic that might be better suited for a middle-tier service?"

4.  **Dependency Mapping:** "What is the 'hard dependency' list for on-premises databases? Are they shared by other systems not included in this migration, or can they move as a single unit?"

5.  **Success Criteria:** "When this migration is complete, what defines success? Is it purely a datacenter exit, or are there specific performance, scalability, or AI-readiness goals?"

---

## Conclusion

Migrating legacy Big-Endian C++ workloads to Azure is a complex engineering challenge that requires a deep understanding of computer architecture. However, by choosing the right migration strategy—whether a rapid rehost or a strategic modernization—organizations can successfully retire aging datacenters.

By leveraging tools like **GitHub Copilot** to handle complex refactoring and adopting managed cloud databases, these legacy systems are not just preserved—they are transformed into modern platforms ready to leverage the full potential of cloud-scale AI and analytics.

---

## Additional Resources

-   [Azure Kubernetes Service (AKS) documentation](https://learn.microsoft.com/azure/aks/)
-   [Azure SQL Managed Instance documentation](https://learn.microsoft.com/azure/azure-sql/managed-instance/)
-   [SQL Server Migration Assistant for DB2](https://learn.microsoft.com/sql/ssma/db2/sql-server-migration-assistant-for-db2-db2tosql)
-   [Oracle Database@Azure](https://learn.microsoft.com/azure/oracle/oracle-db/)
-   [GitHub Copilot documentation](https://docs.github.com/copilot)
-   [Azure Modernization Center](https://learn.microsoft.com/azure/cloud-adoption-framework/modernize/)
-   [IBM Power Virtual Server](https://www.ibm.com/products/power-virtual-server)
-   [Azure ExpressRoute documentation](https://learn.microsoft.com/azure/expressroute/)
