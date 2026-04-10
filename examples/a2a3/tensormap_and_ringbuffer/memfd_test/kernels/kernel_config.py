# Copyright (c) PyPTO Contributors.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details you may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.
# -----------------------------------------------------------------------------------------------------------
"""
memfd_test - AICPU memfd functionality test

This example tests memfd_create and related functionality on AICPU.
No AICore kernels are used - this is purely an AICPU test.
"""

from pathlib import Path

from task_interface import ArgDirection as D  # pyright: ignore[reportAttributeAccessIssue]

_KERNELS_ROOT = Path(__file__).parent

# No AICore kernels needed for this test
KERNELS = []

# Orchestration config - device-side orchestration on AICPU
ORCHESTRATION = {
    "source": str(_KERNELS_ROOT / "orchestration" / "memfd_test_orch.cpp"),
    "function_name": "aicpu_orchestration_entry",
    "signature": [D.OUT],  # Dummy output for test framework
}

# Runtime configuration for tensormap_and_ringbuffer
RUNTIME_CONFIG = {
    "runtime": "tensormap_and_ringbuffer",
    "aicpu_thread_num": 1,  # Single AICPU thread for test
    "block_dim": 1,
    "rounds": 1,
}
