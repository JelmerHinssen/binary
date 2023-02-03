#pragma once
#define SHILM_IO_STATIC
#ifdef SHILM_IO_STATIC
    #define SHILM_IO_EXPORT
#else
    #ifdef SHILM_IO_COMPILE_DYNAMIC
        #define SHILM_IO_EXPORT __declspec(dllexport)
    #else
        #define SHILM_IO_EXPORT __declspec(dllimport)
    #endif // SHILM_IO_COMPILE_DYNAMIC
#endif // SHILM_IO_STATIC
