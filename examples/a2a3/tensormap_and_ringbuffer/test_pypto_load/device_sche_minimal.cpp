/**
 * Minimal implementation of StaticTileFwkBackendKernelServer
 * This provides the symbol required by AICPU without full pypto dependencies
 */

extern "C" {

/**
 * Static entry point - minimal stub
 * AICPU looks for this symbol when loading the .so file
 */
__attribute__((visibility("default"))) int StaticTileFwkBackendKernelServer(void* targ)
{
    (void)targ;
    return 0;
}

} // extern "C"
