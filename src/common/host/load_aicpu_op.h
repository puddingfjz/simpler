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
/**
 * @file load_aicpu_op.h
 * @brief Host-side AICPU operation loader.
 *
 * Two-phase architecture:
 *
 *   1. BootstrapDispatcher (per-DeviceRunner, idempotent across instances in
 *      the same process via a content-fingerprint cache): bundles dispatcher
 *      SO bytes + runtime SO bytes into a single Mode A KFC launch
 *      (`rtAicpuKernelLaunchExWithArgs`, kernel_type =
 *      `KERNEL_TYPE_AICPU_KFC`) targeting libaicpu_extend_kernels. Our
 *      dispatcher then writes the runtime SO to
 *      `/usr/lib64/aicpu_kernels/0/aicpu_kernels_device/simpler_inner_<fp>.so`
 *      using sched-thread (HwHiAiUser) write permission. The dispatcher SO
 *      itself is never persisted to disk.
 *
 *   2. LaunchBuiltInOp (per-task): direct Mode A launch
 *      (`rtAicpuKernelLaunchExWithArgs`, kernel_type = `KERNEL_TYPE_AICPU`)
 *      targeting the runtime SO by its preinstall name. The main aicpu_scheduler
 *      dlopens the preinstall file on first invocation and caches it; subsequent
 *      launches reuse the cached handle. No JSON descriptors, no global op
 *      registry, no per-launch handle bookkeeping.
 *
 * See common/aicpu_dispatcher/aicpu_dispatcher.h for the bootstrap protocol
 * (extended DeviceArgs with inner_so_bin/inner_so_len, fingerprint-named
 * preinstall files).
 */

#ifndef COMMON_HOST_LOAD_AICPU_OP_H_
#define COMMON_HOST_LOAD_AICPU_OP_H_

#include <cstdint>
#include <string>

#include "common/kernel_args.h"
#include "runtime/rt.h"

namespace host {

/**
 * @brief Host-side AICPU operation loader.
 *
 * One instance per DeviceRunner. Owns the per-instance bootstrap state
 * (inner SO fingerprint, derived preinstall basename); per-task launches are
 * stateless beyond that.
 */
class LoadAicpuOp {
public:
    LoadAicpuOp() = default;
    ~LoadAicpuOp() = default;

    LoadAicpuOp(const LoadAicpuOp &) = delete;
    LoadAicpuOp &operator=(const LoadAicpuOp &) = delete;
    LoadAicpuOp(LoadAicpuOp &&) = delete;
    LoadAicpuOp &operator=(LoadAicpuOp &&) = delete;

    /**
     * @brief One-shot bootstrap: upload runtime SO to preinstall via dispatcher.
     *
     * Bundles dispatcher SO bytes + runtime SO bytes into one
     * `rtAicpuKernelLaunchExWithArgs` (KFC) call targeting libaicpu_extend_kernels.
     * libaicpu_extend_kernels dlopens the dispatcher and invokes its Init; our
     * dispatcher reads the runtime SO bytes from the extended DeviceArgs and
     * writes them to `simpler_inner_<fp>.so` in the preinstall dir (HwHiAiUser-
     * writable from the sched thread).
     *
     * Records the runtime SO's content fingerprint internally so per-task
     * launches know which preinstall filename to address. Within a single host
     * process, a content-fingerprint cache skips redundant uploads when the
     * same runtime SO is bootstrapped from multiple DeviceRunner instances.
     *
     * @param dispatcher_so_path  Host path to libsimpler_aicpu_dispatcher.so
     * @param inner_so_data       Runtime SO bytes (caller-owned, must outlive call)
     * @param inner_so_len        Runtime SO size
     * @param stream              Stream on which to enqueue the bootstrap
     * @return 0 on success, error code on failure
     */
    int BootstrapDispatcher(
        const std::string &dispatcher_so_path, const void *inner_so_data, size_t inner_so_len, rtStream_t stream
    );

    /**
     * @brief Launch a runtime SO entry point via `rtAicpuKernelLaunchExWithArgs`.
     *
     * Targets the preinstall-resident runtime SO directly (kernel_type =
     * `KERNEL_TYPE_AICPU`, so_name = `simpler_inner_<fp>.so`). The main
     * aicpu_scheduler dlopens the SO on first call and caches the handle.
     *
     * @param stream       RTS stream
     * @param k_args       Kernel arguments
     * @param aicpu_num    Number of AICPU threads (1 for Init, N for Exec)
     * @param kernel_name  Symbol to invoke in the runtime SO
     *                     (KernelNames::InitName or KernelNames::RunName)
     * @return 0 on success, error code on failure
     */
    int LaunchBuiltInOp(rtStream_t stream, KernelArgs *k_args, int aicpu_num, const std::string &kernel_name);

private:
    uint64_t inner_fp_ = 0;          // Inner SO content fingerprint
    std::string inner_so_basename_;  // simpler_inner_<fp>.so (preinstall basename)
};

// Runtime SO's exported entry points. `LoadAicpuOp::LaunchBuiltInOp` resolves
// these against the dlopen of `simpler_inner_<fp>.so` performed by the main
// aicpu_scheduler.
namespace KernelNames {
constexpr const char *InitName = "simpler_aicpu_init";  // single-threaded init
constexpr const char *RunName = "simpler_aicpu_exec";   // multi-threaded exec
}  // namespace KernelNames

}  // namespace host

#endif  // COMMON_HOST_LOAD_AICPU_OP_H_
