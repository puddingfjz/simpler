// /**
//  * 被 libtilefwk_backend_server.so 加载的目标 .so
//  * 包含实际的业务逻辑函数
//  */

// #include <cstdint>

// extern "C" {

// // 业务逻辑初始化函数
// __attribute__((visibility("default"))) int DynTileFwkBackendKernelServerInit(void* args)
// {
//     // 最简化的实现 - 只返回成功
//     (void)args;
//     return 0;
// }

// // 业务逻辑执行函数
// __attribute__((visibility("default"))) int DynTileFwkBackendKernelServer(void* args)
// {
//     // 最简化的实现 - 只返回成功
//     (void)args;
//     return 0;
// }

// } // extern "C"




#include "device_log.h"
#include <cstdint>
#include <cstdio> 
#include <sched.h>


extern "C" __attribute__((visibility("default"))) int StaticTileFwkBackendKernelServer(void *arg) {
    if (arg == nullptr) {
        // DEV_ERROR("%s", "Invalid kernel arguments: null pointer");
        return -1;
    }

    return 0;
}

extern "C" __attribute__((visibility("default"))) int DynTileFwkBackendKernelServerInit(void *arg) {
    InitLogSwitch();
    if (arg == nullptr) {
        // DEV_ERROR("%s", "Invalid kernel arguments: null pointer");
        return -1;
    }

    DEV_INFO("%s", "Hello World Kernel Init: Initializing AICPU kernel");
    return 0;
}

extern "C" __attribute__((visibility("default"))) int DynTileFwkBackendKernelServer(void *arg) {
    if (arg == nullptr) {
        // DEV_ERROR("%s", "Invalid kernel arguments: null pointer");
        return -1;
    }
    DEV_INFO("%s", "Hello World from AICPU Kernel!");

    DEV_INFO("%s", "Kernel execution completed successfully");
    return 0;
}
