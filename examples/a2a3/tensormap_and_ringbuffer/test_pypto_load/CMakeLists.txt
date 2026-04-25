cmake_minimum_required(VERSION 3.16.3)

project(test_pypto_load LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set ASCEND paths
set(ASCEND_HOME_PATH $ENV{ASCEND_HOME_PATH})
if(NOT ASCEND_HOME_PATH)
    set(ASCEND_HOME_PATH "/usr/local/Ascend/cann-8.5.0")
endif()

message(STATUS "ASCEND_HOME_PATH: ${ASCEND_HOME_PATH}")

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/adapter
    ${ASCEND_HOME_PATH}/include
    ${ASCEND_HOME_PATH}/include/toolchain
    ${ASCEND_HOME_PATH}/pkg_inc/runtime
)

# Link directories
link_directories(
    ${ASCEND_HOME_PATH}/lib64
    ${ASCEND_HOME_PATH}/aarch64-linux/lib64
)

# Find nlohmann/json
find_path(JSON_INCLUDE_DIR nlohmann/json.hpp
    PATHS
        /usr/include
        /usr/local/include
)
if(JSON_INCLUDE_DIR)
    include_directories(${JSON_INCLUDE_DIR})
    message(STATUS "Found nlohmann/json at: ${JSON_INCLUDE_DIR}")
else()
    message(FATAL_ERROR "nlohmann/json not found")
endif()

# Build load_aicpu_op library
add_library(load_aicpu_op STATIC
    src/load_aicpu_op.cpp
)

target_compile_options(load_aicpu_op PRIVATE
    -Wall
    -Wextra
    -fPIC
    -std=c++17
)

# Build test executable
add_executable(test_load
    test_load.cpp
)

target_link_libraries(test_load
    load_aicpu_op
    runtime
    ascendcl
    dl
    pthread
)

target_compile_options(test_load PRIVATE
    -Wall
    -Wextra
    -g
    -std=c++17
)
