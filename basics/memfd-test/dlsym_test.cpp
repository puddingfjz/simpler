/*
 * Test dlsym behavior on AICPU with different SO loading methods
 */

#include <cstdio>
#include <cstring>
#include <cerrno>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

extern "C" int DynTileFwkBackendKernelServerInit() {
    printf("=== AICPU dlsym Test ===\n");

    // Test 1: Load libc.so.6 from filesystem
    printf("\n--- Test 1: Load libc.so.6 from filesystem ---\n");
    void *handle1 = dlopen("/usr/lib64/libc.so.6", RTLD_LAZY | RTLD_GLOBAL);
    printf("dlopen libc.so.6: handle=%p\n", handle1);

    if (handle1 != nullptr) {
        dlerror();
        void *sym_printf = dlsym(handle1, "printf");
        const char *err = dlerror();
        printf("dlsym(handle, \"printf\"): %p (%s)\n", sym_printf, err ? err : "success");

        dlerror();
        void *sym_memcpy = dlsym(handle1, "memcpy");
        err = dlerror();
        printf("dlsym(handle, \"memcpy\"): %p (%s)\n", sym_memcpy, err ? err : "success");

        dlclose(handle1);
    }

    // Test 2: Load libc.so.6 via memfd
    printf("\n--- Test 2: Load libc.so.6 via memfd ---\n");

    // Read libc.so.6
    FILE *fp = fopen("/usr/lib64/libc.so.6", "rb");
    if (fp == nullptr) {
        printf("Failed to open libc.so.6\n");
        return 0;
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    printf("libc.so.6 size: %ld bytes\n", size);

    char *data = new char[size];
    size_t read_size = fread(data, 1, size, fp);
    fclose(fp);

    if (read_size != static_cast<size_t>(size)) {
        printf("Failed to read libc.so.6\n");
        delete[] data;
        return 0;
    }

    // Create memfd
    int fd = memfd_create("test_libc", MFD_CLOEXEC);
    if (fd < 0) {
        printf("memfd_create failed: errno=%d\n", errno);
        delete[] data;
        return 0;
    }

    // Write to memfd
    ssize_t written = write(fd, data, size);
    delete[] data;

    if (written != size) {
        printf("write to memfd failed: written=%zd, expected=%ld\n", written, size);
        close(fd);
        return 0;
    }

    char path[256];
    snprintf(path, sizeof(path), "/proc/self/fd/%d", fd);
    printf("memfd path: %s\n", path);

    // dlopen from memfd
    void *handle2 = dlopen(path, RTLD_LAZY | RTLD_GLOBAL);
    printf("dlopen from memfd: handle=%p\n", handle2);

    if (handle2 != nullptr) {
        dlerror();
        void *sym_printf = dlsym(handle2, "printf");
        const char *err = dlerror();
        printf("dlsym(handle, \"printf\"): %p (%s)\n", sym_printf, err ? err : "success");

        dlerror();
        void *sym_memcpy = dlsym(handle2, "memcpy");
        err = dlerror();
        printf("dlsym(handle, \"memcpy\"): %p (%s)\n", sym_memcpy, err ? err : "success");

        // Try RTLD_DEFAULT
        printf("\n--- Test with RTLD_DEFAULT ---\n");
        dlerror();
        void *sym_printf_default = dlsym(RTLD_DEFAULT, "printf");
        err = dlerror();
        printf("dlsym(RTLD_DEFAULT, \"printf\"): %p (%s)\n", sym_printf_default, err ? err : "success");

        dlclose(handle2);
    } else {
        const char *err = dlerror();
        printf("dlopen failed: %s\n", err ? err : "unknown");
    }

    close(fd);
    printf("\n=== Test Complete ===\n");
    return 0;
}
