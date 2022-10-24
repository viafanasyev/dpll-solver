#pragma once
#include <stdio.h>

#ifdef DEBUG

#define DEBUG_PRINTF(fmt, ...) do { \
    printf(fmt "\n", ##__VA_ARGS__); \
} while (0)

#else

#define DEBUG_PRINTF(fmt, ...) ((void) 0)

#endif

