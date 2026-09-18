#ifndef WIDEBRIM_MODE_SPAWNER_H
#define WIDEBRIM_MODE_SPAWNER_H

#include <stdbool.h>

#include <SDL3/SDL.h>

#include "bg_layer.h"
#include "fader_layer.h"
#include "game_state.h"
#include "mode.h"
#include "screen.h"
#include "screen_controller.h"

/* C port of ScreenCollectionGameModeSpawner: owns the Bg -> [active mode] ->
 * Fader layer stack and the fade-out/unload/load/fade-in mode-switch state
 * machine. Only Reset/Title/Room are wired up; any other mode falls back to
 * an "invalid mode" no-op instead of crashing. */
typedef struct {
    screen_collection layers; /* [0]=bg, [1]=active mode (optional), [last]=fader */
    bg_layer bg;
    fader_layer fader;
    screen_controller controller;
    game_state *state;

    bool has_active_mode;
    mode_handler active_mode;
    game_mode current_active_mode;
    game_mode pending_target_mode;
    bool switch_pending; /* a fade-out toward pending_target_mode is already in flight */

    bool should_quit;
} mode_spawner;

void mode_spawner_init(mode_spawner *ms, game_state *state, SDL_Renderer *renderer);
void mode_spawner_destroy(mode_spawner *ms);

void mode_spawner_update(mode_spawner *ms, float dt_ms);
void mode_spawner_draw(mode_spawner *ms, SDL_Renderer *renderer);
bool mode_spawner_handle_key(mode_spawner *ms, const SDL_Event *event);
bool mode_spawner_handle_touch(mode_spawner *ms, const SDL_Event *event);
void mode_spawner_on_quit(mode_spawner *ms);

#endif
