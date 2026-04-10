/*
 * Systematic test to compare file-based vs memfd-based loading
 * and symbol lookup on AICPU.
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <dlfcn.h>

#define MEMFD_FD_PATH_LEN 256

// Test if a symbol can be found in a handle
static bool test_symbol(void *handle, const char *symbol, const char *so_path) {
    dlerror();
    void *sym = dlsym(handle, symbol);
    const char *err = dlerror();

    printf("[TEST] dlsym(\"%s\") from %s: ", symbol, so_path);
    if (sym != nullptr) {
        printf("FOUND at %p\n", sym);
        return true;
    } else {
        printf("FAILED (%s)\n", err ? err : "unknown");
        return false;
    }
}

// Test loading SO from file path
static bool test_file_loading(const char *so_path) {
    printf("\n=== Test 1: File-based loading ===\n");
    printf("Loading: %s\n", so_path);

    dlerror();
    void *handle = dlopen(so_path, RTLD_LAZY | RTLD_GLOBAL);
    const char *err = dlerror();

    if (handle == nullptr) {
        printf("[TEST] dlopen FAILED: %s\n", err ? err : "unknown");
        return false;
    }

    printf("[TEST] dlopen succeeded: handle=%p\n", handle);

    // Test symbols
    bool result = true;
    result &= test_symbol(handle, "aicpu_orchestration_entry", so_path);
    result &= test_symbol(handle, "aicpu_orchestration_config", so_path);
    result &= test_symbol(handle, "pto2_framework_bind_runtime", so_path);

    dlclose(handle);
    return result;
}

// Test loading SO from memfd
static bool test_memfd_loading(const char *so_path) {
    printf("\n=== Test 2: Memfd-based loading ===\n");
    printf("Loading from memfd: %s\n", so_path);

    // Read SO file
    FILE *fp = fopen(so_path, "rb");
    if (fp == nullptr) {
        printf("[TEST] fopen FAILED: %s\n", so_path);
        return false;
    }

    fseek(fp, 0, SEEK_END);
    long so_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    printf("[TEST] SO size: %ld bytes\n", so_size);

    char *so_data = new char[so_size];
    size_t read_size = fread(so_data, 1, so_size, fp);
    fclose(fp);

    if (read_size != static_cast<size_t>(so_size)) {
        printf("[TEST] fread FAILED\n");
        delete[] so_data;
        return false;
    }

    // Create memfd
    int fd = memfd_create("symbol_test_so", MFD_CLOEXEC);
    if (fd < 0) {
        printf("[TEST] memfd_create FAILED: errno=%d\n", errno);
        delete[] so_data;
        return false;
    }

    // Write SO data to memfd
    ssize_t written = write(fd, so_data, so_size);
    if (written != so_size) {
        printf("[TEST] write FAILED: written=%zd, expected=%ld\n", written, so_size);
        close(fd);
        delete[] so_data;
        return false;
    }

    delete[] so_data;

    // Construct /proc/self/fd/N path
    char memfd_path[MEMFD_FD_PATH_LEN];
    snprintf(memfd_path, MEMFD_FD_PATH_LEN, "/proc/self/fd/%d", fd);

    printf("[TEST] memfd path: %s\n", memfd_path);

    // dlopen from memfd
    dlerror();
    void *handle = dlopen(memfd_path, RTLD_LAZY | RTLD_GLOBAL);
    const char *err = dlerror();

    if (handle == nullptr) {
        printf("[TEST] dlopen from memfd FAILED: %s\n", err ? err : "unknown");
        close(fd);
        return false;
    }

    printf("[TEST] dlopen from memfd succeeded: handle=%p\n", handle);

    // Test symbols
    bool result = true;
    result &= test_symbol(handle, "aicpu_orchestration_entry", memfd_path);
    result &= test_symbol(handle, "aicpu_orchestration_config", memfd_path);
    result &= test_symbol(handle, "pto2_framework_bind_runtime", memfd_path);

    dlclose(handle);
    close(fd);
    return result;
}

extern "C" int DynTileFwkBackendKernelServerInit() {
    printf("\n");
    printf("========================================\n");
    printf("  AICPU Symbol Lookup Test\n");
    printf("========================================\n");

    // Test with libc.so.6 (system library)
    const char *test_libc = "/usr/lib64/libc.so.6";
    printf("\n--- Testing with libc.so.6 ---\n");
    bool libc_file_ok = test_file_loading(test_libc);
    bool libc_memfd_ok = test_memfd_loading(test_libc);
    printf("libc.so.6: file=%s, memfd=%s\n",
           libc_file_ok ? "OK" : "FAIL",
           libc_memfd_ok ? "OK" : "FAIL");

    // Test with orchestration SO (if path provided)
    const char *orch_so = getenv("TEST_ORCH_SO_PATH");
    if (orch_so != nullptr) {
        printf("\n--- Testing with orchestration SO ---\n");
        bool orch_file_ok = test_file_loading(orch_so);
        bool orch_memfd_ok = test_memfd_loading(orch_so);
        printf("Orchestration SO: file=%s, memfd=%s\n",
               orch_file_ok ? "OK" : "FAIL",
               orch_memfd_ok ? "OK" : "FAIL");
    } else {
        printf("\n(Skipping orchestration SO test - set TEST_ORCH_SO_PATH to test)\n");
    }

    printf("\n========================================\n");
    printf("  Test Complete\n");
    printf("========================================\n");

    return 0;
}
