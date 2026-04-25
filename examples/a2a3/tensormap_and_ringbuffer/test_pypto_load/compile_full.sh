#!/bin/bash
# Complete compilation script for pypto_aicpu_interface.cpp

cd "$(dirname "$0")"

echo "Setting up dependencies for full version..."

# Restore original device_log.h with error codes
if [ -f src/machine/utils/device_log.h.bak ]; then
    yes | cp src/machine/utils/device_log.h.bak src/machine/utils/device_log.h
fi

# Create minimal stub headers for missing dependencies
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
// Stub for machine_ws_intf.h
namespace npu { namespace tile_fwk {} }
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

# Compile full version
g++ -Wall -Wextra -std=gnu++17 -O3 -shared -fPIC -D__DEVICE__ -fno-common -rdynamic \
  -I/usr/local/Ascend/cann-8.5.0/include \
  -I/usr/local/Ascend/cann-8.5.0/include/toolchain \
  -I/usr/local/Ascend/cann-8.5.0/pkg_inc/base \
  -I/usr/local/Ascend/cann-8.5.0/aarch64-linux/pkg_inc/op_common/log \
  -I./src \
  -I./src/tilefwk \
  -I./src/utils \
  -I./src/adapter \
  -o build/libtilefwk_backend_server_full.so \
  src/pypto_aicpu_interface.cpp \
  -ldl

if [ $? -eq 0 ]; then
    echo "✓ Compilation successful!"
    echo "Generated: build/libtilefwk_backend_server_full.so"
    ls -lh build/libtilefwk_backend_server_full.so
else
    echo "✗ Compilation failed. Check errors above."
    exit 1
fi
