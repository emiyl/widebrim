#include "../input.h"

#include <SDL3/SDL.h>

#include <stdlib.h>
#include <string.h>

typedef struct sdl_input_impl {
    bool unused;
} sdl_input_impl;

static void sdl_input_event_from_sdl(const SDL_Event *sdl_event, wb_input_event *event) {
    if (!event) {
        return;
    }
    memset(event, 0, sizeof(*event));

    switch (sdl_event->type) {
        case SDL_EVENT_QUIT:
            event->type = WB_INPUT_EVENT_QUIT;
            break;
        case SDL_EVENT_KEY_DOWN:
            event->type = WB_INPUT_EVENT_KEY_DOWN;
            event->data.key.key = (wb_key)sdl_event->key.key;
            break;
        case SDL_EVENT_KEY_UP:
            event->type = WB_INPUT_EVENT_KEY_UP;
            event->data.key.key = (wb_key)sdl_event->key.key;
            break;
        case SDL_EVENT_MOUSE_MOTION:
            event->type = WB_INPUT_EVENT_MOUSE_MOTION;
            event->data.mouse_motion.x = (int)sdl_event->motion.x;
            event->data.mouse_motion.y = (int)sdl_event->motion.y;
            event->data.mouse_motion.dx = (int)sdl_event->motion.xrel;
            event->data.mouse_motion.dy = (int)sdl_event->motion.yrel;
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            event->type = WB_INPUT_EVENT_MOUSE_BUTTON_DOWN;
            event->data.mouse_button.x = (int)sdl_event->button.x;
            event->data.mouse_button.y = (int)sdl_event->button.y;
            event->data.mouse_button.button = (int)sdl_event->button.button;
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            event->type = WB_INPUT_EVENT_MOUSE_BUTTON_UP;
            event->data.mouse_button.x = (int)sdl_event->button.x;
            event->data.mouse_button.y = (int)sdl_event->button.y;
            event->data.mouse_button.button = (int)sdl_event->button.button;
            break;
        default:
            if (sdl_event->type >= SDL_EVENT_USER) {
                event->type = WB_INPUT_EVENT_CUSTOM;
                event->data.custom.type = sdl_event->type;
            } else {
                event->type = WB_INPUT_EVENT_NONE;
            }
            break;
    }
}

static void sdl_input_destroy(input *input_instance) {
    free(input_instance->impl);
    free(input_instance);
}

static bool sdl_input_poll_event(input *input_instance, wb_input_event *event) {
    SDL_Event sdl_event;

    (void)input_instance;
    if (!SDL_PollEvent(&sdl_event)) {
        return false;
    }
    sdl_input_event_from_sdl(&sdl_event, event);
    return true;
}

static const input_vtable g_sdl_input_vtable = {
    .destroy = sdl_input_destroy,
    .poll_event = sdl_input_poll_event,
};

input *input_create_sdl(void) {
    input *input_instance = (input *)calloc(1u, sizeof(*input_instance));
    sdl_input_impl *impl = (sdl_input_impl *)calloc(1u, sizeof(*impl));

    if (!input_instance || !impl) {
        free(input_instance);
        free(impl);
        return NULL;
    }

    input_instance->impl = impl;
    input_instance->vt = &g_sdl_input_vtable;
    return input_instance;
}
