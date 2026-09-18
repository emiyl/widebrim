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

void widebrim_game_state_load_scene(widebrim_game_state *state, uint32_t room_id) {
    if (state == NULL) {
        return;
    }

    switch (room_id) {
        case 1:
            widebrim_room_init_default(&state->current_room, 1, "courtyard");
            state->current_room.bg_r = 34;
            state->current_room.bg_g = 55;
            state->current_room.bg_b = 72;
            state->current_room.accent_r = 110;
            state->current_room.accent_g = 180;
            state->current_room.accent_b = 127;
            state->current_room.hotspot_x = 136;
            state->current_room.hotspot_y = 110;
            break;
        case 2:
            widebrim_room_init_default(&state->current_room, 2, "study");
            state->current_room.bg_r = 66;
            state->current_room.bg_g = 51;
            state->current_room.bg_b = 43;
            state->current_room.accent_r = 201;
            state->current_room.accent_g = 163;
            state->current_room.accent_b = 99;
            state->current_room.hotspot_x = 170;
            state->current_room.hotspot_y = 84;
            break;
        default:
            widebrim_room_init_default(&state->current_room, room_id, "room");
            break;
    }

    state->room_loaded = true;
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
    widebrim_game_state_load_scene(state, 1);
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
