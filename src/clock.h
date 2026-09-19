#ifndef WIDEBRIM_CLOCK_H
#define WIDEBRIM_CLOCK_H

#include <stdint.h>

typedef struct {
    uint64_t prev_frame_counter;
} wb_clock;

void wb_clock_init(wb_clock *clock);

double wb_clock_tick(wb_clock *clock, double target_interval_sec);

#endif
