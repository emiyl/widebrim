#ifndef WIDEBRIM_CLOCK_H
#define WIDEBRIM_CLOCK_H

#include <stdint.h>

typedef struct {
    uint64_t prev_frame_counter;
} widebrim_clock;

void widebrim_clock_init(widebrim_clock *clock);

double widebrim_clock_tick(widebrim_clock *clock, double target_interval_sec);

#endif
