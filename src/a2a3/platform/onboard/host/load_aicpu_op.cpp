/*
 * Copyright (c) PyPTO Contributors.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 * -----------------------------------------------------------------------------------------------------------
 */

#include "load_aicpu_op.h"
#include <fstream>
#include <iostream>
#include <cstring>

namespace simpler {

LoadAicpuOp::~LoadAicpuOp() {
#ifdef BUILD_WITH_NEW_CANN
    // Cleanup: Note - we don't free binHandle_ here as it may be managed by the runtime
    binHandle_ = nullptr;
#endif
}

void LoadAicpuOp::GenKernelOpInfo(const std::string& jsonPath, const std::string& opType,
                                  const std::string& functionName, const std::string& kernelSo) {
    nlohmann::json opConfigJson;

    // Generate three kernel configs like simpler does
    AicpuOpConfig initConfig;
    initConfig.opType = "DynTileFwkKernelServerInit";
    initConfig.functionName = "DynTileFwkKernelServerInit";
    initConfig.kernelSo = kernelSo;
    initConfig.opKernelLib = "KFCKernel";

    AicpuOpConfig runConfig;
    runConfig.opType = "DynTileFwkKernelServer";
    runConfig.functionName = "DynTileFwkKernelServer";
    runConfig.kernelSo = kernelSo;
    runConfig.opKernelLib = "KFCKernel";

    AicpuOpConfig nullConfig;
    nullConfig.opType = "DynTileFwkKernelServerNull";
    nullConfig.functionName = "DynTileFwkKernelServerNull";
    nullConfig.kernelSo = kernelSo;
    nullConfig.opKernelLib = "AICPUKernel";

    GenAicpuOpInfoJson(opConfigJson, {initConfig, runConfig, nullConfig});

    // Write JSON to file
    std::ofstream file(jsonPath);
    if (!file.is_open()) {
        std::cerr << "Failed to open JSON file for writing: " << jsonPath << std::endl;
        return;
    }

    file << opConfigJson.dump(4);
    file.close();

    std::cout << "Generated AICPU op JSON descriptor at: " << jsonPath << std::endl;
}

int LoadAicpuOp::LoadKernelFromJson(const std::string& jsonPath, int cpuKernelMode) {
#ifdef BUILD_WITH_NEW_CANN
    jsonPath_ = jsonPath;

    rtLoadBinaryConfig_t optionCfg = {};
    rtLoadBinaryOption_t loadBinOptions = {};
    optionCfg.options = &loadBinOptions;
    optionCfg.options->optionId = RT_LOAD_BINARY_OPT_CPU_KERNEL_MODE;
    optionCfg.options->value.cpuKernelMode = cpuKernelMode;
    optionCfg.numOpt = 1;

    std::cout << "Loading AICPU kernel from JSON: " << jsonPath << std::endl;
    std::cout << "CPU_KERNEL_MODE: " << cpuKernelMode << std::endl;

    // Direct call to rtsBinaryLoadFromFile (static linking, like simpler)
    auto ret = rtsBinaryLoadFromFile(jsonPath.c_str(), &optionCfg, &binHandle_);
    if (ret != 0) {
        std::cerr << "RuntimeBinaryLoadFromFile failed with error: " << ret << std::endl;
        return ret;
    }

    std::cout << "Successfully loaded AICPU kernel, binHandle: " << binHandle_ << std::endl;
    return 0;
#else
    std::cerr << "Error: BUILD_WITH_NEW_CANN not defined, cannot load kernel" << std::endl;
    return -1;
#endif
}

int LoadAicpuOp::GetFuncHandle(const std::string& opType, rtFuncHandle& funcHandle) {
#ifdef BUILD_WITH_NEW_CANN
    if (binHandle_ == nullptr) {
        std::cerr << "Error: binHandle_ is null, call LoadKernelFromJson first" << std::endl;
        return -1;
    }

    std::cout << "Getting function handle for opType: " << opType << std::endl;

    // Direct call to rtsFuncGetByName (static linking)
    auto ret = rtsFuncGetByName(binHandle_, opType.c_str(), &funcHandle);
    if (ret != 0) {
        std::cerr << "RuntimeFuncGetByName failed for opType '" << opType
                  << "' with error: " << ret << std::endl;
        return ret;
    }

    std::cout << "Successfully got function handle: " << funcHandle << std::endl;
    return 0;
#else
    std::cerr << "Error: BUILD_WITH_NEW_CANN not defined" << std::endl;
    return -1;
#endif
}

int LoadAicpuOp::LaunchKernel(rtFuncHandle funcHandle, rtStream_t stream,
                             DeviceKernelArgs* kArgs, uint32_t blockDim) {
#ifdef BUILD_WITH_NEW_CANN
    if (funcHandle == nullptr) {
        std::cerr << "Error: funcHandle is null" << std::endl;
        return -1;
    }

    std::cout << "Launching AICPU kernel with blockDim: " << blockDim << std::endl;

    // Setup CPU kernel args using CANN official types (matching pypto exactly)
    rtAicpuArgsEx_t rtArgs = {};
    rtArgs.args = kArgs;
    rtArgs.argsSize = sizeof(DeviceKernelArgs);

    rtCpuKernelArgs_t cpuArgs = {};
    cpuArgs.baseArgs = rtArgs;

    // Create attr object (pypto creates one but doesn't set specific values)
    rtLaunchKernelAttr_t launchKernelAttr = {};
    rtKernelLaunchCfg_t kernelLaunchCfg = {nullptr, 0};
    kernelLaunchCfg.attrs = &launchKernelAttr;

    auto ret = rtsLaunchCpuKernel(funcHandle, blockDim, stream,
                                   &kernelLaunchCfg, &cpuArgs);
    if (ret != 0) {
        std::cerr << "RuntimeLaunchCpuKernel failed with error: " << ret << std::endl;
        return ret;
    }

    std::cout << "Successfully launched AICPU kernel" << std::endl;
    return 0;
#else
    std::cerr << "Error: BUILD_WITH_NEW_CANN not defined" << std::endl;
    return -1;
#endif
}

} // namespace simpler
