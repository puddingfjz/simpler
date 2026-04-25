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

#define DEV_ATRACE(fmt, ...) // Stub

// Stub for InitLogSwitch
static inline void InitLogSwitch() {}

// Stub for backtrace
static inline void PrintBacktrace(int, const char*, int = 0) {}

// Stubs for other required symbols
namespace npu {
namespace tile_fwk {
constexpr bool IsDeviceMode() { return true; }
constexpr bool IsLogEnableInfo() { return true; }
constexpr bool IsLogEnableDebug() { return true; }
constexpr bool IsLogEnableWarn() { return true; }
constexpr bool IsLogEnableError() { return true; }
}
}

#define HardBranchTrue(x) (true)
#define HardBranchGroupDefine(name)
#define unlikely(x) (x)
#define DEV_IF_PREPROCESS() if constexpr (false)
#define DEV_AT_RANGECHECK_IF_ERROR_R(cnt, err)
#define DEV_CHECK_VERSION_R(exp_ver, cur_ver, err)
#define DEV_STATIC_ASSERT(...)
namespace ThreadErr { constexpr int ANY = 0; }
