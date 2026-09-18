#ifndef WIDEBRIM_TEXTURE_UTIL_H
#define WIDEBRIM_TEXTURE_UTIL_H

#include <stdint.h>

#include <SDL3/SDL.h>

SDL_Texture *texture_from_rgba(SDL_Renderer *renderer, const uint8_t *rgba, int width, int height);

#endif
