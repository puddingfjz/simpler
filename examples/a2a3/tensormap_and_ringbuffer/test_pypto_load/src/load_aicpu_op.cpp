/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * Simplified version移植自pypto for testing libtilefwk_backend_server.so loading
 */

#include "load_aicpu_op.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <cstring>

using Json = nlohmann::json;

namespace {
// Pypto constants
const std::string BuiltInKernelInitName = "DynPyptoKernelServerInit";
const std::string BuiltInKernelRunName = "DynPyptoKernelServer";
const std::string BuiltInKernelNullName = "DynPyptoKernelServerNull";
const std::string BuiltInSoName = "libtilefwk_backend_server.so";
const std::string KfcKernerLib = "KFCKernel";
const std::string AicpuKernerLib = "AICPUKernel";
constexpr int BuiltInOpNum = 3;
std::string BuiltInFunName[BuiltInOpNum] = {"PyptoInit", "PyptoRun", "PyptoNull"};

// Simplified AicpuOpConfig for JSON generation
struct AicpuOpConfig {
    std::string functionName;
    std::string kernelSo;
    std::string opKernelLib;
    std::string computeCost = "100";
    std::string engine = "DNN_VM_AICPU";
    std::string flagAsync = "False";
    std::string flagPartial = "False";
    std::string userDefined = "False";
    std::string opType;
};

void GenAicpuOpInfoJson(Json& opConfigJson, const std::vector<AicpuOpConfig>& opConfigs) {
    for (const auto& config : opConfigs) {
        Json opInfo;
        opInfo["functionName"] = config.functionName;
        opInfo["kernelSo"] = config.kernelSo;
        opInfo["opKernelLib"] = config.opKernelLib;
        opInfo["computeCost"] = config.computeCost;
        opInfo["engine"] = config.engine;
        opInfo["flagAsync"] = config.flagAsync;
        opInfo["flagPartial"] = config.flagPartial;
        opInfo["userDefined"] = config.userDefined;

        opConfigJson[config.opType]["opInfo"] = opInfo;
    }
}

bool DumpFile(const std::string& content, const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << path << std::endl;
        return false;
    }
    file << content;
    file.close();
    return true;
}
}

namespace npu::tile_fwk {

void LoadAicpuOp::GenBuiltInOpInfo(const std::string& jsonPath)
{
    Json builtInOp;

    AicpuOpConfig pyptoInit;
    pyptoInit.functionName = BuiltInKernelInitName;
    pyptoInit.kernelSo = BuiltInSoName;
    pyptoInit.opKernelLib = KfcKernerLib;
    pyptoInit.opType = BuiltInFunName[0];

    AicpuOpConfig pyptoRun = pyptoInit;
    pyptoRun.opType = BuiltInFunName[1];
    pyptoRun.functionName = BuiltInKernelRunName;

    AicpuOpConfig pyptoNull = pyptoInit;
    pyptoNull.opType = BuiltInFunName[2];
    pyptoNull.opKernelLib = AicpuKernerLib;
    pyptoNull.functionName = BuiltInKernelNullName;

    GenAicpuOpInfoJson(builtInOp, {pyptoInit, pyptoRun, pyptoNull});

    builtInOpJsonPath_ = jsonPath + "/pypto_op_info.json";
    if (!DumpFile(builtInOp.dump(4), builtInOpJsonPath_)) {
        std::cerr << "Failed to generate JSON file" << std::endl;
        return;
    }
    std::cout << "Generated pypto_op_info.json at: " << builtInOpJsonPath_ << std::endl;
}

int LoadAicpuOp::GetBuiltInOpBinHandle()
{
    if (builtInOpJsonPath_.empty()) {
        std::cerr << "JSON path is empty" << std::endl;
        return -1;
    }

    RtLoadBinaryConfig optionCfg;
    RtLoadBinaryOption loadBinOptions{};
    optionCfg.options = &loadBinOptions;
    optionCfg.options->optionId = RtLoadBinaryOptionType::CPU_KERNEL_MODE;
    optionCfg.options->value.cpuKernelMode = 0;  // Built-in mode
    optionCfg.numOpt = 1;

    void* binHandle;
    auto ret = RuntimeBinaryLoadFromFile(builtInOpJsonPath_.c_str(), &optionCfg, reinterpret_cast<void**>(&binHandle));
    if (ret != 0) {
        std::cerr << "RuntimeBinaryLoadFromFile failed: " << ret << std::endl;
        return -1;
    }

    std::cout << "Successfully loaded binary, binHandle: " << binHandle << std::endl;

    for (int i = 0; i < BuiltInOpNum; i++) {
        RtFuncHandle funcHandle;
        ret = RuntimeFuncGetByName(binHandle, BuiltInFunName[i].c_str(), &funcHandle);
        if (ret != 0) {
            std::cerr << "RuntimeFuncGetByName failed for " << BuiltInFunName[i] << ": " << ret << std::endl;
            return -1;
        }
        builtInFuncMap_[BuiltInFunName[i]] = funcHandle;
        std::cout << "Got func handle for " << BuiltInFunName[i] << ": " << funcHandle << std::endl;
    }

    return 0;
}

int LoadAicpuOp::AicpuKernelLaunch(void* funcHandle, const RtStream& stream,
    DeviceKernelArgs* kArgs, const uint32_t& blockDim)
{
    RtFuncHandle aicpuFuncHandle = static_cast<RtFuncHandle>(funcHandle);
    RtAicpuArgsEx rtArgs{};
    rtArgs.args = kArgs;
    rtArgs.argsSize = sizeof(DeviceKernelArgs);

    RtCpuKernelArgs argInfo{};
    argInfo.baseArgs = rtArgs;
    RtKernelLaunchCfg kernelLaunchCfg = {nullptr, 0U};
    auto launchKernelAttr = std::make_unique<RtLaunchKernelAttr>();
    kernelLaunchCfg.attrs = launchKernelAttr.get();

    std::cout << "Launching kernel with blockDim: " << blockDim << std::endl;
    return RuntimeLaunchCpuKernel(aicpuFuncHandle, blockDim, stream, &kernelLaunchCfg, &argInfo);
}

int LoadAicpuOp::LaunchBuiltInOp(RtStream stream, DeviceKernelArgs* kArgs,
    const int& aicpuNum, const std::string& funcName)
{
    auto it = builtInFuncMap_.find(funcName);
    if (it == builtInFuncMap_.end()) {
        std::cerr << "Function name " << funcName << " not found in map" << std::endl;
        return -1;
    }
    RtFuncHandle funcHandle = it->second;

    std::cout << "Launching built-in op: " << funcName << std::endl;
    return AicpuKernelLaunch(funcHandle, stream, kArgs, aicpuNum);
}

} // namespace npu::tile_fwk
