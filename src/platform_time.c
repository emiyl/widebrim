#include "platform_time.h"

#include <stdint.h>

#if defined(_WIN32)

#include <windows.h>

uint64_t platform_time_get_ticks(void) {
    static LARGE_INTEGER frequency;
    static int initialized = 0;

    if (!initialized) {
        QueryPerformanceFrequency(&frequency);
        initialized = 1;
    }

    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);

    return (uint64_t)(
        (counter.QuadPart * 1000ULL) / frequency.QuadPart
    );
}

#else

#include <time.h>

uint64_t platform_time_get_ticks(void) {
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (uint64_t)ts.tv_sec * 1000ULL
         + (uint64_t)ts.tv_nsec / 1000000ULL;
}

#endif