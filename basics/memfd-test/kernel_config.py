"""
Kernel config for symbol lookup test
"""

KERNELS = []

ORCHESTRATION = {
    'source': 'basics/memfd-test/dlsym_test.cpp',
    'function_name': 'DynTileFwkBackendKernelServerInit',
}
