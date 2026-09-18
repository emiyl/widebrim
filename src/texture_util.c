#include "texture_util.h"

#include <stdlib.h>

static SDL_BlendMode g_texture_blend_mode = SDL_BLENDMODE_BLEND;
static SDL_Texture **g_texture_registry = NULL;
static size_t g_texture_registry_count = 0u;
static size_t g_texture_registry_capacity = 0u;

static void texture_register(SDL_Texture *tex) {
    SDL_Texture **grown;
    size_t new_capacity;

    if (!tex) {
        return;
    }
    if (g_texture_registry_count == g_texture_registry_capacity) {
        new_capacity = g_texture_registry_capacity == 0u ? 16u : g_texture_registry_capacity * 2u;
        grown = (SDL_Texture **)realloc(g_texture_registry, new_capacity * sizeof(*grown));
        if (!grown) {
            return;
        }
        g_texture_registry = grown;
        g_texture_registry_capacity = new_capacity;
    }
    g_texture_registry[g_texture_registry_count++] = tex;
}

void texture_set_global_blend_mode(SDL_BlendMode mode) {
    size_t i;

    g_texture_blend_mode = mode;
    for (i = 0; i < g_texture_registry_count; ++i) {
        SDL_SetTextureBlendMode(g_texture_registry[i], mode);
    }
}

SDL_Texture *texture_from_rgba(SDL_Renderer *renderer, const uint8_t *rgba, int width, int height) {
    SDL_Texture *tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, width, height);
    if (!tex) {
        return NULL;
    }
    texture_register(tex);
    SDL_SetTextureBlendMode(tex, g_texture_blend_mode);
    SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
    if (!SDL_UpdateTexture(tex, NULL, rgba, width * 4)) {
        SDL_DestroyTexture(tex);
        return NULL;
    }
    return tex;
}
