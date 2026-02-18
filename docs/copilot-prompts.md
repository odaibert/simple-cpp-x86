# GitHub Copilot Prompts for Big-Endian to Little-Endian Migration

Use these prompts in **GitHub Copilot Chat** (VS Code or Visual Studio) to
accelerate refactoring of legacy C++ code from IBM Power Systems (Big-Endian)
to Azure x86/x64 (Little-Endian).

---

## Prompt 1: Generic Endianness Refactoring

> Best used when you have a specific file or function selected in the editor.

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

---

## Prompt 2: Explain Legacy Code

> Use `/explain` or this prompt to have Copilot analyze a legacy function before refactoring.

```
Explain this C++ function. Specifically:
1. Does it assume a particular byte order (endianness)?
2. Are there any direct memory copies (memcpy, memmove) into structs with
   multi-byte integer fields?
3. Will this code produce correct results on an x86 Little-Endian system
   if the source data was written on a Big-Endian IBM Power system?
```

---

## Prompt 3: Full File Refactoring

> Use when migrating an entire source file.

```
Refactor this entire C++ source file for cross-platform portability.

Context:
- The original code was compiled and run on IBM Power Systems (Big-Endian, OS/400).
- The target environment is Azure x86/x64 Linux containers (Little-Endian).
- Binary data files and network packets originating from the legacy system
  store multi-byte integers in Big-Endian format.

Requirements:
1. Add a portable byte-swap utility (prefer C++20 std::endian / C++23
   std::byteswap, with fallbacks for older compilers).
2. After every raw memcpy into a struct, convert each multi-byte integer
   field from Big-Endian to host byte order.
3. Do NOT modify character arrays or single-byte fields.
4. Add clear comments explaining each conversion for maintainability.
5. Preserve the original function signatures and struct layouts.
```

---

## Prompt 4: EBCDIC to UTF-8 Conversion

> For handling text encoding differences between IBM i and Azure.

```
Write a C++ utility function that converts an EBCDIC-encoded character buffer
to UTF-8. The function should:
1. Accept a const char* input buffer and a size_t length.
2. Return a std::string containing the UTF-8 equivalent.
3. Handle common EBCDIC code page 037 (US/Canada) characters.
4. Be safe for use in a high-throughput POS transaction pipeline.
```

---

## Tips for Effective Copilot Usage

1. **Select code first:** Highlight the specific function or struct before
   opening Copilot Chat. This gives it precise context.

2. **Use `/explain` before `/fix`:** Understanding the legacy logic first
   prevents introducing regressions.

3. **Iterate:** If the first suggestion uses `ntohl()` but you prefer
   `std::byteswap`, ask Copilot to "use C++23 std::byteswap instead."

4. **Validate with tests:** After refactoring, compile and run both the
   legacy (Big-Endian simulation) and refactored versions to confirm the
   output matches.
