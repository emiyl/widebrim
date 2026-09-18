#ifndef WIDEBRIM_GAME_STATE_H
#define WIDEBRIM_GAME_STATE_H

#include <stdbool.h>

#include <mh_datafiles.h>
#include <mh_font.h>

#include "mode.h"

/* Trimmed C port of Layton2GameState/Layton2CollectiveState, covering only
 * the fields needed for the Reset/Title/Room milestone. */
typedef struct {
    mh_datafiles datafiles;
    game_mode current_mode;
    game_mode next_mode;
    int place_num;
    bool first_touch_enabled;
    mh_font font_event;
    bool font_event_loaded;
} game_state;

int game_state_init(game_state *gs, const char *datafiles_root, const char *language);
void game_state_destroy(game_state *gs);

/* Clears transient session state (matches Layton2GameState.resetState(), trimmed to milestone scope). */
void game_state_reset(game_state *gs);

game_mode game_state_get_mode(const game_state *gs);
void game_state_set_mode(game_state *gs, game_mode mode);
game_mode game_state_get_mode_next(const game_state *gs);
void game_state_set_mode_next(game_state *gs, game_mode mode);

int game_state_get_place_num(const game_state *gs);
void game_state_set_place_num(game_state *gs, int place_num);

#endif
