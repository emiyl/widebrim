#ifndef WIDEBRIM_BG_LAYER_H
#define WIDEBRIM_BG_LAYER_H

#include <stdint.h>

#include <SDL3/SDL.h>

#include "screen.h"

#define WIDEBRIM_SCREEN_WIDTH 256
#define WIDEBRIM_SCREEN_HEIGHT 192

/* C port of BgLayer: owns the main (top) and sub (bottom) background
 * textures, palette darkening overlay and screen-shake offsets. */
typedef struct {
    SDL_Renderer *renderer;
    SDL_Texture *tex_main;
    SDL_Texture *tex_sub;
    uint8_t darkness_main;
    uint8_t darkness_sub;
    float shake_main_remaining_ms;
    float shake_sub_remaining_ms;
} bg_layer;

void bg_layer_init(bg_layer *bg, SDL_Renderer *renderer);
void bg_layer_destroy_state(bg_layer *bg);

void bg_layer_set_main_rgba(bg_layer *bg, const uint8_t *rgba, int width, int height);
void bg_layer_set_sub_rgba(bg_layer *bg, const uint8_t *rgba, int width, int height);

void bg_layer_modify_palette_main(bg_layer *bg, uint8_t darkness);
void bg_layer_modify_palette_sub(bg_layer *bg, uint8_t darkness);

void bg_layer_shake_main(bg_layer *bg, float duration_ms);
void bg_layer_shake_sub(bg_layer *bg, float duration_ms);

/* Wraps this bg_layer as a screen_layer for insertion into a screen_collection. */
screen_layer bg_layer_as_screen_layer(bg_layer *bg);

#endif
