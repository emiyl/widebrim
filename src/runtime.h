#ifndef WIDEBRIM_RUNTIME_H
#define WIDEBRIM_RUNTIME_H

#include <stdbool.h>

#include <SDL3/SDL.h>

#include "clock.h"
#include "game_state.h"
#include "mode_spawner.h"
#include "window.h"

typedef struct {
    window *window;
    game_state state;
    mode_spawner spawner;
    widebrim_clock clock;
    bool running;
    bool speed_modifier;
    bool alpha_blend_enabled;
    Uint32 engine_skip_clock_event_type;
} widebrim_runtime;

int widebrim_runtime_init(widebrim_runtime *rt, const char *datafiles_root, const char *language);
void widebrim_runtime_destroy(widebrim_runtime *rt);
void widebrim_runtime_run(widebrim_runtime *rt);

#endif
