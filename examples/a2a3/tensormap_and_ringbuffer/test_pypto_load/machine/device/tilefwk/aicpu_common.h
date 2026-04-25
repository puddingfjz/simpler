#pragma once

#include <cstdint>
#include <cstdlib>

namespace npu {
namespace tile_fwk {

// Minimal DeviceArgs structure
struct DeviceArgs {
    uint64_t aicpuSoBin;
    uint32_t aicpuSoLen;
    uint8_t deviceId;
    uint32_t reserved;
    uint64_t sharedBuffer;
    uint64_t coreRegAddr;
    uint32_t coreRegLen;
    uint64_t corePmuRegAddr;
    uint32_t corePmuRegLen;
    uint64_t devDfxArgAddr;
    uint32_t devDfxArgLen;
};

// Minimal DeviceKernelArgs structure
struct DeviceKernelArgs {
    void* cfgdata;
    uint64_t reserved[8];
};

} // namespace tile_fwk
} // namespace npu
