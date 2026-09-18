#include "engine.h"

#include <stdio.h>

static void widebrim_draw_screen_region(SDL_Renderer *renderer,
                                       int x,
                                       int y,
                                       int w,
                                       int h,
                                       Uint8 r,
                                       Uint8 g,
                                       Uint8 b,
                                       Uint8 a) {
    SDL_FRect rect = { (float)x, (float)y, (float)w, (float)h };
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    SDL_RenderFillRect(renderer, &rect);
}

void widebrim_renderer_init(widebrim_renderer *renderer, const char *title) {
    if (renderer == NULL || title == NULL) {
        return;
    }

    renderer->window = SDL_CreateWindow(title, 640, 768, 0);
    if (renderer->window == NULL) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        return;
    }

    renderer->renderer = SDL_CreateRenderer(renderer->window, NULL);
    if (renderer->renderer == NULL) {
        fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(renderer->window);
        renderer->window = NULL;
        return;
    }

    renderer->framebuffer = SDL_CreateTexture(renderer->renderer,
                                              SDL_PIXELFORMAT_ARGB8888,
                                              SDL_TEXTUREACCESS_STREAMING,
                                              WIDEBRIM_SCREEN_WIDTH,
                                              WIDEBRIM_COMBINED_HEIGHT);
    if (renderer->framebuffer == NULL) {
        fprintf(stderr, "Framebuffer texture creation failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer->renderer);
        renderer->renderer = NULL;
        SDL_DestroyWindow(renderer->window);
        renderer->window = NULL;
        return;
    }

    renderer->initialized = true;
}

void widebrim_renderer_destroy(widebrim_renderer *renderer) {
    if (renderer == NULL) {
        return;
    }

    if (renderer->framebuffer != NULL) {
        SDL_DestroyTexture(renderer->framebuffer);
        renderer->framebuffer = NULL;
    }

    if (renderer->renderer != NULL) {
        SDL_DestroyRenderer(renderer->renderer);
        renderer->renderer = NULL;
    }

    if (renderer->window != NULL) {
        SDL_DestroyWindow(renderer->window);
        renderer->window = NULL;
    }

    renderer->initialized = false;
}

void widebrim_renderer_begin_frame(widebrim_renderer *renderer) {
    if (renderer == NULL || !renderer->initialized || renderer->renderer == NULL) {
        return;
    }

    SDL_SetRenderDrawColor(renderer->renderer, 10, 12, 18, 255);
    SDL_RenderClear(renderer->renderer);
}

void widebrim_renderer_end_frame(widebrim_renderer *renderer) {
    if (renderer == NULL || !renderer->initialized || renderer->renderer == NULL) {
        return;
    }

    SDL_RenderPresent(renderer->renderer);
}

void widebrim_renderer_draw_debug_screen(widebrim_renderer *renderer,
                                        widebrim_mode_kind mode,
                                        uint32_t frame_counter) {
    if (renderer == NULL || !renderer->initialized || renderer->renderer == NULL) {
        return;
    }

    const int screen_top = 0;
    const int screen_bottom = WIDEBRIM_SCREEN_HEIGHT;
    const int screen_w = WIDEBRIM_SCREEN_WIDTH;
    const int screen_h = WIDEBRIM_SCREEN_HEIGHT;

    widebrim_draw_screen_region(renderer->renderer, 0, screen_top, screen_w, screen_h, 24, 28, 38, 255);
    widebrim_draw_screen_region(renderer->renderer, 0, screen_bottom, screen_w, screen_h, 30, 40, 52, 255);

    SDL_SetRenderDrawColor(renderer->renderer, 255, 255, 255, 255);
    SDL_FRect divider = { 0.0f, (float)screen_h, (float)screen_w, 2.0f };
    SDL_RenderFillRect(renderer->renderer, &divider);

    char label[64];
    snprintf(label, sizeof(label), "mode:%u frame:%u", (unsigned)mode, (unsigned)frame_counter);

    SDL_Surface *surface = SDL_LoadBMP("/System/Library/Colors/Blue.tiff");
    if (surface != NULL) {
        SDL_DestroySurface(surface);
    }

    SDL_SetRenderDrawColor(renderer->renderer, 235, 220, 160, 255);
    SDL_RenderDebugText(renderer->renderer, 8, 10, label);
    SDL_RenderDebugText(renderer->renderer, 8, screen_h + 44, "main screen");
    SDL_RenderDebugText(renderer->renderer, 8, screen_h + 120, "sub screen");
}
