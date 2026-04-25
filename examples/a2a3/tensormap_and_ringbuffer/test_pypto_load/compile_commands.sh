#!/bin/bash
# Compilation commands for pypto_aicpu_interface

cd "$(dirname "$0")"

# Method 1: Simplified version (recommended)
# ==========================================
compile_simplified() {
    echo "Compiling simplified version..."
    g++ -Wall -Wextra -std=gnu++17 -O3 -shared -fPIC -D__DEVICE__ -fno-common -rdynamic \
      -o build/libtilefwk_backend_server_mod.so \
      src/pypto_aicpu_interface_mod.cpp \
      -ldl
    echo "Generated: build/libtilefwk_backend_server_mod.so"
}

# Method 2: Full version (requires dependency setup)
# ================================================
setup_dependencies() {
    echo "Setting up dependencies..."

    # Create missing header files
    mkdir -p src/machine/device/dynamic
    touch src/machine/device/dynamic/device_trace.h
    touch src/machine/device/dynamic/aicpu_instrumentation.h

    touch src/machine/utils/device_switch.h
    touch src/machine/utils/machine_ws_intf.h

    # Create tilefwk error_code.h link
    mkdir -p src/tilefwk
    if [ ! -f src/tilefwk/error_code.h ]; then
        ln -sf /usr/local/Ascend/cann-8.5.0/aarch64-linux/pkg_inc/op_common/log/error_code.h src/tilefwk/error_code.h
    fi

    # Create machine/device/tilefwk structure
    mkdir -p src/machine/device/tilefwk
    if [ ! -f src/machine/device/tilefwk/aicpu_common.h ]; then
        ln -sf /data/fangjingzhi/simpler_356_2/examples/a2a3/tensormap_and_ringbuffer/test_pypto_load/src/tilefwk/aicpu_common.h src/machine/device/tilefwk/aicpu_common.h
    fi

    echo "Dependencies setup complete"
}

compile_full() {
    echo "Compiling full version..."
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
      -ldl
    echo "Generated: build/libtilefwk_backend_server.so"
}

# Main
# =====
case "${1:-simplified}" in
    simplified)
        compile_simplified
        ;;
    full)
        setup_dependencies
        compile_full
        ;;
    setup)
        setup_dependencies
        ;;
    *)
        echo "Usage: $0 [simplified|full|setup]"
        echo ""
        echo "  simplified - Compile simplified version (default, recommended)"
        echo "  full       - Compile full version (requires dependency setup)"
        echo "  setup      - Setup dependencies for full version only"
        exit 1
        ;;
esac
