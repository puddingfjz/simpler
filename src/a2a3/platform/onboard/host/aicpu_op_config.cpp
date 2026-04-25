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

#include "aicpu_op_config.h"
#include <fstream>

namespace simpler {

void GenAicpuOpInfoJson(nlohmann::json& opConfigJson, const std::vector<AicpuOpConfig>& opConfigs) {
    for (const auto& opConfig : opConfigs) {
        // Use full format matching simpler's successful implementation
        opConfigJson[opConfig.opType]["opInfo"]["functionName"] = opConfig.functionName;
        opConfigJson[opConfig.opType]["opInfo"]["kernelSo"] = opConfig.kernelSo;
        opConfigJson[opConfig.opType]["opInfo"]["opKernelLib"] = opConfig.opKernelLib;
        opConfigJson[opConfig.opType]["opInfo"]["computeCost"] = opConfig.computeCost;
        opConfigJson[opConfig.opType]["opInfo"]["engine"] = opConfig.engine;
        opConfigJson[opConfig.opType]["opInfo"]["flagAsync"] = opConfig.flagAsync;
        opConfigJson[opConfig.opType]["opInfo"]["flagPartial"] = opConfig.flagPartial;
        opConfigJson[opConfig.opType]["opInfo"]["userDefined"] = opConfig.userDefined;
    }
}

} // namespace simpler
