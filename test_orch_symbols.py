#!/usr/bin/env python3
import sys
sys.path.insert(0, 'python')
sys.path.insert(0, '.')

import os
import tempfile
os.environ['ASCEND_HOME_PATH'] = '/usr/local/Ascend/cann-8.5.0'

from kernel_compiler import KernelCompiler

kc = KernelCompiler()

build_dir = tempfile.mkdtemp(prefix='orch_build_')
print(f'Build dir: {build_dir}')

orch_so = kc.compile_orchestration(
    runtime_name='tensormap_and_ringbuffer',
    source_path='examples/a2a3/tensormap_and_ringbuffer/vector_example/kernels/orchestration/example_orchestration.cpp',
    build_dir=build_dir
)

print(f'SO size from compile: {len(orch_so)} bytes')

for f in os.listdir(build_dir):
    if f.endswith('.so'):
        so_path = os.path.join(build_dir, f)
        print(f'SO file: {so_path}')
        print(f'SO size on disk: {os.path.getsize(so_path)} bytes')
        print(f'Checking symbols...')
        os.system(f'/usr/local/Ascend/cann-8.5.0/tools/hcc/bin/aarch64-target-linux-gnu-readelf -W -s {so_path} 2>&1 | grep -E "aicpu_orchestration|pto2_framework_bind"')

print(f'\nBuild dir: {build_dir}')
