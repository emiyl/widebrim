#include "mode_reset.h"

#include <stdlib.h>

/* C port of ResetHelper, minus the logo GdScript playback (scripting is
 * deferred): reset transient state, fade to black, then jump to Title. */
typedef struct {
    game_state *state;
    bool done;
} mode_reset_impl;

static void mode_reset_on_fade_out_done(void *user) {
    mode_reset_impl *impl = (mode_reset_impl *)user;
    game_state_set_mode(impl->state, GAME_MODE_ROOM); game_state_set_place_num(impl->state, 10);
    impl->done = true;
}

static bool mode_reset_is_done(void *impl) {
    return ((mode_reset_impl *)impl)->done;
}

static void mode_reset_destroy(void *impl) {
    free(impl);
}

mode_handler mode_reset_create(game_state *state, screen_controller *controller) {
    mode_handler handler;
    mode_reset_impl *impl = (mode_reset_impl *)malloc(sizeof(mode_reset_impl));

    impl->state = state;
    impl->done = false;
    game_state_reset(state);
    screen_controller_fade_out(controller, FADER_DEFAULT_DURATION_MS, mode_reset_on_fade_out_done, impl);

    handler.layer.impl = impl;
    handler.layer.update = NULL;
    handler.layer.draw = NULL;
    handler.layer.handle_key = NULL;
    handler.layer.handle_touch = NULL;
    handler.layer.on_quit = NULL;
    handler.layer.destroy = mode_reset_destroy;
    handler.is_done = mode_reset_is_done;
    handler.valid = true;
    return handler;
}
