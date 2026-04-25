/*
 * Copyright (c) PyPTO Contributors.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OR ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 * -----------------------------------------------------------------------------------------------------------
 */

#ifndef RUNTIME_API_WRAPPER_H
#define RUNTIME_API_WRAPPER_H

#ifdef BUILD_WITH_NEW_CANN

#include <cstdint>
#include <cstddef>

namespace simpler {

// Type definitions (matching CANN runtime)
constexpr int32_t RT_SUCCESS = 0;

typedef int32_t RtError;
typedef void *RtStream;
typedef void *RtFuncHandle;
typedef void *RtBinHandle;

// Binary loading configuration
enum class RtLoadBinaryOptionType : uint32_t {
    CPU_KERNEL_MODE = 1,
};

union RtLoadBinaryOptionValue {
    uint32_t cpuKernelMode;
};

struct RtLoadBinaryOption {
    RtLoadBinaryOptionType optionId;
    RtLoadBinaryOptionValue value;
};

struct RtLoadBinaryConfig {
    RtLoadBinaryOption* options;
    uint32_t numOpt;
};

// CPU kernel arguments
struct RtAicpuArgsEx {
    void* args;
    uint32_t argsSize;
    uint32_t reserved[4];
};

struct RtCpuKernelArgs {
    RtAicpuArgsEx baseArgs;
    uint32_t reserved[8];
};

// Kernel launch configuration
struct RtKernelLaunchCfg {
    void* attrs;
    uint32_t numAttrs;
};

// Dynamic loader for libruntime.so and libascendcl.so
// This avoids static initialization hang by loading libraries at runtime
class RuntimeLoader {
public:
    // Function pointer types for RTS API
    typedef RtError (*RtsBinaryLoadFromFileFunc)(const char* binPath, const RtLoadBinaryConfig* optionalCfg,
                                                  RtBinHandle* handle);
    typedef RtError (*RtsFuncGetByNameFunc)(RtBinHandle binHandle, const char* kernelName, RtFuncHandle* funcHandle);
    typedef RtError (*RtsLaunchCpuKernelFunc)(RtFuncHandle funcHandle, uint32_t numBlocks, RtStream stream,
                                               const RtKernelLaunchCfg* cfg, RtCpuKernelArgs* args);

    // Function pointer types for ACL API
    typedef RtError (*AclrtCreateStreamFunc)(RtStream* stm);
    typedef RtError (*AclrtDestroyStreamFunc)(RtStream stm);
    typedef RtError (*AclrtSynchronizeStreamFunc)(RtStream stm);
    typedef RtError (*AclrtSetDeviceFunc)(int32_t devId);

    // Load the runtime library and resolve symbols
    static bool Load();

    // Check if library is loaded
    static bool IsLoaded() { return loaded_; }

    // RTS API function pointers (from libruntime.so)
    static RtsBinaryLoadFromFileFunc rtsBinaryLoadFromFile;
    static RtsFuncGetByNameFunc rtsFuncGetByName;
    static RtsLaunchCpuKernelFunc rtsLaunchCpuKernel;

    // ACL API function pointers (from libascendcl.so)
    static AclrtCreateStreamFunc aclrtCreateStream;
    static AclrtDestroyStreamFunc aclrtDestroyStream;
    static AclrtSynchronizeStreamFunc aclrtSynchronizeStream;
    static AclrtSetDeviceFunc aclrtSetDevice;

private:
    static bool loaded_;
    static void* runtimeHandle_;   // Handle for libruntime.so
    static void* aclHandle_;       // Handle for libascendcl.so
};

namespace runtime_api {

// Wrapper functions for RTS API - now using dynamic loading
inline RtError BinaryLoadFromFile(const char* binPath, int cpuKernelMode, RtBinHandle* handle) {
    if (!RuntimeLoader::IsLoaded() && !RuntimeLoader::Load()) {
        return -1;  // Failed to load runtime
    }

    RtLoadBinaryConfig optionCfg;
    RtLoadBinaryOption loadBinOptions;
    optionCfg.options = &loadBinOptions;
    optionCfg.options->optionId = RtLoadBinaryOptionType::CPU_KERNEL_MODE;
    optionCfg.options->value.cpuKernelMode = static_cast<uint32_t>(cpuKernelMode);
    optionCfg.numOpt = 1;

    return RuntimeLoader::rtsBinaryLoadFromFile(binPath, &optionCfg, handle);
}

inline RtError FuncGetByName(RtBinHandle binHandle, const char* kernelName, RtFuncHandle* funcHandle) {
    if (!RuntimeLoader::IsLoaded() && !RuntimeLoader::Load()) {
        return -1;
    }
    return RuntimeLoader::rtsFuncGetByName(binHandle, kernelName, funcHandle);
}

inline RtError LaunchCpuKernel(RtFuncHandle funcHandle, uint32_t numBlocks, RtStream stream,
                               const RtKernelLaunchCfg* cfg, RtCpuKernelArgs* args) {
    if (!RuntimeLoader::IsLoaded() && !RuntimeLoader::Load()) {
        return -1;
    }
    return RuntimeLoader::rtsLaunchCpuKernel(funcHandle, numBlocks, stream, cfg, args);
}

inline RtError StreamCreate(RtStream* stm, int32_t priority) {
    (void)priority;  // Unused in ACL API
    if (!RuntimeLoader::IsLoaded() && !RuntimeLoader::Load()) {
        return -1;
    }
    return RuntimeLoader::aclrtCreateStream(stm);
}

inline RtError StreamDestroy(RtStream stm) {
    if (!RuntimeLoader::IsLoaded() && !RuntimeLoader::Load()) {
        return -1;
    }
    return RuntimeLoader::aclrtDestroyStream(stm);
}

inline RtError StreamSynchronize(RtStream stm) {
    if (!RuntimeLoader::IsLoaded() && !RuntimeLoader::Load()) {
        return -1;
    }
    return RuntimeLoader::aclrtSynchronizeStream(stm);
}

inline RtError SetDevice(int32_t devId) {
    if (!RuntimeLoader::IsLoaded() && !RuntimeLoader::Load()) {
        return -1;
    }
    return RuntimeLoader::aclrtSetDevice(devId);
}

} // namespace runtime_api

} // namespace simpler

#endif // BUILD_WITH_NEW_CANN

#endif // RUNTIME_API_WRAPPER_H
