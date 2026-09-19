#ifndef WIDEBRIM_WINDOW_H
#define WIDEBRIM_WINDOW_H

#include <stddef.h>

#include "input.h"
#include "renderer.h"

typedef struct window window;

typedef enum {
    WB_LOGICAL_PRESENTATION_DISABLED = 0,
    WB_LOGICAL_PRESENTATION_STRETCH = 1,
    WB_LOGICAL_PRESENTATION_LETTERBOX = 2,
    WB_LOGICAL_PRESENTATION_OVERSCAN = 3,
    WB_LOGICAL_PRESENTATION_INTEGER_SCALE = 4,
} wb_logical_presentation;

typedef enum {
    WB_SCALE_MODE_NEAREST = 0,
} wb_scale_mode;

typedef struct window_vtable {
    void (*destroy)(window *window_instance);
    void (*set_logical_presentation)(window *window_instance, int w, int h, wb_logical_presentation mode);
    void (*set_scale)(window *window_instance, float x_scale, float y_scale);
    void (*set_default_texture_scale_mode)(window *window_instance, wb_scale_mode mode);
    void (*set_draw_blend_mode)(window *window_instance, wb_blend_mode mode);
    void (*convert_event_to_render_coordinates)(window *window_instance, wb_input_event *event);
    void *(*as_native_window)(const window *window_instance);
    void *(*as_native_renderer)(const window *window_instance);
} window_vtable;

struct window {
    void *impl;
    const window_vtable *vt;
};

window *window_create_sdl(const char *title, int width, int height, unsigned int flags);

static inline void window_destroy(window *window_instance) {
    if (window_instance && window_instance->vt && window_instance->vt->destroy) {
        window_instance->vt->destroy(window_instance);
    }
}

static inline void window_set_logical_presentation(window *window_instance,
                                                  int w,
                                                  int h,
                                                  wb_logical_presentation mode) {
    if (window_instance && window_instance->vt && window_instance->vt->set_logical_presentation) {
        window_instance->vt->set_logical_presentation(window_instance, w, h, mode);
    }
}

static inline void window_set_scale(window *window_instance, float x_scale, float y_scale) {
    if (window_instance && window_instance->vt && window_instance->vt->set_scale) {
        window_instance->vt->set_scale(window_instance, x_scale, y_scale);
    }
}

static inline void window_set_default_texture_scale_mode(window *window_instance, wb_scale_mode mode) {
    if (window_instance && window_instance->vt && window_instance->vt->set_default_texture_scale_mode) {
        window_instance->vt->set_default_texture_scale_mode(window_instance, mode);
    }
}

static inline void window_set_draw_blend_mode(window *window_instance, wb_blend_mode mode) {
    if (window_instance && window_instance->vt && window_instance->vt->set_draw_blend_mode) {
        window_instance->vt->set_draw_blend_mode(window_instance, mode);
    }
}

static inline void window_convert_event_to_render_coordinates(window *window_instance, wb_input_event *event) {
    if (window_instance && window_instance->vt && window_instance->vt->convert_event_to_render_coordinates && event) {
        window_instance->vt->convert_event_to_render_coordinates(window_instance, event);
    }
}

static inline void *window_get_native_window(const window *window_instance) {
    if (window_instance && window_instance->vt && window_instance->vt->as_native_window) {
        return window_instance->vt->as_native_window(window_instance);
    }
    return NULL;
}

static inline void *window_get_native_renderer(const window *window_instance) {
    if (window_instance && window_instance->vt && window_instance->vt->as_native_renderer) {
        return window_instance->vt->as_native_renderer(window_instance);
    }
    return NULL;
}

#endif
