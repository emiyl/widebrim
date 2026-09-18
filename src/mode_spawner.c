#include "mode_spawner.h"

#include <stdio.h>

#include "mode_reset.h"
#include "mode_room.h"
#include "mode_title.h"
#include "mode_drama_event.h"

static mode_handler mode_spawner_create_handler(game_mode mode, game_state *state, screen_controller *controller) {
    switch (mode) {
        case GAME_MODE_RESET:
            return mode_reset_create(state, controller);
        case GAME_MODE_TITLE:
            return mode_title_create(state, controller);
        case GAME_MODE_ROOM:
            return mode_room_create(state, controller);
        case GAME_MODE_DRAMA_EVENT:
            return mode_drama_event_create(state, controller);
        default: {
            mode_handler invalid;
            invalid.layer.impl = NULL;
            invalid.layer.update = NULL;
            invalid.layer.draw = NULL;
            invalid.layer.handle_key = NULL;
            invalid.layer.handle_touch = NULL;
            invalid.layer.on_quit = NULL;
            invalid.layer.destroy = NULL;
            invalid.is_done = NULL;
            invalid.valid = false;
            return invalid;
        }
    }
}

static void mode_spawner_void_mode(mode_spawner *ms) {
    if (ms->has_active_mode) {
        screen_layer removed = screen_collection_remove_at(&ms->layers, 1);
        if (removed.destroy) {
            removed.destroy(removed.impl);
        }
    }
    ms->has_active_mode = false;
    ms->current_active_mode = GAME_MODE_INVALID;
}

static void mode_spawner_load_mode(mode_spawner *ms, game_mode mode) {
    screen_layer fader_slot;
    mode_handler handler;

    fprintf(stderr, "widebrim: loading mode %d\n", (int)mode);
    game_state_set_mode(ms->state, mode);
    ms->current_active_mode = mode;

    /* Pop the fader so the new mode is inserted beneath it, then reinstall it on top. */
    fader_slot = screen_collection_pop(&ms->layers);

    handler = mode_spawner_create_handler(mode, ms->state, &ms->controller);
    if (handler.valid) {
        screen_collection_add(&ms->layers, handler.layer);
        ms->active_mode = handler;
        ms->has_active_mode = true;
    } else {
        fprintf(stderr, "widebrim: no handler registered for game mode %d\n", (int)mode);
        ms->has_active_mode = false;
    }

    screen_collection_add(&ms->layers, fader_slot);
}

static void mode_spawner_ready_switch(mode_spawner *ms, game_mode target);

static void mode_spawner_on_fade_out_complete(void *user) {
    mode_spawner *ms = (mode_spawner *)user;
    ms->switch_pending = false;
    mode_spawner_ready_switch(ms, ms->pending_target_mode);
}

static void mode_spawner_ready_switch(mode_spawner *ms, game_mode target) {
    if (fader_layer_is_view_obscured(&ms->fader)) {
        ms->switch_pending = false;
        mode_spawner_void_mode(ms);
        mode_spawner_load_mode(ms, target);
    } else if (!ms->switch_pending) {
        ms->switch_pending = true;
        ms->pending_target_mode = target;
        fader_layer_fade_out(&ms->fader, FADER_DEFAULT_DURATION_MS, mode_spawner_on_fade_out_complete, ms);
    }
}

void mode_spawner_init(mode_spawner *ms, game_state *state, SDL_Renderer *renderer) {
    ms->state = state;
    ms->has_active_mode = false;
    ms->current_active_mode = GAME_MODE_INVALID;
    ms->pending_target_mode = GAME_MODE_INVALID;
    ms->switch_pending = false;
    ms->should_quit = false;

    bg_layer_init(&ms->bg, renderer);
    fader_layer_init(&ms->fader);
    ms->controller.bg = &ms->bg;
    ms->controller.fader = &ms->fader;

    screen_collection_init(&ms->layers);
    screen_collection_add(&ms->layers, bg_layer_as_screen_layer(&ms->bg));
    screen_collection_add(&ms->layers, fader_layer_as_screen_layer(&ms->fader));
}

void mode_spawner_destroy(mode_spawner *ms) {
    screen_collection_free(&ms->layers); /* destroys the active mode's impl, if any */
    bg_layer_destroy_state(&ms->bg);
}

void mode_spawner_update(mode_spawner *ms, float dt_ms) {
    bool mode_done;

    screen_collection_update(&ms->layers, dt_ms);

    mode_done = ms->has_active_mode && ms->active_mode.is_done && ms->active_mode.is_done(ms->active_mode.layer.impl);

    if (ms->has_active_mode) {
        if (mode_done) {
            mode_spawner_ready_switch(ms, game_state_get_mode(ms->state));
        }
    } else {
        game_mode current = game_state_get_mode(ms->state);
        if (ms->current_active_mode != current) {
            mode_spawner_ready_switch(ms, current);
        } else if (game_state_get_mode_next(ms->state) != current) {
            mode_spawner_ready_switch(ms, game_state_get_mode_next(ms->state));
        } else {
            ms->should_quit = true;
        }
    }
}

void mode_spawner_draw(mode_spawner *ms, SDL_Renderer *renderer) {
    screen_collection_draw(&ms->layers, renderer);
}

bool mode_spawner_handle_key(mode_spawner *ms, const SDL_Event *event) {
    return screen_collection_handle_key(&ms->layers, event);
}

bool mode_spawner_handle_touch(mode_spawner *ms, const SDL_Event *event) {
    return screen_collection_handle_touch(&ms->layers, event);
}

void mode_spawner_on_quit(mode_spawner *ms) {
    screen_collection_on_quit(&ms->layers);
}
