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

#include "runtime_api_wrapper.h"

#ifdef BUILD_WITH_NEW_CANN

#include <iostream>
#include <dlfcn.h>

namespace simpler {

// Static member initialization
bool RuntimeLoader::loaded_ = false;
void* RuntimeLoader::runtimeHandle_ = nullptr;
void* RuntimeLoader::aclHandle_ = nullptr;

// RTS API function pointers
RuntimeLoader::RtsBinaryLoadFromFileFunc RuntimeLoader::rtsBinaryLoadFromFile = nullptr;
RuntimeLoader::RtsFuncGetByNameFunc RuntimeLoader::rtsFuncGetByName = nullptr;
RuntimeLoader::RtsLaunchCpuKernelFunc RuntimeLoader::rtsLaunchCpuKernel = nullptr;

// ACL API function pointers
RuntimeLoader::AclrtCreateStreamFunc RuntimeLoader::aclrtCreateStream = nullptr;
RuntimeLoader::AclrtDestroyStreamFunc RuntimeLoader::aclrtDestroyStream = nullptr;
RuntimeLoader::AclrtSynchronizeStreamFunc RuntimeLoader::aclrtSynchronizeStream = nullptr;
RuntimeLoader::AclrtSetDeviceFunc RuntimeLoader::aclrtSetDevice = nullptr;

bool RuntimeLoader::Load() {
    if (loaded_) {
        return true;  // Already loaded
    }

    // Step 1: Load libruntime.so for RTS API
    const char* runtimeLibPaths[] = {
        "libruntime.so",
        "/usr/local/Ascend/ascend-toolkit/latest/lib64/libruntime.so",
        "/usr/local/Ascend/nnae/latest/lib64/libruntime.so",
        "/usr/local/Ascend/cann-8.5.0/lib64/libruntime.so",
        nullptr
    };

    for (int i = 0; runtimeLibPaths[i] != nullptr; ++i) {
        runtimeHandle_ = dlopen(runtimeLibPaths[i], RTLD_LAZY);
        if (runtimeHandle_ != nullptr) {
            std::cout << "[RuntimeLoader] Loaded libruntime.so from: " << runtimeLibPaths[i] << std::endl;
            break;
        }
    }

    if (runtimeHandle_ == nullptr) {
        std::cerr << "[RuntimeLoader] Failed to load libruntime.so: " << dlerror() << std::endl;
        return false;
    }

    // Step 2: Load libascendcl.so for ACL API
    const char* aclLibPaths[] = {
        "libascendcl.so",
        "/usr/local/Ascend/ascend-toolkit/latest/lib64/libascendcl.so",
        "/usr/local/Ascend/nnae/latest/lib64/libascendcl.so",
        "/usr/local/Ascend/cann-8.5.0/lib64/libascendcl.so",
        nullptr
    };

    for (int i = 0; aclLibPaths[i] != nullptr; ++i) {
        aclHandle_ = dlopen(aclLibPaths[i], RTLD_LAZY);
        if (aclHandle_ != nullptr) {
            std::cout << "[RuntimeLoader] Loaded libascendcl.so from: " << aclLibPaths[i] << std::endl;
            break;
        }
    }

    if (aclHandle_ == nullptr) {
        std::cerr << "[RuntimeLoader] Failed to load libascendcl.so: " << dlerror() << std::endl;
        dlclose(runtimeHandle_);
        runtimeHandle_ = nullptr;
        return false;
    }

    // Clear any existing error
    dlerror();

    // Resolve RTS API symbols from libruntime.so
    rtsBinaryLoadFromFile = reinterpret_cast<RtsBinaryLoadFromFileFunc>(
        dlsym(runtimeHandle_, "rtsBinaryLoadFromFile"));
    rtsFuncGetByName = reinterpret_cast<RtsFuncGetByNameFunc>(
        dlsym(runtimeHandle_, "rtsFuncGetByName"));
    rtsLaunchCpuKernel = reinterpret_cast<RtsLaunchCpuKernelFunc>(
        dlsym(runtimeHandle_, "rtsLaunchCpuKernel"));

    // Resolve ACL API symbols from libascendcl.so
    aclrtCreateStream = reinterpret_cast<AclrtCreateStreamFunc>(
        dlsym(aclHandle_, "aclrtCreateStream"));
    aclrtDestroyStream = reinterpret_cast<AclrtDestroyStreamFunc>(
        dlsym(aclHandle_, "aclrtDestroyStream"));
    aclrtSynchronizeStream = reinterpret_cast<AclrtSynchronizeStreamFunc>(
        dlsym(aclHandle_, "aclrtSynchronizeStream"));
    aclrtSetDevice = reinterpret_cast<AclrtSetDeviceFunc>(
        dlsym(aclHandle_, "aclrtSetDevice"));

    // Check for errors
    char* error = dlerror();
    if (error != nullptr) {
        std::cerr << "[RuntimeLoader] Failed to resolve symbols: " << error << std::endl;
        dlclose(runtimeHandle_);
        dlclose(aclHandle_);
        runtimeHandle_ = nullptr;
        aclHandle_ = nullptr;
        return false;
    }

    // Verify all symbols were resolved
    if (!rtsBinaryLoadFromFile || !rtsFuncGetByName || !rtsLaunchCpuKernel ||
        !aclrtCreateStream || !aclrtDestroyStream || !aclrtSynchronizeStream || !aclrtSetDevice) {
        std::cerr << "[RuntimeLoader] Some symbols were not resolved" << std::endl;
        dlclose(runtimeHandle_);
        dlclose(aclHandle_);
        runtimeHandle_ = nullptr;
        aclHandle_ = nullptr;
        return false;
    }

    loaded_ = true;
    std::cout << "[RuntimeLoader] Successfully loaded all runtime symbols" << std::endl;
    return true;
}

} // namespace simpler

#endif // BUILD_WITH_NEW_CANN
