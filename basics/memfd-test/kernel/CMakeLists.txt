cmake_minimum_required(VERSION 3.16.3)

project(memfd_test_kernel LANGUAGES C CXX)

# Set ASCEND_CANN_PACKAGE_PATH if not already set
if(NOT DEFINED ASCEND_CANN_PACKAGE_PATH)
    if(DEFINED ENV{ASCEND_HOME_PATH})
        set(ASCEND_CANN_PACKAGE_PATH $ENV{ASCEND_HOME_PATH})
    else()
        message(FATAL_ERROR "ASCEND_CANN_PACKAGE_PATH or ASCEND_HOME_PATH must be set")
    endif()
endif()

set(CMAKE_C_COMPILER "${ASCEND_CANN_PACKAGE_PATH}/toolkit/toolchain/hcc/bin/aarch64-target-linux-gnu-gcc")
set(CMAKE_CXX_COMPILER "${ASCEND_CANN_PACKAGE_PATH}/toolkit/toolchain/hcc/bin/aarch64-target-linux-gnu-g++")

add_library(memfd_test_kernel SHARED)
target_sources(memfd_test_kernel PRIVATE memfd_test_kernel.cpp device_log.cpp)

target_compile_options(memfd_test_kernel
        PRIVATE
            -Wall
            -Wextra
            -std=gnu++17
            -rdynamic
            -O3
            -shared
            -fPIC
            -Werror
            -Wno-error=variadic-macros
            -fno-common
            -g
)
target_include_directories(memfd_test_kernel
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE
            ${ASCEND_CANN_PACKAGE_PATH}/include
            ${ASCEND_CANN_PACKAGE_PATH}/include/toolchain
            ${ASCEND_CANN_PACKAGE_PATH}/aarch64-linux/pkg_inc/base
)
target_link_directories(memfd_test_kernel
        PRIVATE
            ${ASCEND_CANN_PACKAGE_PATH}/${CMAKE_SYSTEM_PROCESSOR}-linux/devlib/linux/aarch64/
            ${ASCEND_CANN_PACKAGE_PATH}/${CMAKE_SYSTEM_PROCESSOR}-linux/devlib/device/
            ${ASCEND_CANN_PACKAGE_PATH}/lib64
)
set_target_properties(memfd_test_kernel PROPERTIES OUTPUT_NAME tilefwk_backend_server)
