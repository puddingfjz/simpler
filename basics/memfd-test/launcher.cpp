/**
 * AICPU Memfd Test Launcher
 * Test memfd_create + write + dlopen system calls ON AICPU (not host)
 */

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <runtime/rt.h>

// AICPU kernel structures
struct DeviceArgs {
    uint64_t unused[12] = {0};
    uint64_t aicpuSoBin{0};
    uint64_t aicpuSoLen{0};
};

struct KernelArgs {
    uint64_t unused[5] = {0};
    int64_t *deviceArgs{nullptr};

    int InitDeviceArgs(const DeviceArgs &hostDeviceArgs) {
        if (deviceArgs == nullptr) {
            void *deviceArgsDev = nullptr;
            uint64_t deviceArgsSize = sizeof(DeviceArgs);
            int allocRc = rtMalloc(&deviceArgsDev, deviceArgsSize, RT_MEMORY_HBM, 0);
            if (allocRc != 0) {
                std::cerr << "Error: rtMalloc for deviceArgs failed: " << allocRc << '\n';
                return allocRc;
            }
            deviceArgs = reinterpret_cast<int64_t *>(deviceArgsDev);
        }
        int rc =
            rtMemcpy(deviceArgs, sizeof(DeviceArgs), &hostDeviceArgs, sizeof(DeviceArgs), RT_MEMCPY_HOST_TO_DEVICE);
        if (rc != 0) {
            std::cerr << "Error: rtMemcpy failed: " << rc << '\n';
            rtFree(deviceArgs);
            deviceArgs = nullptr;
            return rc;
        }
        return 0;
    }

    int FinalizeDeviceArgs() {
        if (deviceArgs != nullptr) {
            int rc = rtFree(deviceArgs);
            deviceArgs = nullptr;
            return rc;
        }
        return 0;
    }
};

// Load SO from file into device memory
struct AicpuSoInfo {
    uint64_t aicpuSoBin{0};
    uint64_t aicpuSoLen{0};

    int Init(const std::string &soPath) {
        std::ifstream file(soPath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open " << soPath << '\n';
            return -1;
        }

        size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);
        std::vector<char> buffer(fileSize);
        file.read(buffer.data(), fileSize);

        void *dAicpuData = nullptr;
        int rc = rtMalloc(&dAicpuData, fileSize, RT_MEMORY_HBM, 0);
        if (rc != 0) {
            std::cerr << "Error: rtMalloc failed: " << rc << '\n';
            return rc;
        }
        rc = rtMemcpy(dAicpuData, fileSize, buffer.data(), fileSize, RT_MEMCPY_HOST_TO_DEVICE);
        if (rc != 0) {
            std::cerr << "Error: rtMemcpy failed: " << rc << '\n';
            rtFree(dAicpuData);
            dAicpuData = nullptr;
            return rc;
        }

        aicpuSoBin = reinterpret_cast<uint64_t>(dAicpuData);
        aicpuSoLen = fileSize;
        std::cout << "[Host] Loaded SO: " << soPath << " (" << fileSize << " bytes) to device memory\n";
        return 0;
    }

    int Finalize() {
        if (aicpuSoBin != 0) {
            int rc = rtFree(reinterpret_cast<void *>(aicpuSoBin));
            aicpuSoBin = 0;
            return rc;
        }
        return 0;
    }
};

class DeviceRunner {
  public:
    static DeviceRunner &Get() {
        static DeviceRunner runner;
        return runner;
    }

    int LaunchAiCpuKernel(rtStream_t stream, KernelArgs *kArgs, const char *kernelName, int aicpuNum) {
        struct Args {
            KernelArgs kArgs;
            char kernelName[32];
            const char soName[32] = {"libaicpu_extend_kernels.so"};
            const char opName[32] = {""};
        } args;

        args.kArgs = *kArgs;
        strncpy(args.kernelName, kernelName, sizeof(args.kernelName) - 1);
        args.kernelName[sizeof(args.kernelName) - 1] = '\0';

        rtAicpuArgsEx_t rtArgs;
        memset(&rtArgs, 0, sizeof(rtArgs));
        rtArgs.args = &args;
        rtArgs.argsSize = sizeof(args);
        rtArgs.kernelNameAddrOffset = offsetof(struct Args, kernelName);
        rtArgs.soNameAddrOffset = offsetof(struct Args, soName);

        return rtAicpuKernelLaunchExWithArgs(rtKernelType_t::KERNEL_TYPE_AICPU_KFC, "AST_DYN_AICPU", aicpuNum, &rtArgs,
                                             nullptr, stream, 0);
    }

    int Run(rtStream_t stream, KernelArgs *kernelArgs, int launchAicpuNum = 1) {
        if (kernelArgs == nullptr) {
            std::cerr << "Error: kernelArgs is null\n";
            return -1;
        }

        // Launch init - this is where memfd_create is called ON AICPU
        int rc = LaunchAiCpuKernel(stream, kernelArgs, "DynTileFwkKernelServerInit", 1);
        if (rc != 0) {
            std::cerr << "Error: LaunchAiCpuKernel (init) failed: " << rc << '\n';
            return rc;
        }

        // Launch main kernel
        rc = LaunchAiCpuKernel(stream, kernelArgs, "DynTileFwkKernelServer", launchAicpuNum);
        if (rc != 0) {
            std::cerr << "Error: LaunchAiCpuKernel (main) failed: " << rc << '\n';
            return rc;
        }

        // Synchronize stream
        rc = rtStreamSynchronize(stream);
        if (rc != 0) {
            std::cerr << "Error: rtStreamSynchronize failed: " << rc << '\n';
            return rc;
        }

        return 0;
    }

  private:
    DeviceRunner() {}
};

int main(int argc, char **argv) {
    int deviceId = 9;
    if (argc > 1) {
        try {
            deviceId = std::stoi(argv[1]);
            if (deviceId < 0 || deviceId > 15) {
                std::cerr << "Error: deviceId out of range [0, 15]\n";
                return -1;
            }
        } catch (const std::exception &e) {
            std::cerr << "Error: invalid deviceId argument\n";
            return -1;
        }
    }

    std::string soPath = "./kernel/libtilefwk_backend_server.so";
    if (argc > 2) {
        soPath = argv[2];
    }

    std::cout << "=== AICPU Memfd Test ===\n";
    std::cout << "Device ID: " << deviceId << "\n";
    std::cout << "SO Path: " << soPath << "\n";
    std::cout << "\nThis test runs memfd_create + write ON AICPU (not host CPU)\n\n";

    // Set up device
    int devRc = rtSetDevice(deviceId);
    if (devRc != 0) {
        std::cerr << "Error: rtSetDevice failed: " << devRc << '\n';
        return devRc;
    }

    rtStream_t stream = nullptr;
    int rc = rtStreamCreate(&stream, 0);
    if (rc != 0) {
        std::cerr << "Error: rtStreamCreate failed: " << rc << '\n';
        return rc;
    }

    // Load SO to device memory
    AicpuSoInfo soInfo{};
    rc = soInfo.Init(soPath);
    if (rc != 0) {
        std::cerr << "Error: AicpuSoInfo::Init failed\n";
        rtStreamDestroy(stream);
        return rc;
    }

    // Prepare kernel args
    KernelArgs kernelArgs{};
    DeviceArgs deviceArgs{};
    deviceArgs.aicpuSoBin = soInfo.aicpuSoBin;
    deviceArgs.aicpuSoLen = soInfo.aicpuSoLen;
    rc = kernelArgs.InitDeviceArgs(deviceArgs);
    if (rc != 0) {
        std::cerr << "Error: KernelArgs::InitDeviceArgs failed\n";
        soInfo.Finalize();
        rtStreamDestroy(stream);
        return rc;
    }

    // Run AICPU kernel - memfd_create happens INSIDE AICPU
    std::cout << "[Host] Launching AICPU kernel (memfd test will run ON AICPU)...\n";
    DeviceRunner &runner = DeviceRunner::Get();
    rc = runner.Run(stream, &kernelArgs, 1);

    // Cleanup
    kernelArgs.FinalizeDeviceArgs();
    soInfo.Finalize();
    rtStreamDestroy(stream);

    if (rc == 0) {
        std::cout << "\n[Host] === Test Completed ===\n";
        std::cout << "[Host] Check AICPU output above for memfd test results\n";
        std::cout << "[Host] Look for: PASS/FAIL messages from AICPU\n";
    } else {
        std::cout << "\n[Host] === Test Failed ===\n";
    }

    return rc;
}
