#include "device_log.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <dlfcn.h>

// Test a system SO file via memfd + dlopen on AICPU
#define TEST_SO_PATH "/usr/lib64/libc.so.6"

extern "C" __attribute__((visibility("default"))) int StaticTileFwkBackendKernelServer(void *arg) {
    if (arg == nullptr) {
        DEV_ERROR("%s", "Invalid kernel arguments: null pointer");
        return -1;
    }
    return 0;
}

extern "C" __attribute__((visibility("default"))) int DynTileFwkBackendKernelServerInit(void *arg) {
    InitLogSwitch();
    if (arg == nullptr) {
        DEV_ERROR("%s", "Invalid kernel arguments: null pointer");
        return -1;
    }

    DEV_INFO("%s", "=== AICPU memfd + dlopen test ===");
    DEV_INFO("Target SO: %s", TEST_SO_PATH);

    // Step 1: Read SO file from disk
    DEV_INFO("%s", "Step 1: Reading SO file from disk...");
    FILE *fp = fopen(TEST_SO_PATH, "rb");
    if (fp == nullptr) {
        DEV_ERROR("Cannot open %s (errno=%d: %s)", TEST_SO_PATH, errno, strerror(errno));
        return -1;
    }

    // Get file size
    fseek(fp, 0, SEEK_END);
    long so_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (so_size <= 0) {
        DEV_ERROR("Invalid file size: %ld", so_size);
        fclose(fp);
        return -1;
    }

    DEV_INFO("SO file size: %ld bytes", so_size);

    // Allocate buffer and read file
    char *so_data = new char[so_size];
    size_t read_size = fread(so_data, 1, so_size, fp);
    fclose(fp);

    if (read_size != static_cast<size_t>(so_size)) {
        DEV_ERROR("Read incomplete: %zu/%ld bytes", read_size, so_size);
        delete[] so_data;
        return -1;
    }

    DEV_INFO("Successfully read %zu bytes from %s", read_size, TEST_SO_PATH);

    // Step 2: Create memfd
    DEV_INFO("%s", "Step 2: Creating memfd...");
    int fd = memfd_create("aicpu_dlopen_test", MFD_CLOEXEC);
    if (fd < 0) {
        DEV_ERROR("memfd_create failed (errno=%d: %s)", errno, strerror(errno));
        delete[] so_data;
        return -1;
    }
    DEV_INFO("memfd_create succeeded (fd=%d)", fd);

    // Step 3: Write SO data to memfd
    DEV_INFO("%s", "Step 3: Writing SO data to memfd...");
    ssize_t written = write(fd, so_data, so_size);
    delete[] so_data;  // Free buffer after writing

    if (written < 0) {
        DEV_ERROR("write failed (errno=%d: %s)", errno, strerror(errno));
        close(fd);
        return -1;
    }
    if (written != so_size) {
        DEV_ERROR("write incomplete: %zd/%ld bytes", written, so_size);
        close(fd);
        return -1;
    }
    DEV_INFO("write succeeded: %zd bytes", written);

    // Step 4: Get memfd path
    char memfd_path[256];
    snprintf(memfd_path, sizeof(memfd_path), "/proc/self/fd/%d", fd);
    DEV_INFO("memfd path: %s", memfd_path);

    // Step 5: dlopen from memfd
    DEV_INFO("%s", "Step 4: Attempting dlopen from memfd...");
    dlerror();  // Clear any existing error
    void *handle = dlopen(memfd_path, RTLD_LAZY | RTLD_LOCAL);
    const char *err = dlerror();

    if (handle == nullptr) {
        DEV_ERROR("dlopen FAILED: %s", err ? err : "unknown error");
        close(fd);
        return -1;
    }

    DEV_INFO("dlopen SUCCESS! Handle=%p", handle);

    // Step 6: Try to get a symbol
    DEV_INFO("%s", "Step 5: Testing symbol lookup...");
    dlerror();
    void *symbol = dlsym(handle, "printf");
    err = dlerror();

    if (err != nullptr || symbol == nullptr) {
        DEV_WARN("printf symbol not found: %s", err ? err : "unknown");
    } else {
        DEV_INFO("Symbol lookup SUCCESS! printf at %p", symbol);
    }

    // Clean up
    dlclose(handle);
    close(fd);

    DEV_INFO("%s", "=== Test completed: memfd + dlopen works on AICPU! ===");
    return 0;
}

extern "C" __attribute__((visibility("default"))) int DynTileFwkBackendKernelServer(void *arg) {
    if (arg == nullptr) {
        DEV_ERROR("%s", "Invalid kernel arguments: null pointer");
        return -1;
    }
    DEV_INFO("%s", "memfd_test_kernel: Running");
    return 0;
}
