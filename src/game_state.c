#include "game_state.h"

#include <string.h>

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
    if (mh_datafiles_get_data(&gs->datafiles, "data_lt2/font/fontevent.NFTR", &font_data) == 0) {
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

/* Clears session/progression state only - deliberately leaves current/next
 * mode untouched, since the mode spawner sets those around this call. */
void game_state_reset(game_state *gs) {
    gs->place_num = 0;
    gs->first_touch_enabled = true;
}

game_mode game_state_get_mode(const game_state *gs) {
    return gs->current_mode;
}

void game_state_set_mode(game_state *gs, game_mode mode) {
    gs->current_mode = mode;
    gs->next_mode = GAME_MODE_INVALID;
}

game_mode game_state_get_mode_next(const game_state *gs) {
    return gs->next_mode;
}

void game_state_set_mode_next(game_state *gs, game_mode mode) {
    gs->next_mode = mode;
}

int game_state_get_place_num(const game_state *gs) {
    return gs->place_num;
}

void game_state_set_place_num(game_state *gs, int place_num) {
    gs->place_num = place_num;
}
