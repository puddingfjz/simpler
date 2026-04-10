#!/usr/bin/env python3
"""
Quick test to check if the issue is with memfd loading or symbol resolution.
This compiles an orchestration SO and checks if the symbols are present.
"""
import sys
sys.path.insert(0, 'python')
sys.path.insert(0, '.')

import os
import tempfile
os.environ['ASCEND_HOME_PATH'] = '/usr/local/Ascend/cann-8.5.0'

from kernel_compiler import KernelCompiler

print("=== Testing orchestration SO compilation and symbol check ===")

kc = KernelCompiler()

build_dir = tempfile.mkdtemp(prefix='quick_test_')
print(f'Build dir: {build_dir}')

orch_so = kc.compile_orchestration(
    runtime_name='tensormap_and_ringbuffer',
    source_path='examples/a2a3/tensormap_and_ringbuffer/vector_example/kernels/orchestration/example_orchestration.cpp',
    build_dir=build_dir
)

print(f'SO size: {len(orch_so)} bytes')

# Check if the symbols are in the SO bytes
symbols_to_check = [
    b'aicpu_orchestration_entry',
    b'aicpu_orchestration_config',
    b'pto2_framework_bind_runtime'
]

for symbol in symbols_to_check:
    if symbol in orch_so:
        print(f'✓ Found {symbol.decode()} in SO bytes')
    else:
        print(f'✗ NOT FOUND: {symbol.decode()} in SO bytes')

# Find the SO file for further inspection
so_files = [f for f in os.listdir(build_dir) if f.endswith('.so')]
if so_files:
    so_path = os.path.join(build_dir, so_files[0])
    print(f'\nSO file: {so_path}')

    # Check with readelf
    result = os.popen(f'/usr/local/Ascend/cann-8.5.0/tools/hcc/bin/aarch64-target-linux-gnu-readelf -s {so_path} 2>&1 | grep -E "aicpu_orchestration_entry|aicpu_orchestration_config|pto2_framework_bind_runtime"').read()
    print('readelf output:')
    print(result)

print(f'\nConclusion: SO compilation {"succeeded" if all(s in orch_so for s in symbols_to_check) else "FAILED"}')
print(f'Build dir: {build_dir}')
