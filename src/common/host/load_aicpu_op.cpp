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
 * AICPU Operation Loader Implementation
 */

#include "load_aicpu_op.h"

#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <mutex>
#include <unordered_set>
#include <vector>

#include "acl/acl.h"
#include "common/unified_log.h"
#include "runtime/rt.h"

namespace host {

namespace {

// Inner SO basename in the main scheduler's preinstall path. Same fingerprint
// scheme the dispatcher uses (see simpler_dispatcher::MakeInnerSoPath) so both
// sides land on the same file name without any other channel.
std::string MakeInnerSoBasename(uint64_t fp) {
    char buf[64];
    snprintf(buf, sizeof(buf), "simpler_inner_%016lx.so", fp);
    return buf;
}

// FNV-1a over first 64 bytes XOR'd with len. Must match
// simpler_dispatcher::Fingerprint exactly — host computes the preinstall
// filename, dispatcher writes to it.
uint64_t FingerprintBytes(const void *data, size_t len) {
    constexpr uint64_t kFnvOffset = 0xcbf29ce484222325ULL;
    constexpr uint64_t kFnvPrime = 0x100000001b3ULL;
    uint64_t h = kFnvOffset;
    size_t n = len < 64 ? len : 64;
    auto *p = reinterpret_cast<const unsigned char *>(data);
    for (size_t i = 0; i < n; ++i) {
        h ^= p[i];
        h *= kFnvPrime;
    }
    return h ^ static_cast<uint64_t>(len);
}

// Read whole file into memory.
bool ReadFileBytes(const std::string &path, std::vector<char> &out) {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in.is_open()) {
        LOG_ERROR("ReadFileBytes: cannot open %s: %s", path.c_str(), strerror(errno));
        return false;
    }
    std::streamsize len = in.tellg();
    in.seekg(0);
    out.resize(static_cast<size_t>(len));
    if (!in.read(out.data(), len)) {
        LOG_ERROR("ReadFileBytes: read failed for %s", path.c_str());
        return false;
    }
    return true;
}

// RAII for device memory.
struct DeviceBuf {
    void *ptr = nullptr;
    ~DeviceBuf() {
        if (ptr != nullptr) (void)aclrtFree(ptr);
    }
    aclError alloc(size_t bytes) { return aclrtMalloc(&ptr, bytes, ACL_MEM_MALLOC_HUGE_FIRST); }
};

// Process-level cache of inner-SO fingerprints we've already bootstrapped.
// Bootstrap is idempotent for same content (same FP → same preinstall file),
// so re-invoking libaicpu_extend_kernels is wasted work and incremental
// memory pressure. Multiple DeviceRunner instances in the same host process
// (e.g. sequential pytest tests in one xdist worker, or two ChipWorkers in
// the same process) share one entry per runtime here.
std::unordered_set<uint64_t> &BootstrappedFps() {
    static std::unordered_set<uint64_t> kSet;
    return kSet;
}
std::mutex &BootstrapMutex() {
    static std::mutex kMutex;
    return kMutex;
}

}  // namespace

int LoadAicpuOp::BootstrapDispatcher(
    const std::string &dispatcher_so_path, const void *inner_so_data, size_t inner_so_len, rtStream_t stream
) {
    if (inner_so_data == nullptr || inner_so_len == 0) {
        LOG_ERROR("BootstrapDispatcher: empty inner SO bytes");
        return -1;
    }
    // Compute inner SO fingerprint first (cheap, no IO) and capture per-instance state.
    inner_fp_ = FingerprintBytes(inner_so_data, inner_so_len);
    inner_so_basename_ = MakeInnerSoBasename(inner_fp_);

    // Skip the libaicpu_extend_kernels round-trip if a prior LoadAicpuOp in
    // this process already wrote the same inner SO to preinstall.
    {
        std::lock_guard<std::mutex> lock(BootstrapMutex());
        if (BootstrappedFps().count(inner_fp_) > 0) {
            LOG_INFO_V2("BootstrapDispatcher: inner SO fp=%016lx already bootstrapped, skipping", inner_fp_);
            return 0;
        }
    }

    // 1. Read dispatcher SO from disk; inner SO is already in memory.
    std::vector<char> dispatcher_bytes;
    if (!ReadFileBytes(dispatcher_so_path, dispatcher_bytes)) return -1;
    size_t dispatcher_len = dispatcher_bytes.size();
    const char *inner_bytes = reinterpret_cast<const char *>(inner_so_data);
    size_t inner_len = inner_so_len;

    // 2. Copy both onto device.
    DeviceBuf dev_dispatcher;
    DeviceBuf dev_inner;
    aclError rc = dev_dispatcher.alloc(dispatcher_len);
    if (rc != ACL_SUCCESS) {
        LOG_ERROR("BootstrapDispatcher: aclrtMalloc(dispatcher) failed: %d", rc);
        return rc;
    }
    rc = aclrtMemcpy(
        dev_dispatcher.ptr, dispatcher_len, dispatcher_bytes.data(), dispatcher_len, ACL_MEMCPY_HOST_TO_DEVICE
    );
    if (rc != ACL_SUCCESS) {
        LOG_ERROR("BootstrapDispatcher: aclrtMemcpy(dispatcher) failed: %d", rc);
        return rc;
    }
    rc = dev_inner.alloc(inner_len);
    if (rc != ACL_SUCCESS) {
        LOG_ERROR("BootstrapDispatcher: aclrtMalloc(inner) failed: %d", rc);
        return rc;
    }
    rc = aclrtMemcpy(dev_inner.ptr, inner_len, inner_bytes, inner_len, ACL_MEMCPY_HOST_TO_DEVICE);
    if (rc != ACL_SUCCESS) {
        LOG_ERROR("BootstrapDispatcher: aclrtMemcpy(inner) failed: %d", rc);
        return rc;
    }

    // 3. Build DeviceArgs. libaicpu_extend_kernels reads aicpu_so_bin/len/deviceId
    //    (offsets 96/104/112). Our dispatcher additionally reads inner_so_bin/len
    //    (offsets 120/128) — fields libaicpu_extend_kernels ignores.
    constexpr size_t kDeviceArgsBytes = 160;
    char host_dev_args[kDeviceArgsBytes] = {};
    auto write_qword = [&](size_t offset, uint64_t value) {
        std::memcpy(host_dev_args + offset, &value, sizeof(value));
    };
    write_qword(96, reinterpret_cast<uint64_t>(dev_dispatcher.ptr));
    write_qword(104, static_cast<uint64_t>(dispatcher_len));
    write_qword(112, 0);  // deviceId
    write_qword(120, reinterpret_cast<uint64_t>(dev_inner.ptr));
    write_qword(128, static_cast<uint64_t>(inner_len));

    DeviceBuf dev_args;
    rc = dev_args.alloc(kDeviceArgsBytes);
    if (rc != ACL_SUCCESS) {
        LOG_ERROR("BootstrapDispatcher: aclrtMalloc(device_args) failed: %d", rc);
        return rc;
    }
    rc = aclrtMemcpy(dev_args.ptr, kDeviceArgsBytes, host_dev_args, kDeviceArgsBytes, ACL_MEMCPY_HOST_TO_DEVICE);
    if (rc != ACL_SUCCESS) {
        LOG_ERROR("BootstrapDispatcher: aclrtMemcpy(device_args) failed: %d", rc);
        return rc;
    }

    // 4. Issue the Mode A KFC launch — libaicpu_extend_kernels dlopens our
    //    dispatcher with these bytes, dispatcher Init reads inner_so_bin/len
    //    from DeviceArgs and writes to preinstall.
    struct Args {
        struct {
            uint64_t unused[5] = {0};
            uint64_t device_args_ptr = 0;
            uint64_t pad[20] = {0};
        } k_args;
        char kernel_name[32];
        char so_name[32];
        char op_name[32];
    } args = {};
    args.k_args.device_args_ptr = reinterpret_cast<uint64_t>(dev_args.ptr);
    std::strncpy(args.kernel_name, "DynTileFwkKernelServerInit", sizeof(args.kernel_name) - 1);
    std::strncpy(args.so_name, "libaicpu_extend_kernels.so", sizeof(args.so_name) - 1);
    args.op_name[0] = '\0';

    rtAicpuArgsEx_t rt_args = {};
    rt_args.args = &args;
    rt_args.argsSize = sizeof(args);
    rt_args.kernelNameAddrOffset = offsetof(Args, kernel_name);
    rt_args.soNameAddrOffset = offsetof(Args, so_name);

    rtError_t rrc = rtAicpuKernelLaunchExWithArgs(
        rtKernelType_t::KERNEL_TYPE_AICPU_KFC, "AST_DYN_AICPU", 1, &rt_args, nullptr, stream, 0
    );
    if (rrc != RT_ERROR_NONE) {
        LOG_ERROR("BootstrapDispatcher: rtAicpuKernelLaunchExWithArgs failed: %d", rrc);
        return rrc;
    }
    rc = aclrtSynchronizeStream(stream);
    if (rc != ACL_SUCCESS) {
        LOG_ERROR("BootstrapDispatcher: aclrtSynchronizeStream failed: %d", rc);
        return rc;
    }
    LOG_INFO_V0(
        "BootstrapDispatcher: bundled dispatcher (%zu B) + inner SO (%zu B) uploaded; inner SO at %s", dispatcher_len,
        inner_len, inner_so_basename_.c_str()
    );
    {
        std::lock_guard<std::mutex> lock(BootstrapMutex());
        BootstrappedFps().insert(inner_fp_);
    }
    return 0;
}

int LoadAicpuOp::LaunchBuiltInOp(rtStream_t stream, KernelArgs *k_args, int aicpu_num, const std::string &kernel_name) {
    if (inner_so_basename_.empty()) {
        LOG_ERROR("LaunchBuiltInOp: BootstrapDispatcher must be called first");
        return -1;
    }
    // Mode A type 2 (KERNEL_TYPE_AICPU) targets the main aicpu_scheduler. It
    // dlopens `so_name` from the preinstall dir on first invocation, then
    // dispatches to `kernel_name`. Subsequent launches reuse the cached dlopen.
    //
    // so_name must hold "simpler_inner_<16-hex-fp>.so" = 33 chars; CANN
    // reads via rt_args.soNameAddrOffset (no size limit beyond what fits
    // before kernel_name overflows into it), so 64 bytes is safe headroom.
    struct Args {
        KernelArgs k_args;
        char kernel_name[32];
        char so_name[64];
        char op_name[32];
    } args = {};
    if (inner_so_basename_.size() >= sizeof(args.so_name)) {
        LOG_ERROR("LaunchBuiltInOp: inner SO basename too long: %s", inner_so_basename_.c_str());
        return -1;
    }
    args.k_args = *k_args;
    std::strncpy(args.kernel_name, kernel_name.c_str(), sizeof(args.kernel_name) - 1);
    std::strncpy(args.so_name, inner_so_basename_.c_str(), sizeof(args.so_name) - 1);
    args.op_name[0] = '\0';

    rtAicpuArgsEx_t rt_args = {};
    rt_args.args = &args;
    rt_args.argsSize = sizeof(args);
    rt_args.kernelNameAddrOffset = offsetof(Args, kernel_name);
    rt_args.soNameAddrOffset = offsetof(Args, so_name);

    rtError_t rc = rtAicpuKernelLaunchExWithArgs(
        rtKernelType_t::KERNEL_TYPE_AICPU, "AST_DYN_AICPU", aicpu_num, &rt_args, nullptr, stream, 0
    );
    if (rc != RT_ERROR_NONE) {
        LOG_ERROR("LaunchBuiltInOp(%s) failed: %d", kernel_name.c_str(), rc);
        return rc;
    }
    return 0;
}

}  // namespace host
