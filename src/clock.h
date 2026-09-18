#ifndef WIDEBRIM_CLOCK_H
#define WIDEBRIM_CLOCK_H

#include <stdint.h>

/* Sleep/busy-wait hybrid frame timer, ported from widebrim's AltClock
 * (widebrim/widebrim/engine/state/clock.py). */
typedef struct {
    uint64_t prev_frame_counter;
} widebrim_clock;

void widebrim_clock_init(widebrim_clock *clock);

/* target_interval_sec is the desired frame interval (e.g. 1.0/60.0).
 * Returns the actual elapsed frame time in milliseconds. */
double widebrim_clock_tick(widebrim_clock *clock, double target_interval_sec);

#endif
