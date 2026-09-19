#ifndef PLATFORM_TIME_H
#define PLATFORM_TIME_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

uint64_t platform_time_get_ticks(void);

#if defined(__cplusplus)
}
#endif

#endif // PLATFORM_TIME_H