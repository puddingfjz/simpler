#!/usr/bin/env python3
import sys
sys.path.insert(0, 'python')
sys.path.insert(0, '.')

import os
import tempfile
os.environ['ASCEND_HOME_PATH'] = '/usr/local/Ascend/cann-8.5.0'

from kernel_compiler import KernelCompiler

kc = KernelCompiler()

build_dir = tempfile.mkdtemp(prefix='orch_compare_')
print(f'Build dir: {build_dir}')

# Compile the SO
orch_so = kc.compile_orchestration(
    runtime_name='tensormap_and_ringbuffer',
    source_path='examples/a2a3/tensormap_and_ringbuffer/vector_example/kernels/orchestration/example_orchestration.cpp',
    build_dir=build_dir
)

print(f'SO size from compile: {len(orch_so)} bytes')

# Check SO header (ELF magic number)
print(f'SO header: {orch_so[:4].hex()}')
if orch_so[:4] == b'\x7fELF':
    print('Valid ELF header')
else:
    print('INVALID ELF header!')

# Find the SO file
for f in os.listdir(build_dir):
    if f.endswith('.so'):
        so_path = os.path.join(build_dir, f)
        print(f'SO file: {so_path}')
        print(f'SO file size on disk: {os.path.getsize(so_path)} bytes')

        # Compare bytes
        with open(so_path, 'rb') as f:
            disk_bytes = f.read()

        if disk_bytes == orch_so:
            print('Bytes match: SO bytes == disk file bytes')
        else:
            print(f'Bytes differ: SO has {len(orch_so)} bytes, disk has {len(disk_bytes)} bytes')

        # Check for symbol strings in the SO bytes
        if b'aicpu_orchestration_entry' in orch_so:
            print('Found "aicpu_orchestration_entry" string in SO bytes')
        else:
            print('NOT FOUND: "aicpu_orchestration_entry" string in SO bytes')

        if b'aicpu_orchestration_config' in orch_so:
            print('Found "aicpu_orchestration_config" string in SO bytes')
        else:
            print('NOT FOUND: "aicpu_orchestration_config" string in SO bytes')

        if b'pto2_framework_bind_runtime' in orch_so:
            print('Found "pto2_framework_bind_runtime" string in SO bytes')
        else:
            print('NOT FOUND: "pto2_framework_bind_runtime" string in SO bytes')

print(f'\nBuild dir: {build_dir}')
