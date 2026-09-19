#include "mode_drama_event.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "bg_loader.h"

#define MODE_DRAMA_EVENT_SCRIPT_MAX_STEPS 16u

enum {
    EVENT_OP_EXIT_SCRIPT = 1,
    EVENT_OP_FADE_IN = 2,
    EVENT_OP_FADE_OUT = 3,
    EVENT_OP_TEXT_WINDOW = 4,
    EVENT_OP_SET_PLACE = 5,
    EVENT_OP_SET_GAME_MODE = 6,
    EVENT_OP_SET_END_GAME_MODE = 7,
    EVENT_OP_SET_MOVIE_NUM = 8,
    EVENT_OP_SET_DRAMA_EVENT_NUM = 9,
    EVENT_OP_SET_AUTO_EVENT_NUM = 10,
    EVENT_OP_SET_PUZZLE_NUM = 11,
    EVENT_OP_LOAD_BG = 33,
    EVENT_OP_LOAD_SUB_BG = 34,
    EVENT_OP_WAIT_FRAME = 49,
    EVENT_OP_FADE_IN_ONLY_MAIN = 50,
    EVENT_OP_FADE_OUT_ONLY_MAIN = 51,
    EVENT_OP_WAIT_INPUT = 105,
    EVENT_OP_SHAKE_BG = 106,
    EVENT_OP_SHAKE_SUB_BG = 107,
    EVENT_OP_WAIT_VSYNC_OR_PEN_TOUCH = 108,
};

typedef struct {
    uint16_t opcode;
    int32_t arg[4];
    size_t argc;
} event_script_step;

typedef struct {
    game_state *state;
    screen_controller *controller;
    bool done;
    int event_id;
    event_script_step script[MODE_DRAMA_EVENT_SCRIPT_MAX_STEPS];
    size_t script_count;
    size_t script_pc;
    bool waiting_for_input;
    bool in_fade;
} mode_drama_event_impl;

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

static void mode_drama_event_load_placeholder_script(mode_drama_event_impl *impl) {
    event_script_step *s = impl->script;
    size_t count = 0u;

    s[count].opcode = EVENT_OP_TEXT_WINDOW;
    s[count].argc = 1u;
    s[count].arg[0] = impl->event_id;
    count += 1u;

    s[count].opcode = EVENT_OP_WAIT_INPUT;
    s[count].argc = 0u;
    count += 1u;

    s[count].opcode = EVENT_OP_FADE_OUT;
    s[count].argc = 0u;
    count += 1u;

    s[count].opcode = EVENT_OP_SET_GAME_MODE;
    s[count].argc = 1u;
    s[count].arg[0] = GAME_MODE_ROOM;
    count += 1u;

    impl->script_count = count;
    impl->script_pc = 0u;
    impl->waiting_for_input = false;
    impl->in_fade = false;
}

static void mode_drama_event_step(mode_drama_event_impl *impl) {
    if (impl->script_pc >= impl->script_count) {
        screen_controller_fade_out(impl->controller, FADER_DEFAULT_DURATION_MS,
                                    mode_drama_event_finish, impl);
        return;
    }

    switch (impl->script[impl->script_pc].opcode) {
        case EVENT_OP_TEXT_WINDOW:
            fprintf(stderr, "widebrim: drama event %d: TextWindow opcode (arg=%d)\n",
                    impl->event_id, impl->script[impl->script_pc].arg[0]);
            impl->waiting_for_input = true;
            impl->script_pc += 1u;
            break;

        case EVENT_OP_WAIT_INPUT:
            fprintf(stderr, "widebrim: drama event %d: WaitInput opcode\n", impl->event_id);
            impl->waiting_for_input = true;
            impl->script_pc += 1u;
            break;

        case EVENT_OP_FADE_OUT:
            fprintf(stderr, "widebrim: drama event %d: FadeOut opcode\n", impl->event_id);
            impl->in_fade = true;
            screen_controller_fade_out(impl->controller, FADER_DEFAULT_DURATION_MS, NULL, NULL);
            impl->script_pc += 1u;
            break;

        case EVENT_OP_SET_GAME_MODE:
            fprintf(stderr, "widebrim: drama event %d: SetGameMode %d\n",
                    impl->event_id, impl->script[impl->script_pc].arg[0]);
            game_state_set_mode(impl->state, (game_mode)impl->script[impl->script_pc].arg[0]);
            impl->script_pc += 1u;
            break;

        case EVENT_OP_SET_END_GAME_MODE:
            fprintf(stderr, "widebrim: drama event %d: SetEndGameMode %d\n",
                    impl->event_id, impl->script[impl->script_pc].arg[0]);
            game_state_set_mode_next(impl->state, (game_mode)impl->script[impl->script_pc].arg[0]);
            impl->script_pc += 1u;
            break;

        case EVENT_OP_SET_PLACE:
            fprintf(stderr, "widebrim: drama event %d: SetPlace %d\n",
                    impl->event_id, impl->script[impl->script_pc].arg[0]);
            game_state_set_place_num(impl->state, impl->script[impl->script_pc].arg[0]);
            impl->script_pc += 1u;
            break;

        case EVENT_OP_SET_DRAMA_EVENT_NUM:
            fprintf(stderr, "widebrim: drama event %d: SetDramaEventNum %d\n",
                    impl->event_id, impl->script[impl->script_pc].arg[0]);
            game_state_set_event_id(impl->state, impl->script[impl->script_pc].arg[0]);
            impl->script_pc += 1u;
            break;

        case EVENT_OP_LOAD_BG:
            fprintf(stderr, "widebrim: drama event %d: LoadBG %d\n",
                    impl->event_id, impl->script[impl->script_pc].arg[0]);
            impl->script_pc += 1u;
            break;

        case EVENT_OP_LOAD_SUB_BG:
            fprintf(stderr, "widebrim: drama event %d: LoadSubBG %d\n",
                    impl->event_id, impl->script[impl->script_pc].arg[0]);
            impl->script_pc += 1u;
            break;

        case EVENT_OP_WAIT_FRAME:
            fprintf(stderr, "widebrim: drama event %d: WaitFrame %d\n",
                    impl->event_id, impl->script[impl->script_pc].arg[0]);
            impl->script_pc += 1u;
            break;

        case EVENT_OP_EXIT_SCRIPT:
            fprintf(stderr, "widebrim: drama event %d: ExitScript\n", impl->event_id);
            impl->script_pc = impl->script_count;
            screen_controller_fade_out(impl->controller, FADER_DEFAULT_DURATION_MS,
                                        mode_drama_event_finish, impl);
            break;

        default:
            fprintf(stderr, "widebrim: drama event %d: unhandled opcode %u\n",
                    impl->event_id, (unsigned)impl->script[impl->script_pc].opcode);
            impl->script_pc += 1u;
            break;
    }
}

static bool mode_drama_event_is_done(void *impl) {
    return ((mode_drama_event_impl *)impl)->done;
}

static void mode_drama_event_destroy(void *impl) {
    free(impl);
}

static void mode_drama_event_update(void *implp, float dt_ms) {
    mode_drama_event_impl *impl = (mode_drama_event_impl *)implp;
    (void)dt_ms;

    if (impl->done) {
        return;
    }

    if (!impl->waiting_for_input && !impl->in_fade && impl->script_pc < impl->script_count) {
        mode_drama_event_step(impl);
    }
}

static bool mode_drama_event_handle_key(void *impl, const wb_input_event *event) {
    mode_drama_event_impl *drama = (mode_drama_event_impl *)impl;
    if (event && event->type == WB_INPUT_EVENT_KEY_DOWN) {
        if (drama->waiting_for_input) {
            drama->waiting_for_input = false;
            mode_drama_event_step(drama);
            return true;
        }
        fprintf(stderr, "widebrim: drama event %d acknowledged by key input; advancing script\n",
                drama->event_id);
        mode_drama_event_step(drama);
        return true;
    }
    return false;
}

static bool mode_drama_event_handle_touch(void *impl, const wb_input_event *event) {
    mode_drama_event_impl *drama = (mode_drama_event_impl *)impl;
    if (event && event->type == WB_INPUT_EVENT_MOUSE_BUTTON_DOWN) {
        if (drama->waiting_for_input) {
            drama->waiting_for_input = false;
            mode_drama_event_step(drama);
            return true;
        }
        fprintf(stderr, "widebrim: drama event %d acknowledged by touch input; advancing script\n",
                drama->event_id);
        mode_drama_event_step(drama);
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
    impl->script_count = 0u;
    impl->script_pc = 0u;
    impl->waiting_for_input = false;
    impl->in_fade = false;

    fprintf(stderr, "widebrim: loading drama event %d\n", impl->event_id);
    mode_drama_event_load_placeholder_script(impl);

    bg_loader_load(state, controller, "bg/title/title.arc", screen_controller_set_bg_main);
    bg_loader_load(state, controller, "bg/title/title_sub.arc", screen_controller_set_bg_sub);
    screen_controller_fade_in(controller, FADER_DEFAULT_DURATION_MS, NULL, NULL);

    handler.layer.impl = impl;
    handler.layer.update = mode_drama_event_update;
    handler.layer.draw = NULL;
    handler.layer.handle_key = mode_drama_event_handle_key;
    handler.layer.handle_touch = mode_drama_event_handle_touch;
    handler.layer.on_quit = NULL;
    handler.layer.destroy = mode_drama_event_destroy;
    handler.is_done = mode_drama_event_is_done;
    handler.valid = true;
    return handler;
}
