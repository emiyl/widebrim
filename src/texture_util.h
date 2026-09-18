#ifndef WIDEBRIM_TEXTURE_UTIL_H
#define WIDEBRIM_TEXTURE_UTIL_H

#include <stdint.h>

#include <SDL3/SDL.h>

/* Uploads an RGBA8888 buffer as a blend-enabled SDL_Texture. Returns NULL on failure. */
SDL_Texture *texture_from_rgba(SDL_Renderer *renderer, const uint8_t *rgba, int width, int height);

#endif
