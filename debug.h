#pragma once
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#ifdef DEBUG

#define DEBUG_PRINTF(fmt, ...) do { \
    printf(fmt "\n", ##__VA_ARGS__); \
} while (0)

#define DEBUG_PRINT(msg) do { \
    printf(msg "\n"); \
} while (0)

#define assertf(cond, fmt, ...) do { \
    if(!(cond)) { \
        fprintf(stderr, "[ERROR] (%s:%d) " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
        assert(cond); \
    } \
} while (0)

#else

#define DEBUG_PRINTF(fmt, ...) ((void) 0)

#define DEBUG_PRINT(msg) ((void) 0)

#define assertf(cond, fmt, ...) ((void) 0)

#endif

