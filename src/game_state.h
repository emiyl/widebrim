#ifndef WIDEBRIM_GAME_STATE_H
#define WIDEBRIM_GAME_STATE_H

#include <stdbool.h>
#include <stdint.h>

#include <mh_datafiles.h>
#include <mh_font.h>

#include "mode.h"

typedef struct {
    mh_datafiles datafiles;
    game_mode current_mode;
    game_mode next_mode;
    int place_num;
    int event_id;
    bool first_touch_enabled;
    uint8_t room_hint_data[64];
    uint8_t party_flags;
    uint16_t hint_coin_encountered;
    uint16_t hint_coin_available;
    mh_font font_event;
    bool font_event_loaded;
} game_state;

int game_state_init(game_state *gs, const char *datafiles_root, const char *language);
void game_state_destroy(game_state *gs);

void game_state_reset(game_state *gs);

game_mode game_state_get_mode(const game_state *gs);
void game_state_set_mode(game_state *gs, game_mode mode);
game_mode game_state_get_mode_next(const game_state *gs);
void game_state_set_mode_next(game_state *gs, game_mode mode);
game_mode game_state_consume_mode_next(game_state *gs);

int game_state_get_place_num(const game_state *gs);
void game_state_set_place_num(game_state *gs, int place_num);
int game_state_get_event_id(const game_state *gs);
void game_state_set_event_id(game_state *gs, int event_id);

bool game_state_party_member_active(const game_state *gs, int member_index);
void game_state_party_member_set_active(game_state *gs, int member_index, bool active);

bool game_state_room_hint_coin_found(const game_state *gs, int room_num, int coin_index);
void game_state_room_hint_coin_set_found(game_state *gs, int room_num, int coin_index);
void game_state_hint_coin_mark_found(game_state *gs, int room_num, int coin_index);

#endif
