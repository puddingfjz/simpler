cmake_minimum_required(VERSION 3.15)
project(aicpu_kernel_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -Wpedantic)

# compile kernel
add_subdirectory(kernel)

# compile host kernel launcher
add_executable(launcher launcher.cpp)

target_include_directories(launcher PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link CANN runtime (must be available)
if(DEFINED ENV{ASCEND_HOME_PATH})
    set(ASCEND_HOME_PATH $ENV{ASCEND_HOME_PATH})
    target_include_directories(launcher PRIVATE
        ${ASCEND_HOME_PATH}/include
        ${ASCEND_HOME_PATH}/include/experiment
        ${ASCEND_HOME_PATH}/include/experiment/runtime
        ${ASCEND_HOME_PATH}/include/experiment/msprof
        ${ASCEND_HOME_PATH}/aarch64-linux/pkg_inc/runtime
    )
    target_link_directories(launcher PRIVATE
        ${ASCEND_HOME_PATH}/lib64
        ${ASCEND_HOME_PATH}/runtime/lib64
    )
else()
    message(FATAL_ERROR "ASCEND_HOME_PATH is not set, CANN runtime is required")
endif()

target_link_libraries(launcher PRIVATE runtime)
