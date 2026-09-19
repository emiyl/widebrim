#include "../renderer.h"

#include <stdlib.h>

typedef struct sdl_renderer_impl {
    SDL_Renderer *renderer;
    SDL_BlendMode global_texture_blend_mode;
    SDL_Texture **texture_registry;
    size_t texture_registry_count;
    size_t texture_registry_capacity;
} sdl_renderer_impl;

struct renderer_texture {
    SDL_Texture *texture;
};

static void sdl_renderer_register_texture(sdl_renderer_impl *impl, SDL_Texture *tex) {
    SDL_Texture **grown;
    size_t new_capacity;

    if (!tex) {
        return;
    }
    if (impl->texture_registry_count == impl->texture_registry_capacity) {
        new_capacity = impl->texture_registry_capacity == 0u ? 16u : impl->texture_registry_capacity * 2u;
        grown = (SDL_Texture **)realloc(impl->texture_registry, new_capacity * sizeof(*grown));
        if (!grown) {
            return;
        }
        impl->texture_registry = grown;
        impl->texture_registry_capacity = new_capacity;
    }
    impl->texture_registry[impl->texture_registry_count++] = tex;
}

static void sdl_renderer_destroy_texture(renderer *renderer_instance, renderer_texture *texture) {
    (void)renderer_instance;
    if (!texture) {
        return;
    }
    if (texture->texture) {
        SDL_DestroyTexture(texture->texture);
        texture->texture = NULL;
    }
    free(texture);
}

static renderer_texture *sdl_renderer_create_texture_from_rgba(renderer *renderer_instance,
                                                               const uint8_t *rgba,
                                                               int width,
                                                               int height) {
    sdl_renderer_impl *impl = (sdl_renderer_impl *)renderer_instance->impl;
    renderer_texture *texture = (renderer_texture *)calloc(1u, sizeof(*texture));
    SDL_Texture *sdl_texture;

    if (!texture) {
        return NULL;
    }

    sdl_texture = SDL_CreateTexture(impl->renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, width, height);
    if (!sdl_texture) {
        free(texture);
        return NULL;
    }

    SDL_SetTextureBlendMode(sdl_texture, impl->global_texture_blend_mode);
    SDL_SetTextureScaleMode(sdl_texture, SDL_SCALEMODE_NEAREST);
    if (!SDL_UpdateTexture(sdl_texture, NULL, rgba, width * 4)) {
        SDL_DestroyTexture(sdl_texture);
        free(texture);
        return NULL;
    }

    sdl_renderer_register_texture(impl, sdl_texture);
    texture->texture = sdl_texture;
    return texture;
}

static void sdl_renderer_draw_texture(renderer *renderer_instance, const renderer_texture *texture, const SDL_FRect *dst) {
    sdl_renderer_impl *impl = (sdl_renderer_impl *)renderer_instance->impl;
    if (!texture || !texture->texture || !dst) {
        return;
    }
    SDL_RenderTexture(impl->renderer, texture->texture, NULL, dst);
}

static void sdl_renderer_draw_rect(renderer *renderer_instance, const SDL_FRect *rect,
                                  uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    sdl_renderer_impl *impl = (sdl_renderer_impl *)renderer_instance->impl;
    if (!rect) {
        return;
    }
    SDL_SetRenderDrawColor(impl->renderer, r, g, b, a);
    SDL_RenderRect(impl->renderer, rect);
}

static void sdl_renderer_fill_rect(renderer *renderer_instance, const SDL_FRect *rect,
                                   uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    sdl_renderer_impl *impl = (sdl_renderer_impl *)renderer_instance->impl;
    if (!rect) {
        return;
    }
    SDL_SetRenderDrawColor(impl->renderer, r, g, b, a);
    SDL_RenderFillRect(impl->renderer, rect);
}

static void sdl_renderer_set_blend_mode(renderer *renderer_instance, SDL_BlendMode mode) {
    sdl_renderer_impl *impl = (sdl_renderer_impl *)renderer_instance->impl;
    SDL_SetRenderDrawBlendMode(impl->renderer, mode);
}

static void sdl_renderer_set_global_texture_blend_mode(renderer *renderer_instance, SDL_BlendMode mode) {
    sdl_renderer_impl *impl = (sdl_renderer_impl *)renderer_instance->impl;
    size_t i;

    impl->global_texture_blend_mode = mode;
    for (i = 0; i < impl->texture_registry_count; ++i) {
        SDL_SetTextureBlendMode(impl->texture_registry[i], mode);
    }
}

static void sdl_renderer_clear(renderer *renderer_instance, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    sdl_renderer_impl *impl = (sdl_renderer_impl *)renderer_instance->impl;
    SDL_SetRenderDrawColor(impl->renderer, r, g, b, a);
    SDL_RenderClear(impl->renderer);
}

static void sdl_renderer_present(renderer *renderer_instance) {
    sdl_renderer_impl *impl = (sdl_renderer_impl *)renderer_instance->impl;
    SDL_RenderPresent(impl->renderer);
}

static void sdl_renderer_get_texture_size(renderer *renderer_instance, const renderer_texture *texture,
                                          int *w, int *h) {
    sdl_renderer_impl *impl = (sdl_renderer_impl *)renderer_instance->impl;
    float fw = 0.0f;
    float fh = 0.0f;

    if (w) {
        *w = 0;
    }
    if (h) {
        *h = 0;
    }
    if (!texture || !texture->texture) {
        return;
    }
    if (SDL_GetTextureSize(texture->texture, &fw, &fh) == 0 && fw > 0.0f && fh > 0.0f) {
        if (w) {
            *w = (int)fw;
        }
        if (h) {
            *h = (int)fh;
        }
    }
    (void)impl;
}

static void sdl_renderer_destroy(renderer *renderer_instance) {
    sdl_renderer_impl *impl = (sdl_renderer_impl *)renderer_instance->impl;
    size_t i;

    for (i = 0; i < impl->texture_registry_count; ++i) {
        if (impl->texture_registry[i]) {
            SDL_DestroyTexture(impl->texture_registry[i]);
        }
    }
    free(impl->texture_registry);
    free(impl);
    free(renderer_instance);
}

static const renderer_vtable g_sdl_renderer_vtable = {
    .destroy_texture = sdl_renderer_destroy_texture,
    .create_texture_from_rgba = sdl_renderer_create_texture_from_rgba,
    .draw_texture = sdl_renderer_draw_texture,
    .draw_rect = sdl_renderer_draw_rect,
    .fill_rect = sdl_renderer_fill_rect,
    .set_blend_mode = sdl_renderer_set_blend_mode,
    .set_global_texture_blend_mode = sdl_renderer_set_global_texture_blend_mode,
    .clear = sdl_renderer_clear,
    .present = sdl_renderer_present,
    .get_texture_size = sdl_renderer_get_texture_size,
    .destroy = sdl_renderer_destroy,
};

renderer *renderer_create_sdl(SDL_Renderer *sdl_renderer) {
    renderer *renderer_instance = (renderer *)calloc(1u, sizeof(*renderer_instance));
    sdl_renderer_impl *impl = (sdl_renderer_impl *)calloc(1u, sizeof(*impl));

    if (!renderer_instance || !impl) {
        free(renderer_instance);
        free(impl);
        return NULL;
    }

    impl->renderer = sdl_renderer;
    impl->global_texture_blend_mode = SDL_BLENDMODE_BLEND;
    impl->texture_registry = NULL;
    impl->texture_registry_count = 0u;
    impl->texture_registry_capacity = 0u;
    renderer_instance->impl = impl;
    renderer_instance->vt = &g_sdl_renderer_vtable;
    return renderer_instance;
}
