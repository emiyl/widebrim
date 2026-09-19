#ifndef WIDEBRIM_INPUT_H
#define WIDEBRIM_INPUT_H

#include <stdbool.h>

typedef enum {
    WB_INPUT_EVENT_NONE = 0,
    WB_INPUT_EVENT_QUIT,
    WB_INPUT_EVENT_KEY_DOWN,
    WB_INPUT_EVENT_KEY_UP,
    WB_INPUT_EVENT_MOUSE_MOTION,
    WB_INPUT_EVENT_MOUSE_BUTTON_DOWN,
    WB_INPUT_EVENT_MOUSE_BUTTON_UP,
    WB_INPUT_EVENT_CUSTOM,
} wb_input_event_type;

typedef enum {
    WB_KEY_TAB = 9,
    WB_KEY_M = 109,
} wb_key;

typedef struct {
    int x;
    int y;
    int dx;
    int dy;
} wb_input_mouse_motion;

typedef struct {
    int x;
    int y;
    int button;
} wb_input_mouse_button;

typedef struct {
    wb_key key;
} wb_input_key;

typedef struct {
    int type;
} wb_input_custom_event;

typedef struct {
    wb_input_event_type type;
    union {
        wb_input_key key;
        wb_input_mouse_motion mouse_motion;
        wb_input_mouse_button mouse_button;
        wb_input_custom_event custom;
    } data;
} wb_input_event;

typedef struct input input;

typedef struct input_vtable {
    void (*destroy)(input *input_instance);
    bool (*poll_event)(input *input_instance, wb_input_event *event);
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

static inline bool input_poll_event(input *input_instance, wb_input_event *event) {
    if (input_instance && input_instance->vt && input_instance->vt->poll_event) {
        return input_instance->vt->poll_event(input_instance, event);
    }
    return false;
}

#endif
