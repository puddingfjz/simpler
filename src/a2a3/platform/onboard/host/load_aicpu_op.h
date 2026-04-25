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

#ifndef LOAD_AICPU_OP_H
#define LOAD_AICPU_OP_H

#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>

#include "aicpu_op_config.h"

#ifdef BUILD_WITH_NEW_CANN
// Use static linking to libruntime.so instead of dynamic loading
// This avoids complex header dependencies
extern "C" {
    // Binary loading types
    typedef enum {
        RT_LOAD_BINARY_OPT_CPU_KERNEL_MODE = 3,
    } rtLoadBinaryOption;

    typedef union {
        int32_t cpuKernelMode;
    } rtLoadBinaryOptionValue_t;

    typedef struct {
        rtLoadBinaryOption optionId;
        rtLoadBinaryOptionValue_t value;
    } rtLoadBinaryOption_t;

    typedef struct {
        rtLoadBinaryOption_t *options;
        size_t numOpt;
    } rtLoadBinaryConfig_t;

    // CPU kernel args (from CANN kernel.h - must match exactly)
    typedef struct tagRtAicpuArgsEx {
        void *args;                      // args host mem addr
        void *hostInputInfoPtr;
        void *kernelOffsetInfoPtr;
        uint32_t argsSize;
        uint16_t hostInputInfoNum;
        uint16_t kernelOffsetInfoNum;
        uint32_t soNameAddrOffset;        // just for CCE Kernel, default value is 0xffff for FWK kernel
        uint32_t kernelNameAddrOffset;    // just for CCE Kernel, default value is 0xffff for FWK kernel
        bool isNoNeedH2DCopy;
        uint16_t timeout;
        uint8_t reserved;
    } rtAicpuArgsEx_t;

    typedef struct {
        rtAicpuArgsEx_t baseArgs;
        uint32_t reserved[8];
    } rtCpuKernelArgs_t;

    // Kernel launch config
    typedef enum {
        RT_LAUNCH_KERNEL_ATTR_LOCAL_MEM_SIZE = 2,
    } rtLaunchKernelAttrId;

    typedef union {
        uint32_t localMemorySize;
        uint32_t rsv[4];
    } rtLaunchKernelAttrVal_t;

    typedef struct {
        rtLaunchKernelAttrId id;
        rtLaunchKernelAttrVal_t value;
    } rtLaunchKernelAttr_t;

    typedef struct {
        rtLaunchKernelAttr_t *attrs;
        size_t numAttrs;
    } rtKernelLaunchCfg_t;

    // Function declarations (from libruntime.so)
    typedef void* rtBinHandle;
    typedef void* rtFuncHandle;
    typedef void* rtStream_t;
    typedef int rtError_t;

    rtError_t rtsBinaryLoadFromFile(const char *binPath, const rtLoadBinaryConfig_t *optionalCfg,
                                     rtBinHandle *handle);
    rtError_t rtsFuncGetByName(rtBinHandle binHandle, const char *kernelName, rtFuncHandle *funcHandle);
    rtError_t rtsLaunchCpuKernel(rtFuncHandle funcHandle, uint32_t blockDim, rtStream_t stm,
                                  const rtKernelLaunchCfg_t *cfg, rtCpuKernelArgs_t *argsInfo);
}
#endif

namespace simpler {

// Simple device kernel args structure (minimal for hello world)
struct DeviceKernelArgs {
    void* cfgdata;
    uint64_t reserved[8];
};

class LoadAicpuOp {
private:
#ifdef BUILD_WITH_NEW_CANN
    rtBinHandle binHandle_ = nullptr;
    std::string jsonPath_;
#endif

public:
    LoadAicpuOp() = default;
    ~LoadAicpuOp();

    // Generate JSON descriptor for kernel
    void GenKernelOpInfo(const std::string& jsonPath, const std::string& opType,
                        const std::string& functionName, const std::string& kernelSo);

    // Load binary using JSON descriptor
    int LoadKernelFromJson(const std::string& jsonPath, int cpuKernelMode = 0);

    // Get function handle by name
    int GetFuncHandle(const std::string& opType, rtFuncHandle& funcHandle);

    // Launch kernel
    int LaunchKernel(rtFuncHandle funcHandle, rtStream_t stream,
                    DeviceKernelArgs* kArgs, uint32_t blockDim);
};

} // namespace simpler

#endif // LOAD_AICPU_OP_H
