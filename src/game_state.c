#include "game_state.h"

#include <string.h>
#include <stdio.h>

int game_state_init(game_state *gs, const char *datafiles_root, const char *language) {
    mh_buffer font_data;

    memset(gs, 0, sizeof(*gs));
    if (mh_datafiles_init(&gs->datafiles, datafiles_root, language) != 0) {
        return -1;
    }
    gs->current_mode = GAME_MODE_INVALID;
    gs->next_mode = GAME_MODE_INVALID;
    game_state_reset(gs);

    mh_buffer_init(&font_data);
    if (mh_datafiles_get_data(&gs->datafiles, "font/fontevent.NFTR", &font_data) == 0) {
        gs->font_event_loaded = mh_font_load_nftr(&gs->font_event, font_data.data, font_data.len) == 0;
    }
    mh_buffer_free(&font_data);

    return 0;
}

void game_state_destroy(game_state *gs) {
    if (gs->font_event_loaded) {
        mh_font_free(&gs->font_event);
    }
    mh_datafiles_free(&gs->datafiles);
}

void game_state_reset(game_state *gs) {
    memset(gs->room_hint_data, 0, sizeof(gs->room_hint_data));
    gs->place_num = 1;
    gs->event_id = -1;
    gs->first_touch_enabled = true;
    gs->hint_coin_encountered = 10u;
    gs->hint_coin_available = 10u;
}

bool game_state_room_hint_coin_found(const game_state *gs, int room_num, int coin_index) {
    const uint8_t *packed;
    uint8_t value;

    if (!gs || room_num < 0 || room_num >= 128 || coin_index < 0 || coin_index >= 4) {
        return false;
    }

    packed = &gs->room_hint_data[room_num / 2];
    value = (room_num & 1) ? ((packed[0] >> 4) & 0x0f) : (packed[0] & 0x0f);
    return (value & (1u << coin_index)) != 0u;
}

void game_state_room_hint_coin_set_found(game_state *gs, int room_num, int coin_index) {
    uint8_t *packed;
    uint8_t current_value;
    uint8_t mask;

    if (!gs || room_num < 0 || room_num >= 128 || coin_index < 0 || coin_index >= 4) {
        return;
    }

    packed = &gs->room_hint_data[room_num / 2];
    mask = (uint8_t)(1u << coin_index);
    if (room_num & 1) {
        current_value = (*packed >> 4) & 0x0f;
        *packed = (uint8_t)((*packed & 0x0f) | (((current_value | mask) & 0x0f) << 4));
    } else {
        current_value = *packed & 0x0f;
        *packed = (uint8_t)((*packed & 0xf0) | (current_value | mask));
    }
}

void game_state_hint_coin_mark_found(game_state *gs, int room_num, int coin_index) {
    if (!gs || game_state_room_hint_coin_found(gs, room_num, coin_index)) {
        return;
    }

    game_state_room_hint_coin_set_found(gs, room_num, coin_index);
    gs->hint_coin_encountered += 1u;
    gs->hint_coin_available += 1u;
}

game_mode game_state_get_mode(const game_state *gs) {
    return gs->current_mode;
}

void game_state_set_mode(game_state *gs, game_mode mode) {
    gs->current_mode = mode;
}

game_mode game_state_get_mode_next(const game_state *gs) {
    return gs->next_mode;
}

void game_state_set_mode_next(game_state *gs, game_mode mode) {
    gs->next_mode = mode;
}

game_mode game_state_consume_mode_next(game_state *gs) {
    game_mode mode = gs->next_mode;
    gs->next_mode = GAME_MODE_INVALID;
    return mode;
}

int game_state_get_place_num(const game_state *gs) {
    return gs->place_num;
}

void game_state_set_place_num(game_state *gs, int place_num) {
    fprintf(stderr, "widebrim: Setting place_num to %d\n", place_num);
    gs->place_num = place_num;
}

int game_state_get_event_id(const game_state *gs) {
    return gs->event_id;
}

void game_state_set_event_id(game_state *gs, int event_id) {
    gs->event_id = event_id;
}
