#include "engine.h"

#include <stdio.h>

void widebrim_game_state_init(widebrim_game_state *state) {
    if (state == NULL) {
        return;
    }

    state->current_mode = WIDEBRIM_MODE_BOOT;
    state->frame_counter = 0;
    state->last_tick_ms = 0;
    widebrim_madhatter_init(&state->madhatter);
}

void widebrim_game_state_destroy(widebrim_game_state *state) {
    if (state == NULL) {
        return;
    }

    widebrim_madhatter_free(&state->madhatter);
    state->current_mode = WIDEBRIM_MODE_BOOT;
    state->frame_counter = 0;
}
