#!/bin/bash
# Complete compilation script for pypto_aicpu_interface.cpp (v2)

cd "$(dirname "$0")"

echo "Compiling pypto_aicpu_interface.cpp with minimal dependencies..."

# Use minimal device_log.h
cat > src/machine/utils/device_log.h << 'EOF'
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
EOF

# Create stub headers for missing dependencies
mkdir -p src/machine/device/dynamic
cat > src/machine/device/dynamic/device_trace.h << 'EOF'
#pragma once
// Stub for device_trace.h
EOF

cat > src/machine/device/dynamic/aicpu_instrumentation.h << 'EOF'
#pragma once
// Stub for aicpu_instrumentation.h
EOF

cat > src/machine/utils/device_switch.h << 'EOF'
#pragma once
#include <cstdint>
constexpr bool IsDeviceMode() { return true; }
EOF

cat > src/machine/utils/machine_ws_intf.h << 'EOF'
#pragma once
#include <cstdint>

namespace npu {
namespace tile_fwk {
// Forward declarations
struct DeviceKernelArgsParameter {
    uint64_t argsAddr;
    uint32_t argsLen;
    uint32_t argsValid;
};

struct DeviceKernelArgs {
    void* cfgdata;
    uint64_t reserved[8];
};
}
}
EOF

# Create symlinks for tilefwk headers
mkdir -p src/machine/device/tilefwk
if [ ! -f src/machine/device/tilefwk/aicpu_common.h ]; then
    ln -sf /data/fangjingzhi/simpler_356_2/examples/a2a3/tensormap_and_ringbuffer/test_pypto_load/src/tilefwk/aicpu_common.h src/machine/device/tilefwk/aicpu_common.h
fi

mkdir -p src/tilefwk
if [ ! -f src/tilefwk/error_code.h ]; then
    ln -sf /usr/local/Ascend/cann-8.5.0/aarch64-linux/pkg_inc/op_common/log/error_code.h src/tilefwk/error_code.h
fi

echo "Dependencies setup complete. Compiling..."

# Compile full version (including both pypto_aicpu_interface.cpp and device_sche_minimal.cpp)
g++ -Wall -Wextra -std=gnu++17 -O3 -shared -fPIC -D__DEVICE__ -fno-common -rdynamic \
  -I/usr/local/Ascend/cann-8.5.0/include \
  -I/usr/local/Ascend/cann-8.5.0/include/toolchain \
  -I/usr/local/Ascend/cann-8.5.0/pkg_inc/base \
  -I/usr/local/Ascend/cann-8.5.0/aarch64-linux/pkg_inc/op_common/log \
  -I./src \
  -I./src/tilefwk \
  -I./src/utils \
  -I./src/adapter \
  -o build/libtilefwk_backend_server.so \
  src/pypto_aicpu_interface.cpp \
  device_sche_minimal.cpp \
  -ldl

if [ $? -eq 0 ]; then
    echo "✓ Compilation successful!"
    echo "Generated: build/libtilefwk_backend_server.so"
    ls -lh build/libtilefwk_backend_server.so

    echo ""
    echo "Exported symbols:"
    nm -D build/libtilefwk_backend_server.so | grep -E "DynPypto|Static"
else
    echo "✗ Compilation failed. Check errors above."
    exit 1
fi
