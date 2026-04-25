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

#ifndef AICPU_OP_CONFIG_H
#define AICPU_OP_CONFIG_H

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace simpler {

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

void GenAicpuOpInfoJson(nlohmann::json& opConfigJson, const std::vector<AicpuOpConfig>& opConfigs);

} // namespace simpler

#endif // AICPU_OP_CONFIG_H
