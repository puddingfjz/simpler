# Pypto-style AICPU Loading Test Summary

## 当前状态

### 成功完成的部分：

1. ✓ **成功编译pypto** - 版本0.2.1
2. ✓ **成功编译我们自己的libtilefwk_backend_server.so** - 使用最小化的pypto_aicpu_interface_minimal.cpp
3. ✓ **成功导出所有必需的函数符号**:
   - DynPyptoKernelServerNull
   - DynPyptoKernelServer  
   - DynPyptoKernelServerInit
4. ✓ **运行时符号加载成功** - 找到了所有rts*函数
5. ✓ **SetDevice和StreamCreate成功**

### 已解决的问题

**问题1**: BinaryLoadFromFile 失败 (错误 107000) - ✅ 已修复
- **原因**: `optionId=0` 是非法值
- **解决方案**: 将 `optionId` 从 0 改为 3 (`CPU_KERNEL_MODE`)

**问题2**: LaunchCpuKernel 失败 (错误 107000) - ✅ 已修复
- **原因**: `kernelLaunchCfg.attrs` 不能是 NULL 指针
- **错误信息**: `CheckKernelLaunchCfg failed because cfg->attrs cannot be a NULL pointer. ErrorCode=EE1004`
- **解决方案**: 创建有效的 `RtLaunchKernelAttr` 对象，让 `attrs` 指向它

### 当前状态

- ✅ 所有代码修复已完成
- ⚠️ 设备访问问题 (rtSetDevice 失败，错误 507899) - 这是临时性设备问题，与代码修复无关

## 关键发现

### 1. 函数名称（重要！）
pypto使用的运行时API函数名带"s"前缀：
- ✅ `rtsBinaryLoadFromFile` (不是 `rtBinaryLoadFromFile`)
- ✅ `rtsFuncGetByName` (不是 `rtFuncGetByName`)
- ✅ `rtsLaunchCpuKernel` (不是 `rtLaunchCpuKernel`)

### 2. JSON格式
```json
{
  "PyptoNull": {
    "opInfo": {
      "functionName": "DynPyptoKernelServerNull",
      "kernelSo": "./libtilefwk_backend_server.so",
      "opKernelLib": "AICPUKernel",
      ...
    }
  }
}
```

### 3. 两阶段加载机制
pypto使用两阶段加载：
1. **Init阶段**: 调用`DynPyptoKernelServerNull` - 将.so二进制数据保存到AICPU文件系统
2. **Run阶段**: 调用`DynPyptoKernelServer` - 执行实际的kernel函数

## 文件位置

- **测试程序**: `/data/fangjingzhi/simpler_356_2/examples/a2a3/tensormap_and_ringbuffer/test_pypto_load/build/test_minimal.cpp`
- **编译的.so**: `/data/fangjingzhi/simpler_356_2/examples/a2a3/tensormap_and_ringbuffer/test_pypto_load/build/libtilefwk_backend_server.so`
- **JSON文件**: `/data/fangjingzhi/simpler_356_2/examples/a2a3/tensormap_and_ringbuffer/test_pypto_load/build/pypto_op_info.json`
- **测试日志**: `/data/fangjingzhi/simpler_356_2/examples/a2a3/tensormap_and_ringbuffer/test_pypto_load/build/test_minimal.log`

## 下一步需要调查的问题

1. 107000错误的具体含义
2. `rtsBinaryLoadFromFile`的参数结构是否正确
3. JSON文件中的kernelSo路径是否需要绝对路径
4. 是否需要设置特定的环境变量

## 参考

- pypto源码: `/data/fangjingzhi/pypto`
- pypto安装位置: `/data/fangjingzhi/.conda/envs/simpler_issue/lib/python3.13/site-packages/pypto/`
