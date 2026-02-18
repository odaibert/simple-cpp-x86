cmake_minimum_required(VERSION 3.16)
project(simple-cpp-x86 LANGUAGES CXX)

# Default to C++20; C++23 enables std::byteswap
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Legacy Big-Endian example (demonstrates the bug on x86) ---
add_executable(legacy_bigendian src/legacy_bigendian.cpp)

# --- Refactored x86 example (correct output on all platforms) ---
add_executable(refactored_x86 src/refactored_x86.cpp)

# Try C++23 for refactored target (enables std::byteswap)
set_target_properties(refactored_x86 PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED OFF   # Gracefully fall back to C++20
)
