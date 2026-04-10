# AICPU Memfd Test

测试 **AICPU 进程内部**能否执行 `memfd_create + write` 系统调用。

## 背景

AICPU 运行在 NPU 上，可能不是完整的 Linux 环境。本测试验证 memfd 相关系统调用是否可用。

## 测试原理

```
Host CPU                              AICPU (NPU)
   │                                     │
   │  1. 加载 SO 到设备内存               │
   │  rtMalloc + rtMemcpy                │
   │                                     │
   │  2. Launch Kernel                   │
   │  rtAicpuKernelLaunchExWithArgs  ───▶│
   │                                     │
   │  3. AICPU 执行 kernel 代码          │
   │     - memfd_create() ◄──────────────│  测试点 1
   │     - write()                       │  测试点 2
   │     - close()                       │
   │                                     │
   │  4. AICPU 输出日志 ─────────────────│
   │     [AICPU] PASS/FAIL              │
```

## 编译

```bash
cd basics/memfd-test
mkdir build && cd build
cmake ..
make
```

## 运行

```bash
# 使用默认设备 ID (9)
./memfd_test_launcher

# 指定设备 ID
./memfd_test_launcher 0

# 指定设备 ID 和 SO 路径
./memfd_test_launcher 0 /path/to/libtilefwk_backend_server.so
```

## 预期输出

### 如果 AICPU 支持 memfd：

```
=== AICPU Memfd Test ===
Device ID: 9
SO Path: ./kernel/libtilefwk_backend_server.so

This test runs memfd_create + write ON AICPU (not host CPU)

[Host] Loaded SO: ./kernel/libtilefwk_backend_server.so (XXXX bytes) to device memory
[Host] Launching AICPU kernel (memfd test will run ON AICPU)...

[AICPU] memfd_test_kernel: Init
[AICPU] Testing memfd_create...
[AICPU] PASS: memfd_create succeeded (fd=3)
[AICPU] Testing write to memfd...
[AICPU] PASS: write succeeded (18 bytes)
[AICPU] memfd path: /proc/self/fd/3
[AICPU] memfd test completed successfully
[AICPU] memfd_test_kernel: Running

[Host] === Test Completed ===
```

### 如果 AICPU 不支持 memfd：

```
[AICPU] memfd_test_kernel: Init
[AICPU] Testing memfd_create...
[AICPU] FAIL: memfd_create failed (errno=38)  # ENOSYS: 系统调用不存在
```

## 文件说明

| 文件 | 说明 |
|------|------|
| `kernel/memfd_test_kernel.cpp` | AICPU kernel，内部调用 memfd_create + write |
| `launcher.cpp` | Host 端程序，加载 SO 并启动 AICPU kernel |
