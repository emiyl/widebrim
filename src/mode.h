#ifndef WIDEBRIM_MODE_H
#define WIDEBRIM_MODE_H

#include <stdbool.h>

#include "screen.h"

/* Matches widebrim's GAMEMODES enum values exactly (widebrim/widebrim/engine/state/enum_mode.py). */
typedef enum {
    GAME_MODE_RESET = 0,
    GAME_MODE_ROOM = 1,
    GAME_MODE_DRAMA_EVENT = 3,
    GAME_MODE_MOVIE = 6,
    GAME_MODE_START_PUZZLE = 7,
    GAME_MODE_END_PUZZLE = 8,
    GAME_MODE_STAY_PUZZLE = 9,
    GAME_MODE_PUZZLE = 10,
    GAME_MODE_UNK_NAZO = 11,
    GAME_MODE_TITLE = 12,
    GAME_MODE_NARRATION = 13,
    GAME_MODE_SUB_CAMERA = 14,
    GAME_MODE_SUB_HERB_TEA = 15,
    GAME_MODE_SUB_HAMSTER = 16,
    GAME_MODE_BAG = 17,
    GAME_MODE_NAME = 18,
    GAME_MODE_JITEN_BAG = 19,
    GAME_MODE_MYSTERY = 20,
    GAME_MODE_STAFF = 21,
    GAME_MODE_JITEN_WIFI = 22,
    GAME_MODE_MEMO = 23,
    GAME_MODE_CHALLENGE = 24,
    GAME_MODE_EVENT_TEA = 25,
    GAME_MODE_UNK_SUB_PHOTO0 = 26,
    GAME_MODE_UNK_SUB_PHOTO1 = 27,
    GAME_MODE_SECRET_MENU = 28,
    GAME_MODE_WIFI_SECRET_MENU = 29,
    GAME_MODE_TOP_SECRET_MENU = 30,
    GAME_MODE_JITEN_SECRET = 31,
    GAME_MODE_ART_MODE = 32,
    GAME_MODE_CHR_VIEW_MODE = 33,
    GAME_MODE_MUSIC_MODE = 34,
    GAME_MODE_VOICE_MODE = 35,
    GAME_MODE_MOVIE_VIEW_MODE = 36,
    GAME_MODE_HAMSTER_NAME = 37,
    GAME_MODE_NINTENDO_WFC_SETUP = 38,
    GAME_MODE_WIFI_DOWNLOAD_PUZZLE = 39,
    GAME_MODE_PASSCODE = 40,
    GAME_MODE_CODE_INPUT_PANDORA = 41,
    GAME_MODE_CODE_INPUT_FUTURE = 42,
    GAME_MODE_DIARY = 43,
    GAME_MODE_NAZOBA = 44,
    GAME_MODE_INVALID = 255
} game_mode;

/* A game-mode handler: a screen_layer plus a "has this mode finished" query,
 * used only by the mode spawner (not by the generic screen_collection). */
typedef struct {
    screen_layer layer;
    bool (*is_done)(void *impl);
    bool valid;
} mode_handler;

#endif
