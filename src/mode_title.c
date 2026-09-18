#include "mode_title.h"

#include <stdlib.h>

#include "bg_loader.h"

// simplified title player: shows the title background and advances straight
// to Room on any input (new game / continue menu logic is deferred)
typedef struct {
    game_state *state;
    bool done;
} mode_title_impl;

static bool mode_title_is_done(void *impl) {
    return ((mode_title_impl *)impl)->done;
}

static void mode_title_destroy(void *impl) {
    free(impl);
}

static bool mode_title_advance(mode_title_impl *impl) {
    if (impl->done) {
        return false;
    }
    
    game_state_set_place_num(impl->state, 10);
    game_state_set_mode(impl->state, GAME_MODE_ROOM);
    impl->done = true;
    return true;
}

static bool mode_title_handle_key(void *impl, const SDL_Event *event) {
    if (event->type == SDL_EVENT_KEY_DOWN) {
        return mode_title_advance((mode_title_impl *)impl);
    }
    return false;
}

static bool mode_title_handle_touch(void *impl, const SDL_Event *event) {
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        return mode_title_advance((mode_title_impl *)impl);
    }
    return false;
}

mode_handler mode_title_create(game_state *state, screen_controller *controller) {
    mode_handler handler;
    mode_title_impl *impl = (mode_title_impl *)malloc(sizeof(mode_title_impl));

    impl->state = state;
    impl->done = false;

    bg_loader_load(state, controller, "bg/title/title.arc", screen_controller_set_bg_main);
    bg_loader_load(state, controller, "bg/title/title_sub.arc", screen_controller_set_bg_sub);
    screen_controller_fade_in(controller, FADER_DEFAULT_DURATION_MS, NULL, NULL);

    handler.layer.impl = impl;
    handler.layer.update = NULL;
    handler.layer.draw = NULL;
    handler.layer.handle_key = mode_title_handle_key;
    handler.layer.handle_touch = mode_title_handle_touch;
    handler.layer.on_quit = NULL;
    handler.layer.destroy = mode_title_destroy;
    handler.is_done = mode_title_is_done;
    handler.valid = true;
    return handler;
}
