/*
 * Minimal test to check dlsym behavior on AICPU
 */

#include <cstdio>
#include <cstdlib>
#include <dlfcn.h>

// Test 1: Can we find a simple function pointer?
extern "C" void simple_test_function() {
    printf("simple_test_function called\n");
}

// Export a config symbol
__attribute__((visibility("default"))) int simple_config = 42;

extern "C" int DynTileFwkBackendKernelServerInit() {
    printf("=== Simple AICPU Test ===\n");

    // Test dlsym on RTLD_DEFAULT (should find symbols in main executable)
    dlerror();
    void *sym1 = dlsym(RTLD_DEFAULT, "simple_test_function");
    const char *err1 = dlerror();
    printf("dlsym(RTLD_DEFAULT, \"simple_test_function\"): %p (%s)\n",
           sym1, err1 ? err1 : "success");

    dlerror();
    void *sym2 = dlsym(RTLD_DEFAULT, "simple_config");
    const char *err2 = dlerror();
    printf("dlsym(RTLD_DEFAULT, \"simple_config\"): %p (%s)\n",
           sym2, err2 ? err2 : "success");

    // Call the function if found
    if (sym1 != nullptr) {
        auto *func = reinterpret_cast<void(*)()>(sym1);
        printf("Calling simple_test_function...\n");
        func();
    }

    printf("=== Test Complete ===\n");
    return 0;
}
