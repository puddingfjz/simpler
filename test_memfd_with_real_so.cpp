/*
 * Test memfd loading with actual SO file
 */

#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <cstring>
#include <cstdio>
#include <cerrno>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <path_to_so_file>\n", argv[0]);
        printf("Example: %s /usr/lib64/libc.so.6\n", argv[0]);
        return 1;
    }

    const char *so_path = argv[1];
    printf("Testing memfd loading with: %s\n", so_path);

    // Read SO file
    FILE *fp = fopen(so_path, "rb");
    if (!fp) {
        printf("FAIL: Cannot open %s (errno=%d)\n", so_path, errno);
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    long so_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (so_size <= 0) {
        printf("FAIL: Invalid file size\n");
        fclose(fp);
        return 1;
    }

    char *so_data = new char[so_size];
    size_t read_size = fread(so_data, 1, so_size, fp);
    fclose(fp);

    if (read_size != static_cast<size_t>(so_size)) {
        printf("FAIL: Read incomplete (%zu/%ld)\n", read_size, so_size);
        delete[] so_data;
        return 1;
    }

    printf("INFO: Loaded %ld bytes from %s\n", so_size, so_path);

    // Create memfd
    int fd = memfd_create("test_so", MFD_CLOEXEC);
    if (fd < 0) {
        printf("FAIL: memfd_create failed (errno=%d)\n", errno);
        delete[] so_data;
        return 1;
    }
    printf("PASS: memfd_create succeeded (fd=%d)\n", fd);

    // Write SO data to memfd
    ssize_t written = write(fd, so_data, so_size);
    if (written != so_size) {
        printf("FAIL: write incomplete (%zd/%ld)\n", written, so_size);
        close(fd);
        delete[] so_data;
        return 1;
    }
    printf("PASS: Wrote %ld bytes to memfd\n", so_size);

    // Construct /proc/self/fd/N path
    char memfd_path[256];
    snprintf(memfd_path, sizeof(memfd_path), "/proc/self/fd/%d", fd);
    printf("INFO: memfd path: %s\n", memfd_path);

    // Try dlopen from memfd
    printf("\n=== Testing dlopen from memfd ===\n");
    dlerror();
    void *handle = dlopen(memfd_path, RTLD_LAZY | RTLD_LOCAL);
    const char *err = dlerror();

    if (handle == nullptr) {
        printf("FAIL: dlopen from memfd failed: %s\n", err ? err : "unknown");
        close(fd);
        delete[] so_data;
        return 1;
    }

    printf("PASS: dlopen from memfd succeeded! Handle=%p\n", handle);

    // Try to get a symbol from the loaded SO
    Dl_info info;
    if (dladdr(dlsym(handle, "main"), &info)) {
        printf("INFO: Found symbol info: %s\n", info.dli_fname);
    }

    dlclose(handle);
    close(fd);
    delete[] so_data;

    printf("\n=== SUCCESS ===\n");
    printf("memfd loading works in this environment!\n");
    return 0;
}
