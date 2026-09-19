#ifndef WIDEBRIM_MODE_SPAWNER_H
#define WIDEBRIM_MODE_SPAWNER_H

#include <stdbool.h>

#include <SDL3/SDL.h>

#include "bg_layer.h"
#include "fader_layer.h"
#include "game_state.h"
#include "mode.h"
#include "renderer.h"
#include "screen.h"
#include "screen_controller.h"

typedef struct {
    screen_collection layers;
    bg_layer bg;
    fader_layer fader;
    screen_controller controller;
    game_state *state;

    bool has_active_mode;
    mode_handler active_mode;
    game_mode current_active_mode;
    game_mode pending_target_mode;
    bool switch_pending;

    bool should_quit;
} mode_spawner;

void mode_spawner_init(mode_spawner *ms, game_state *state, SDL_Renderer *renderer);
void mode_spawner_destroy(mode_spawner *ms);

void mode_spawner_update(mode_spawner *ms, float dt_ms);
void mode_spawner_draw(mode_spawner *ms, renderer *renderer_instance);
bool mode_spawner_handle_key(mode_spawner *ms, const SDL_Event *event);
bool mode_spawner_handle_touch(mode_spawner *ms, const SDL_Event *event);
void mode_spawner_on_quit(mode_spawner *ms);

#endif
