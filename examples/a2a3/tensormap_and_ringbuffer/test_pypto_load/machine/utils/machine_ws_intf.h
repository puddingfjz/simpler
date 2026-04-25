#pragma once

// Minimal machine_ws_intf.h for pypto_aicpu_interface compilation
// Most of the original definitions are not needed by aicpu_interface

#include <cstdint>
#include <cstdlib>

namespace npu {
namespace tile_fwk {

// Minimal enums - just what might be needed
enum class MachineStatus { START = 0, FINISH = 1, STOP = 2 };

} // namespace tile_fwk
} // namespace npu
