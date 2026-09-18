#include "bg_layer.h"

#include <stdlib.h>

#include "texture_util.h"

void bg_layer_init(bg_layer *bg, SDL_Renderer *renderer) {
    bg->renderer = renderer;
    bg->tex_main = NULL;
    bg->tex_sub = NULL;
    bg->darkness_main = 0;
    bg->darkness_sub = 0;
    bg->shake_main_remaining_ms = 0.0f;
    bg->shake_sub_remaining_ms = 0.0f;
}

void bg_layer_destroy_state(bg_layer *bg) {
    if (bg->tex_main) {
        SDL_DestroyTexture(bg->tex_main);
        bg->tex_main = NULL;
    }
    if (bg->tex_sub) {
        SDL_DestroyTexture(bg->tex_sub);
        bg->tex_sub = NULL;
    }
}

void bg_layer_set_main_rgba(bg_layer *bg, const uint8_t *rgba, int width, int height) {
    if (bg->tex_main) {
        SDL_DestroyTexture(bg->tex_main);
    }
    bg->tex_main = texture_from_rgba(bg->renderer, rgba, width, height);
}

void bg_layer_set_sub_rgba(bg_layer *bg, const uint8_t *rgba, int width, int height) {
    if (bg->tex_sub) {
        SDL_DestroyTexture(bg->tex_sub);
    }
    bg->tex_sub = texture_from_rgba(bg->renderer, rgba, width, height);
}

void bg_layer_modify_palette_main(bg_layer *bg, uint8_t darkness) {
    bg->darkness_main = darkness;
}

void bg_layer_modify_palette_sub(bg_layer *bg, uint8_t darkness) {
    bg->darkness_sub = darkness;
}

void bg_layer_shake_main(bg_layer *bg, float duration_ms) {
    bg->shake_main_remaining_ms = duration_ms;
}

void bg_layer_shake_sub(bg_layer *bg, float duration_ms) {
    bg->shake_sub_remaining_ms = duration_ms;
}

static void bg_layer_update_impl(void *impl, float dt_ms) {
    bg_layer *bg = (bg_layer *)impl;
    if (bg->shake_main_remaining_ms > 0.0f) {
        bg->shake_main_remaining_ms -= dt_ms;
    }
    if (bg->shake_sub_remaining_ms > 0.0f) {
        bg->shake_sub_remaining_ms -= dt_ms;
    }
}

/* Matches BgLayer.draw: sub screen at (0,0), main screen at (0, WIDEBRIM_SCREEN_HEIGHT). */
static void bg_layer_draw_one(SDL_Renderer *renderer, SDL_Texture *tex, int y_offset, float shake_remaining_ms, uint8_t darkness) {
    SDL_FRect dst;
    int shake_x = 0, shake_y = 0;

    dst.x = 0.0f;
    dst.y = (float)y_offset;
    dst.w = (float)WIDEBRIM_SCREEN_WIDTH;
    dst.h = (float)WIDEBRIM_SCREEN_HEIGHT;

    if (shake_remaining_ms > 0.0f) {
        shake_x = (SDL_rand(5) - 2);
        shake_y = (SDL_rand(5) - 2);
        dst.x += (float)shake_x;
        dst.y += (float)shake_y;
    }

    if (tex) {
        SDL_RenderTexture(renderer, tex, NULL, &dst);
    }

    if (darkness > 0) {
        SDL_FRect overlay;
        overlay.x = 0.0f;
        overlay.y = (float)y_offset;
        overlay.w = (float)WIDEBRIM_SCREEN_WIDTH;
        overlay.h = (float)WIDEBRIM_SCREEN_HEIGHT;
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, darkness);
        SDL_RenderFillRect(renderer, &overlay);
    }
}

static void bg_layer_draw_impl(void *impl, SDL_Renderer *renderer) {
    bg_layer *bg = (bg_layer *)impl;
    bg_layer_draw_one(renderer, bg->tex_sub, 0, bg->shake_sub_remaining_ms, bg->darkness_sub);
    bg_layer_draw_one(renderer, bg->tex_main, WIDEBRIM_SCREEN_HEIGHT, bg->shake_main_remaining_ms, bg->darkness_main);
}

screen_layer bg_layer_as_screen_layer(bg_layer *bg) {
    screen_layer layer;
    layer.impl = bg;
    layer.update = bg_layer_update_impl;
    layer.draw = bg_layer_draw_impl;
    layer.handle_key = NULL;
    layer.handle_touch = NULL;
    layer.on_quit = NULL;
    layer.destroy = NULL; /* bg_layer is owned by mode_spawner, not heap-allocated here */
    return layer;
}
