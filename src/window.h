#ifndef WIDEBRIM_WINDOW_H
#define WIDEBRIM_WINDOW_H

#include <SDL3/SDL.h>

typedef struct window window;

typedef struct window_vtable {
    void (*destroy)(window *window_instance);
    void (*set_logical_presentation)(window *window_instance, int w, int h, SDL_RendererLogicalPresentation mode);
    void (*set_scale)(window *window_instance, float x_scale, float y_scale);
    void (*set_default_texture_scale_mode)(window *window_instance, SDL_ScaleMode mode);
    void (*set_draw_blend_mode)(window *window_instance, SDL_BlendMode mode);
    void (*convert_event_to_render_coordinates)(window *window_instance, SDL_Event *event);
    SDL_Window *(*as_sdl_window)(const window *window_instance);
    SDL_Renderer *(*as_sdl_renderer)(const window *window_instance);
} window_vtable;

struct window {
    void *impl;
    const window_vtable *vt;
};

window *window_create_sdl(const char *title, int width, int height, Uint32 flags);

static inline void window_destroy(window *window_instance) {
    if (window_instance && window_instance->vt && window_instance->vt->destroy) {
        window_instance->vt->destroy(window_instance);
    }
}

static inline void window_set_logical_presentation(window *window_instance,
                                                  int w,
                                                  int h,
                                                  SDL_RendererLogicalPresentation mode) {
    if (window_instance && window_instance->vt && window_instance->vt->set_logical_presentation) {
        window_instance->vt->set_logical_presentation(window_instance, w, h, mode);
    }
}

static inline void window_set_scale(window *window_instance, float x_scale, float y_scale) {
    if (window_instance && window_instance->vt && window_instance->vt->set_scale) {
        window_instance->vt->set_scale(window_instance, x_scale, y_scale);
    }
}

static inline void window_set_default_texture_scale_mode(window *window_instance, SDL_ScaleMode mode) {
    if (window_instance && window_instance->vt && window_instance->vt->set_default_texture_scale_mode) {
        window_instance->vt->set_default_texture_scale_mode(window_instance, mode);
    }
}

static inline void window_set_draw_blend_mode(window *window_instance, SDL_BlendMode mode) {
    if (window_instance && window_instance->vt && window_instance->vt->set_draw_blend_mode) {
        window_instance->vt->set_draw_blend_mode(window_instance, mode);
    }
}

static inline void window_convert_event_to_render_coordinates(window *window_instance, SDL_Event *event) {
    if (window_instance && window_instance->vt && window_instance->vt->convert_event_to_render_coordinates && event) {
        window_instance->vt->convert_event_to_render_coordinates(window_instance, event);
    }
}

static inline SDL_Window *window_get_sdl_window(const window *window_instance) {
    if (window_instance && window_instance->vt && window_instance->vt->as_sdl_window) {
        return window_instance->vt->as_sdl_window(window_instance);
    }
    return NULL;
}

static inline SDL_Renderer *window_get_sdl_renderer(const window *window_instance) {
    if (window_instance && window_instance->vt && window_instance->vt->as_sdl_renderer) {
        return window_instance->vt->as_sdl_renderer(window_instance);
    }
    return NULL;
}

#endif
