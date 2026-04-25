#pragma once

#include <cstdio>
#include <cstdint>

// Minimal error codes
namespace DevCommonErr {
    constexpr uint32_t NULLPTR = 0x50001;
    constexpr uint32_t FILE_ERROR = 0x50002;
}

namespace ServerKernelErr {
    constexpr uint32_t KERNEL_EXEC_FAILED = 0x60001;
}

// Minimal logging macros
#define DEV_ERROR(errCode, fmt, ...) \
    printf("[ERROR] " fmt "\n", ##__VA_ARGS__)

#define DEV_WARN(fmt, ...) \
    printf("[WARN] " fmt "\n", ##__VA_ARGS__)

#define DEV_INFO(fmt, ...) \
    printf("[INFO] " fmt "\n", ##__VA_ARGS__)

#define DEV_DEBUG(fmt, ...) \
    printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)

// Stub for InitLogSwitch
static inline void InitLogSwitch() {}
