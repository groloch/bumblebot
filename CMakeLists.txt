cmake_minimum_required(VERSION 3.10)
project(bumblebot)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CheckIPOSupported)
check_ipo_supported(RESULT BUMBLEBOT_LTO_OK)

set(ORT_ROOT "$ENV{HOME}/Documents/onnxruntime")

add_executable(bumblebot
    src/bumblebot/main.cpp
    src/bumblebot/bitboard.cpp
    src/bumblebot/movegen.cpp
    src/bumblebot/position.cpp
    src/bumblebot/magics.cpp
    src/bumblebot/perft.cpp
    src/bumblebot/bench.cpp
    src/bumblebot/uci.cpp
    src/bumblebot/zobrist.cpp
    src/bumblebot/nn/evaluator.cpp
    src/bumblebot/nn/nn_utils.cpp
    src/bumblebot/search/node.cpp
    src/bumblebot/search/search.cpp
)

target_include_directories(bumblebot PRIVATE
    src/bumblebot
    ${ORT_ROOT}/include/onnxruntime/core/session
)

set(EVALFILE "model.onnx" CACHE STRING "Path to the model")
target_compile_definitions(bumblebot PRIVATE EVALFILE="${EVALFILE}")

target_link_directories(bumblebot PRIVATE ${ORT_ROOT}/build/Linux/Release)
target_link_libraries(bumblebot PRIVATE onnxruntime)
set_target_properties(bumblebot PROPERTIES BUILD_RPATH ${ORT_ROOT}/build/Linux/Release)

if(BUMBLEBOT_LTO_OK)
    set_property(TARGET bumblebot PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    set_property(TARGET bumblebot PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
endif()
