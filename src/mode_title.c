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
    
    game_state_set_place_num(impl->state, 8);
    game_state_set_mode(impl->state, GAME_MODE_ROOM);
    impl->done = true;
    return true;
}

static bool mode_title_handle_key(void *impl, const wb_input_event *event) {
    if (event && event->type == WB_INPUT_EVENT_KEY_DOWN) {
        return mode_title_advance((mode_title_impl *)impl);
    }
    return false;
}

static bool mode_title_handle_touch(void *impl, const wb_input_event *event) {
    if (event && event->type == WB_INPUT_EVENT_MOUSE_BUTTON_DOWN) {
        return mode_title_advance((mode_title_impl *)impl);
    }
    return false;
}

mode_handler mode_title_create(game_state *state, screen_controller *controller) {
    mode_handler handler;
    mode_title_impl *impl = (mode_title_impl *)malloc(sizeof(mode_title_impl));

    impl->state = state;
    impl->done = false;

    char *bg_path, *sub_bg_path;
    switch (state->version) {
        case WB_GAME_LAYTON1:
            bg_path = "bg/start_select2.arc";
            sub_bg_path = "bg/select_title.arc";
            break;
        case WB_GAME_LAYTON2:
            bg_path = "bg/title/title.arc";
            sub_bg_path = "bg/title/title_sub.arc";
            break;
        default:
            bg_path = "";
            sub_bg_path = "";
            break;
    }

    bg_loader_load(state, controller, bg_path, screen_controller_set_bg_main);
    bg_loader_load(state, controller, sub_bg_path, screen_controller_set_bg_sub);
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
