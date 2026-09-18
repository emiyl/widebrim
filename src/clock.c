#include "clock.h"

#include <SDL3/SDL.h>

/* Matches AltClock.PLATFORM_CLOCK_PRECISION_SEC. */
#define WIDEBRIM_CLOCK_PRECISION_SEC 0.0015

static double widebrim_clock_elapsed_sec(uint64_t since, uint64_t freq) {
    return (double)(SDL_GetPerformanceCounter() - since) / (double)freq;
}

void widebrim_clock_init(widebrim_clock *clock) {
    clock->prev_frame_counter = SDL_GetPerformanceCounter();
}

double widebrim_clock_tick(widebrim_clock *clock, double target_interval_sec) {
    uint64_t freq = SDL_GetPerformanceFrequency();
    uint64_t last = clock->prev_frame_counter;
    double time_idle = target_interval_sec - widebrim_clock_elapsed_sec(last, freq);
    double time_sleep = time_idle - WIDEBRIM_CLOCK_PRECISION_SEC;

    if (time_sleep > 0.0) {
        SDL_DelayNS((Uint64)(time_sleep * 1e9));
    }

    while (widebrim_clock_elapsed_sec(last, freq) < target_interval_sec) {
        /* busy-wait remainder for precision, matching AltClock */
    }

    clock->prev_frame_counter = SDL_GetPerformanceCounter();
    return widebrim_clock_elapsed_sec(last, freq) * 1000.0;
}
