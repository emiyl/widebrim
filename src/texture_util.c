#include "texture_util.h"

SDL_Texture *texture_from_rgba(SDL_Renderer *renderer, const uint8_t *rgba, int width, int height) {
    SDL_Texture *tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, width, height);
    if (!tex) {
        return NULL;
    }
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
    if (!SDL_UpdateTexture(tex, NULL, rgba, width * 4)) {
        SDL_DestroyTexture(tex);
        return NULL;
    }
    return tex;
}
