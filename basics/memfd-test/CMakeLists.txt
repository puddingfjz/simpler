cmake_minimum_required(VERSION 3.15)
project(aicpu_memfd_test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lrt")

add_compile_options(-Wall -Wextra -Wpedantic)

# compile kernel
add_subdirectory(kernel)

# compile host launcher
add_executable(memfd_test_launcher launcher.cpp)

target_include_directories(memfd_test_launcher PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link CANN runtime
if(DEFINED ENV{ASCEND_HOME_PATH})
    set(ASCEND_HOME_PATH $ENV{ASCEND_HOME_PATH})
    target_include_directories(memfd_test_launcher PRIVATE
        ${ASCEND_HOME_PATH}/include
        ${ASCEND_HOME_PATH}/include/experiment
        ${ASCEND_HOME_PATH}/include/experiment/runtime
        ${ASCEND_HOME_PATH}/include/experiment/msprof
        ${ASCEND_HOME_PATH}/aarch64-linux/pkg_inc/runtime
    )
    target_link_directories(memfd_test_launcher PRIVATE
        ${ASCEND_HOME_PATH}/lib64
        ${ASCEND_HOME_PATH}/runtime/lib64
    )
else()
    message(FATAL_ERROR "ASCEND_HOME_PATH is not set")
endif()

target_link_libraries(memfd_test_launcher PRIVATE runtime)
