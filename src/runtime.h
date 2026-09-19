#ifndef WIDEBRIM_RUNTIME_H
#define WIDEBRIM_RUNTIME_H

#include <stdbool.h>

#include <SDL3/SDL.h>

#include "clock.h"
#include "game_state.h"
#include "input.h"
#include "mode_spawner.h"
#include "window.h"

typedef struct {
    window *window;
    input *input;
    game_state state;
    mode_spawner spawner;
    wb_clock clock;
    bool running;
    bool speed_modifier;
    bool alpha_blend_enabled;
    Uint32 engine_skip_clock_event_type;
} wb_runtime;

int wb_runtime_init(wb_runtime *rt, const char *datafiles_root, const char *language);
void wb_runtime_destroy(wb_runtime *rt);
void wb_runtime_run(wb_runtime *rt);

#endif
