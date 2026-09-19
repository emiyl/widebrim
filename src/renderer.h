#ifndef WIDEBRIM_RENDERER_H
#define WIDEBRIM_RENDERER_H

#include <stdint.h>

#include <SDL3/SDL.h>

typedef struct renderer_texture renderer_texture;
typedef struct renderer renderer;

typedef struct renderer_vtable {
    void (*destroy_texture)(renderer *renderer, renderer_texture *texture);
    renderer_texture *(*create_texture_from_rgba)(renderer *renderer, const uint8_t *rgba, int width, int height);
    void (*draw_texture)(renderer *renderer, const renderer_texture *texture, const SDL_FRect *dst);
    void (*draw_rect)(renderer *renderer, const SDL_FRect *rect, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    void (*fill_rect)(renderer *renderer, const SDL_FRect *rect, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    void (*set_blend_mode)(renderer *renderer, SDL_BlendMode mode);
    void (*set_global_texture_blend_mode)(renderer *renderer, SDL_BlendMode mode);
    void (*clear)(renderer *renderer, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    void (*present)(renderer *renderer);
    void (*get_texture_size)(renderer *renderer, const renderer_texture *texture, int *w, int *h);
    void (*destroy)(renderer *renderer);
} renderer_vtable;

struct renderer {
    void *impl;
    const renderer_vtable *vt;
};

renderer *renderer_create_sdl(SDL_Renderer *sdl_renderer);

static inline void renderer_destroy(renderer *renderer_instance) {
    if (renderer_instance && renderer_instance->vt && renderer_instance->vt->destroy) {
        renderer_instance->vt->destroy(renderer_instance);
    }
}

static inline void renderer_set_blend_mode(renderer *renderer_instance, SDL_BlendMode mode) {
    if (renderer_instance && renderer_instance->vt && renderer_instance->vt->set_blend_mode) {
        renderer_instance->vt->set_blend_mode(renderer_instance, mode);
    }
}

static inline void renderer_set_global_texture_blend_mode(renderer *renderer_instance, SDL_BlendMode mode) {
    if (renderer_instance && renderer_instance->vt && renderer_instance->vt->set_global_texture_blend_mode) {
        renderer_instance->vt->set_global_texture_blend_mode(renderer_instance, mode);
    }
}

static inline void renderer_clear(renderer *renderer_instance, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (renderer_instance && renderer_instance->vt && renderer_instance->vt->clear) {
        renderer_instance->vt->clear(renderer_instance, r, g, b, a);
    }
}

static inline void renderer_present(renderer *renderer_instance) {
    if (renderer_instance && renderer_instance->vt && renderer_instance->vt->present) {
        renderer_instance->vt->present(renderer_instance);
    }
}

static inline void renderer_destroy_texture(renderer *renderer_instance, renderer_texture *texture) {
    if (renderer_instance && renderer_instance->vt && renderer_instance->vt->destroy_texture) {
        renderer_instance->vt->destroy_texture(renderer_instance, texture);
    }
}

static inline renderer_texture *renderer_create_texture_from_rgba(renderer *renderer_instance,
                                                                  const uint8_t *rgba,
                                                                  int width,
                                                                  int height) {
    if (!renderer_instance || !renderer_instance->vt || !renderer_instance->vt->create_texture_from_rgba) {
        return NULL;
    }
    return renderer_instance->vt->create_texture_from_rgba(renderer_instance, rgba, width, height);
}

static inline void renderer_draw_texture(renderer *renderer_instance, const renderer_texture *texture,
                                        const SDL_FRect *dst) {
    if (renderer_instance && renderer_instance->vt && renderer_instance->vt->draw_texture && texture) {
        renderer_instance->vt->draw_texture(renderer_instance, texture, dst);
    }
}

static inline void renderer_draw_rect(renderer *renderer_instance, const SDL_FRect *rect,
                                     uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (renderer_instance && renderer_instance->vt && renderer_instance->vt->draw_rect) {
        renderer_instance->vt->draw_rect(renderer_instance, rect, r, g, b, a);
    }
}

static inline void renderer_fill_rect(renderer *renderer_instance, const SDL_FRect *rect,
                                      uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (renderer_instance && renderer_instance->vt && renderer_instance->vt->fill_rect) {
        renderer_instance->vt->fill_rect(renderer_instance, rect, r, g, b, a);
    }
}

static inline void renderer_get_texture_size(renderer *renderer_instance, const renderer_texture *texture,
                                            int *w, int *h) {
    if (renderer_instance && renderer_instance->vt && renderer_instance->vt->get_texture_size && texture) {
        renderer_instance->vt->get_texture_size(renderer_instance, texture, w, h);
    } else if (w && h) {
        *w = 0;
        *h = 0;
    }
}

#endif
