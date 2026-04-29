cmake_minimum_required(VERSION 3.10)
project(bumblebot)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(bumblebot 
    src/bumblebot/main.cpp
    src/bumblebot/bitboard.cpp
    src/bumblebot/move.cpp
    src/bumblebot/position.cpp
    src/bumblebot/magics.cpp
    src/bumblebot/move.cpp
    src/bumblebot/perft.cpp
    src/bumblebot/uci.cpp
)

target_include_directories(bumblebot PRIVATE src/bumblebot)
