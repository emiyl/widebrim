#include "../input.h"

#include <stdlib.h>

typedef struct sdl_input_impl {
    bool unused;
} sdl_input_impl;

static void sdl_input_destroy(input *input_instance) {
    free(input_instance->impl);
    free(input_instance);
}

static bool sdl_input_poll_event(input *input_instance, SDL_Event *event) {
    (void)input_instance;
    return SDL_PollEvent(event);
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
