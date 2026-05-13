cmake_minimum_required(VERSION 3.10)
project(bumblebot)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CheckIPOSupported)
check_ipo_supported(RESULT BUMBLEBOT_LTO_OK)

add_executable(bumblebot
    src/bumblebot/main.cpp
    src/bumblebot/bitboard.cpp
    src/bumblebot/movegen.cpp
    src/bumblebot/position.cpp
    src/bumblebot/magics.cpp
    src/bumblebot/perft.cpp
    src/bumblebot/uci.cpp
    src/bumblebot/zobrist.cpp
)

target_include_directories(bumblebot PRIVATE
    src/bumblebot
)

if(BUMBLEBOT_LTO_OK)
    set_property(TARGET bumblebot PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    set_property(TARGET bumblebot PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
endif()
