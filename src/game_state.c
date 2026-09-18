#include "engine.h"

#include <stdio.h>

void widebrim_room_init_default(widebrim_room *room, uint32_t id, const char *name) {
    if (room == NULL) {
        return;
    }

    memset(room, 0, sizeof(*room));
    room->id = id;
    if (name != NULL) {
        snprintf(room->name, sizeof(room->name), "%s", name);
    } else {
        snprintf(room->name, sizeof(room->name), "room_%u", (unsigned)id);
    }

    room->bg_r = 22;
    room->bg_g = 29;
    room->bg_b = 36;
    room->accent_r = 88;
    room->accent_g = 161;
    room->accent_b = 145;
    room->hotspot_x = WIDEBRIM_SCREEN_WIDTH / 2;
    room->hotspot_y = WIDEBRIM_SCREEN_HEIGHT / 2;
}

void widebrim_game_state_init(widebrim_game_state *state) {
    if (state == NULL) {
        return;
    }

    memset(state, 0, sizeof(*state));
    state->current_mode = WIDEBRIM_MODE_BOOT;
    state->frame_counter = 0;
    state->last_tick_ms = 0;
    state->mode_elapsed_sec = 0.0f;
    widebrim_room_init_default(&state->current_room, 1, "debug_room");
    state->room_loaded = true;
    widebrim_madhatter_init(&state->madhatter);
}

void widebrim_game_state_destroy(widebrim_game_state *state) {
    if (state == NULL) {
        return;
    }

    widebrim_madhatter_free(&state->madhatter);
    state->current_mode = WIDEBRIM_MODE_BOOT;
    state->frame_counter = 0;
    state->mode_elapsed_sec = 0.0f;
    state->room_loaded = false;
}
