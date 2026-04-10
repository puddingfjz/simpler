/*
 * Test memfd loading functionality for AICPU environment
 * Compile with: g++ -o test_memfd_aicpu test_memfd_aicpu.cpp -ldl
 * Run on device: ./test_memfd_aicpu <path_to_so_file>
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE  // Required for memfd_create on glibc
#endif

#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <cstring>
#include <cstdio>
#include <cerrno>

// AICPU-style logging macros
#define AICPU_LOG(fmt, ...) printf("[AICPU_TEST] " fmt "\n", ##__VA_ARGS__)
#define AICPU_LOG_ERROR(fmt, ...) printf("[AICPU_TEST] ERROR: " fmt "\n", ##__VA_ARGS__)
#define AICPU_FLUSH() fflush(stdout)

// Test result tracking
struct TestResult {
    const char *name;
    bool passed;
    char message[256];
};

static TestResult results[10];
static int result_count = 0;

static void record_result(const char *name, bool passed, const char *msg) {
    if (result_count < 10) {
        results[result_count].name = name;
        results[result_count].passed = passed;
        snprintf(results[result_count].message, sizeof(results[result_count].message), "%s", msg);
        result_count++;
    }
}

static void print_summary() {
    printf("\n");
    printf("========================================\n");
    printf("           TEST SUMMARY                 \n");
    printf("========================================\n");
    int passed = 0;
    for (int i = 0; i < result_count; i++) {
        printf("[%s] %s: %s\n",
               results[i].passed ? "PASS" : "FAIL",
               results[i].name,
               results[i].message);
        if (results[i].passed) passed++;
    }
    printf("========================================\n");
    printf("Total: %d/%d tests passed\n", passed, result_count);
    printf("========================================\n");
}

// Test 1: memfd_create availability
static bool test_memfd_create_available() {
    AICPU_LOG("Test 1: Checking memfd_create availability...");
    AICPU_FLUSH();

    int fd = memfd_create("test_available", MFD_CLOEXEC);
    if (fd < 0) {
        char msg[128];
        snprintf(msg, sizeof(msg), "memfd_create not available (errno=%d: %s)", errno, strerror(errno));
        record_result("memfd_create availability", false, msg);
        return false;
    }

    close(fd);
    record_result("memfd_create availability", true, "memfd_create is available");
    AICPU_LOG("  PASS: memfd_create is available");
    return true;
}

// Test 2: Write to memfd
static bool test_memfd_write() {
    AICPU_LOG("Test 2: Testing write to memfd...");
    AICPU_FLUSH();

    int fd = memfd_create("test_write", MFD_CLOEXEC);
    if (fd < 0) {
        record_result("memfd write", false, "memfd_create failed");
        return false;
    }

    const char *test_data = "Hello, memfd on AICPU!";
    size_t data_len = strlen(test_data);
    ssize_t written = write(fd, test_data, data_len);

    if (written != static_cast<ssize_t>(data_len)) {
        close(fd);
        char msg[128];
        snprintf(msg, sizeof(msg), "write incomplete (%zd/%zu)", written, data_len);
        record_result("memfd write", false, msg);
        return false;
    }

    // Verify by reading back
    char read_buf[64];
    lseek(fd, 0, SEEK_SET);
    ssize_t read_bytes = read(fd, read_buf, sizeof(read_buf) - 1);
    close(fd);

    if (read_bytes != static_cast<ssize_t>(data_len) ||
        strncmp(test_data, read_buf, data_len) != 0) {
        record_result("memfd write", false, "read verification failed");
        return false;
    }

    record_result("memfd write", true, "write and read verification passed");
    AICPU_LOG("  PASS: Write to memfd succeeded with verification");
    return true;
}

// Test 3: /proc/self/fd/N accessibility
static bool test_proc_fd_access() {
    AICPU_LOG("Test 3: Testing /proc/self/fd/N access...");
    AICPU_FLUSH();

    int fd = memfd_create("test_proc", MFD_CLOEXEC);
    if (fd < 0) {
        record_result("/proc/self/fd/N access", false, "memfd_create failed");
        return false;
    }

    // Write some data
    const char *test_data = "proc_test";
    write(fd, test_data, strlen(test_data));

    // Construct /proc/self/fd/N path
    char path[256];
    snprintf(path, sizeof(path), "/proc/self/fd/%d", fd);

    // Try to open via /proc
    FILE *fp = fopen(path, "r");
    if (fp == nullptr) {
        close(fd);
        char msg[128];
        snprintf(msg, sizeof(msg), "cannot open %s (errno=%d)", path, errno);
        record_result("/proc/self/fd/N access", false, msg);
        return false;
    }

    // Verify content
    char read_buf[64];
    size_t read_size = fread(read_buf, 1, sizeof(read_buf) - 1, fp);
    fclose(fp);
    close(fd);

    read_buf[read_size] = '\0';
    if (strcmp(test_data, read_buf) != 0) {
        record_result("/proc/self/fd/N access", false, "content mismatch");
        return false;
    }

    record_result("/proc/self/fd/N access", true, "can access and read via /proc/self/fd/N");
    AICPU_LOG("  PASS: /proc/self/fd/N is accessible");
    return true;
}

// Test 4: dlopen from memfd (requires SO file)
static bool test_dlopen_from_memfd(const char *so_path) {
    AICPU_LOG("Test 4: Testing dlopen from memfd with actual SO...");
    AICPU_FLUSH();

    // Read SO file
    FILE *fp = fopen(so_path, "rb");
    if (!fp) {
        char msg[256];
        snprintf(msg, sizeof(msg), "cannot open SO file %s (errno=%d)", so_path, errno);
        record_result("dlopen from memfd", false, msg);
        return false;
    }

    fseek(fp, 0, SEEK_END);
    long so_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (so_size <= 0) {
        fclose(fp);
        record_result("dlopen from memfd", false, "invalid SO file size");
        return false;
    }

    char *so_data = new char[so_size];
    size_t read_size = fread(so_data, 1, so_size, fp);
    fclose(fp);

    if (read_size != static_cast<size_t>(so_size)) {
        delete[] so_data;
        record_result("dlopen from memfd", false, "SO file read incomplete");
        return false;
    }

    AICPU_LOG("  Loaded %ld bytes from %s", so_size, so_path);
    AICPU_FLUSH();

    // Create memfd
    int fd = memfd_create("test_so_dlopen", MFD_CLOEXEC);
    if (fd < 0) {
        delete[] so_data;
        char msg[128];
        snprintf(msg, sizeof(msg), "memfd_create failed (errno=%d)", errno);
        record_result("dlopen from memfd", false, msg);
        return false;
    }

    AICPU_LOG("  memfd_create succeeded (fd=%d)", fd);
    AICPU_FLUSH();

    // Write SO data
    ssize_t written = write(fd, so_data, so_size);
    if (written != so_size) {
        close(fd);
        delete[] so_data;
        char msg[128];
        snprintf(msg, sizeof(msg), "write incomplete (%zd/%ld)", written, so_size);
        record_result("dlopen from memfd", false, msg);
        return false;
    }

    AICPU_LOG("  Wrote %ld bytes to memfd", so_size);
    AICPU_FLUSH();

    // Construct /proc/self/fd/N path
    char memfd_path[256];
    snprintf(memfd_path, sizeof(memfd_path), "/proc/self/fd/%d", fd);
    AICPU_LOG("  memfd path: %s", memfd_path);
    AICPU_FLUSH();

    // Try dlopen
    dlerror();
    void *handle = dlopen(memfd_path, RTLD_LAZY | RTLD_LOCAL);
    const char *err = dlerror();

    if (handle == nullptr) {
        close(fd);
        delete[] so_data;
        char msg[256];
        snprintf(msg, sizeof(msg), "dlopen failed: %s", err ? err : "unknown");
        record_result("dlopen from memfd", false, msg);
        AICPU_LOG_ERROR("  dlopen from memfd failed: %s", err ? err : "unknown");
        return false;
    }

    AICPU_LOG("  dlopen from memfd succeeded! handle=%p", handle);
    AICPU_FLUSH();

    // Try to get a symbol
    Dl_info info;
    void *addr = dlsym(handle, "main");
    if (addr != nullptr && dladdr(addr, &info)) {
        AICPU_LOG("  Found symbol: %s", info.dli_fname ? info.dli_fname : "(null)");
    }

    dlclose(handle);
    close(fd);
    delete[] so_data;

    char msg[256];
    snprintf(msg, sizeof(msg), "successfully loaded %s (%ld bytes)", so_path, so_size);
    record_result("dlopen from memfd", true, msg);
    AICPU_LOG("  PASS: dlopen from memfd completed successfully");
    return true;
}

// Test 5: memfd size limit check
static bool test_memfd_size_limit() {
    AICPU_LOG("Test 5: Testing memfd with large data (4MB)...");
    AICPU_FLUSH();

    const size_t large_size = 4 * 1024 * 1024;  // 4MB, typical max SO size
    char *large_data = new char[large_size];
    memset(large_data, 0xAA, large_size);

    int fd = memfd_create("test_large", MFD_CLOEXEC);
    if (fd < 0) {
        delete[] large_data;
        record_result("memfd size limit", false, "memfd_create failed");
        return false;
    }

    ssize_t written = write(fd, large_data, large_size);
    delete[] large_data;

    if (written != static_cast<ssize_t>(large_size)) {
        close(fd);
        char msg[128];
        snprintf(msg, sizeof(msg), "write incomplete (%zd/%zu)", written, large_size);
        record_result("memfd size limit", false, msg);
        return false;
    }

    close(fd);
    char msg[128];
    snprintf(msg, sizeof(msg), "successfully wrote %zu bytes", large_size);
    record_result("memfd size limit", true, msg);
    AICPU_LOG("  PASS: Large memfd write (4MB) succeeded");
    return true;
}

// Test 6: Multiple memfd create/close cycles
static bool test_memfd_cycles() {
    AICPU_LOG("Test 6: Testing multiple memfd create/close cycles...");
    AICPU_FLUSH();

    const int cycles = 10;
    bool all_passed = true;

    for (int i = 0; i < cycles; i++) {
        int fd = memfd_create("test_cycle", MFD_CLOEXEC);
        if (fd < 0) {
            char msg[128];
            snprintf(msg, sizeof(msg), "failed at cycle %d (errno=%d)", i, errno);
            record_result("memfd cycles", false, msg);
            return false;
        }
        close(fd);
    }

    char msg[128];
    snprintf(msg, sizeof(msg), "completed %d create/close cycles", cycles);
    record_result("memfd cycles", true, msg);
    AICPU_LOG("  PASS: Multiple cycles completed");
    return true;
}

int main(int argc, char *argv[]) {
    printf("========================================\n");
    printf("    AICPU memfd Functionality Test      \n");
    printf("========================================\n");
    printf("\n");

    bool has_so_file = (argc >= 2);

    // Run basic tests
    test_memfd_create_available();
    test_memfd_write();
    test_proc_fd_access();
    test_memfd_size_limit();
    test_memfd_cycles();

    // Run dlopen test if SO file provided
    if (has_so_file) {
        AICPU_LOG("\nSO file provided: %s", argv[1]);
        test_dlopen_from_memfd(argv[1]);
    } else {
        AICPU_LOG("\nNo SO file provided - skipping dlopen test");
        AICPU_LOG("To test dlopen, run: %s <path_to_so_file>", argv[0]);
    }

    // Print summary
    print_summary();

    // Return 0 if all tests passed
    int all_passed = 1;
    for (int i = 0; i < result_count; i++) {
        if (!results[i].passed) {
            all_passed = 0;
            break;
        }
    }

    return all_passed ? 0 : 1;
}
