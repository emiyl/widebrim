#include "clock.h"

#include <SDL3/SDL.h>

#define WB_CLOCK_PRECISION_SEC 0.0015

static double wb_clock_elapsed_sec(uint64_t since, uint64_t freq) {
    return (double)(SDL_GetPerformanceCounter() - since) / (double)freq;
}

void wb_clock_init(wb_clock *clock) {
    clock->prev_frame_counter = SDL_GetPerformanceCounter();
}

double wb_clock_tick(wb_clock *clock, double target_interval_sec) {
    uint64_t freq = SDL_GetPerformanceFrequency();
    uint64_t last = clock->prev_frame_counter;
    double time_idle = target_interval_sec - wb_clock_elapsed_sec(last, freq);
    double time_sleep = time_idle - WB_CLOCK_PRECISION_SEC;

    if (time_sleep > 0.0) {
        SDL_DelayNS((Uint64)(time_sleep * 1e9));
    }

    while (wb_clock_elapsed_sec(last, freq) < target_interval_sec) {
        // busy wait
    }

    clock->prev_frame_counter = SDL_GetPerformanceCounter();
    return wb_clock_elapsed_sec(last, freq) * 1000.0;
}
