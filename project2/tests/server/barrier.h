#pragma once

#define SERVER_FILE_PREFIX "server_files/"
#define CONCURRENCY_DEGREE 5

// Wait until 'CONCURRENCY_DEGREE' threads all have initiated barrier().
// Then, allow all of them to proceed.
// clang-format off
#ifdef __cplusplus
    extern "C" int barrier(void);
#else
    int barrier(void);
#endif
