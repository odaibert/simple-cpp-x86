cmake_minimum_required(VERSION 3.16)
project(simple-cpp-x86 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Legacy OS/400 example — demonstrates the endianness bug on x86
add_executable(pos_legacy src/pos_transaction.cpp)
set_target_properties(pos_legacy PROPERTIES CXX_STANDARD 17)

# Modernized x86 example — correct output on all platforms
add_executable(pos_modern src/pos_transaction_x86.cpp)
