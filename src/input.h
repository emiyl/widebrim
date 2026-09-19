#ifndef WIDEBRIM_INPUT_H
#define WIDEBRIM_INPUT_H

#include <stdbool.h>

#include <SDL3/SDL.h>

typedef struct input input;

typedef struct input_vtable {
    void (*destroy)(input *input_instance);
    bool (*poll_event)(input *input_instance, SDL_Event *event);
} input_vtable;

struct input {
    void *impl;
    const input_vtable *vt;
};

input *input_create_sdl(void);

static inline void input_destroy(input *input_instance) {
    if (input_instance && input_instance->vt && input_instance->vt->destroy) {
        input_instance->vt->destroy(input_instance);
    }
}

static inline bool input_poll_event(input *input_instance, SDL_Event *event) {
    if (input_instance && input_instance->vt && input_instance->vt->poll_event) {
        return input_instance->vt->poll_event(input_instance, event);
    }
    return false;
}

#endif
