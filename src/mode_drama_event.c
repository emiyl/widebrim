#include "mode_drama_event.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "bg_loader.h"

typedef struct {
    game_state *state;
    screen_controller *controller;
    bool done;
    int event_id;
} mode_drama_event_impl;

static bool mode_drama_event_is_done(void *impl) {
    return ((mode_drama_event_impl *)impl)->done;
}

static void mode_drama_event_destroy(void *impl) {
    free(impl);
}

static void mode_drama_event_finish(void *user) {
    mode_drama_event_impl *impl = (mode_drama_event_impl *)user;
    game_mode next_mode = game_state_get_mode_next(impl->state);

    if (next_mode != GAME_MODE_INVALID) {
        game_state_set_mode(impl->state, next_mode);
        game_state_set_mode_next(impl->state, GAME_MODE_INVALID);
    } else {
        game_state_set_mode(impl->state, GAME_MODE_ROOM);
    }

    impl->done = true;
}

static bool mode_drama_event_handle_key(void *impl, const SDL_Event *event) {
    mode_drama_event_impl *drama = (mode_drama_event_impl *)impl;
    if (event->type == SDL_EVENT_KEY_DOWN) {
        fprintf(stderr, "widebrim: drama event %d acknowledged by key input; completion stub active\n",
                drama->event_id);
        screen_controller_fade_out(drama->controller, FADER_DEFAULT_DURATION_MS,
                                    mode_drama_event_finish, drama);
        return true;
    }
    return false;
}

static bool mode_drama_event_handle_touch(void *impl, const SDL_Event *event) {
    mode_drama_event_impl *drama = (mode_drama_event_impl *)impl;
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        fprintf(stderr, "widebrim: drama event %d acknowledged by touch input; completion stub active\n",
                drama->event_id);
        screen_controller_fade_out(drama->controller, FADER_DEFAULT_DURATION_MS,
                                    mode_drama_event_finish, drama);
        return true;
    }
    return false;
}

mode_handler mode_drama_event_create(game_state *state, screen_controller *controller) {
    mode_handler handler;
    mode_drama_event_impl *impl = (mode_drama_event_impl *)malloc(sizeof(mode_drama_event_impl));

    impl->state = state;
    impl->controller = controller;
    impl->done = false;
    impl->event_id = game_state_get_event_id(state);

    fprintf(stderr, "widebrim: loading drama event %d\n", impl->event_id);

    /* Minimal placeholder event setup: load a neutral background and fade in. */
    bg_loader_load(state, controller, "data_lt2/bg/title/title.arc", screen_controller_set_bg_main);
    bg_loader_load(state, controller, "data_lt2/bg/title/title_sub.arc", screen_controller_set_bg_sub);
    screen_controller_fade_in(controller, FADER_DEFAULT_DURATION_MS, NULL, NULL);

    handler.layer.impl = impl;
    handler.layer.update = NULL;
    handler.layer.draw = NULL;
    handler.layer.handle_key = mode_drama_event_handle_key;
    handler.layer.handle_touch = mode_drama_event_handle_touch;
    handler.layer.on_quit = NULL;
    handler.layer.destroy = mode_drama_event_destroy;
    handler.is_done = mode_drama_event_is_done;
    handler.valid = true;
    return handler;
}
