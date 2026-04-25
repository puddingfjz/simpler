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
