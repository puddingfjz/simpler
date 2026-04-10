# Copyright (c) PyPTO Contributors.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details you may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.
# -----------------------------------------------------------------------------------------------------------
"""
Golden script for memfd_test example.

This test doesn't produce actual tensor outputs - it's a functionality test
that verifies memfd operations on AICPU.
"""

import torch

__outputs__ = ["dummy_output"]

RTOL = 1e-5
ATOL = 1e-5


def generate_inputs(params: dict) -> list:
    """Generate input tensors - minimal setup for memfd test."""
    # Create a dummy output tensor
    dummy_output = torch.zeros(1, dtype=torch.float32)

    return [
        ("dummy_output", dummy_output),
    ]


def compute_golden(tensors: dict, params: dict) -> None:
    """Compute golden results - dummy value for test framework."""
    tensors["dummy_output"][0] = 1.0  # Test passes if memfd operations succeed
