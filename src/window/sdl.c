#include "../window.h"

#include <stdlib.h>

typedef struct sdl_window_impl {
    SDL_Window *window;
    SDL_Renderer *renderer;
} sdl_window_impl;

static void sdl_window_destroy(window *window_instance) {
    sdl_window_impl *impl = (sdl_window_impl *)window_instance->impl;

    if (!impl) {
        free(window_instance);
        return;
    }

    if (impl->renderer) {
        SDL_DestroyRenderer(impl->renderer);
        impl->renderer = NULL;
    }
    if (impl->window) {
        SDL_DestroyWindow(impl->window);
        impl->window = NULL;
    }
    free(impl);
    free(window_instance);
}

static void sdl_window_set_logical_presentation(window *window_instance, int w, int h, SDL_RendererLogicalPresentation mode) {
    sdl_window_impl *impl = (sdl_window_impl *)window_instance->impl;
    if (impl && impl->renderer) {
        SDL_SetRenderLogicalPresentation(impl->renderer, w, h, mode);
    }
}

static void sdl_window_set_scale(window *window_instance, float x_scale, float y_scale) {
    sdl_window_impl *impl = (sdl_window_impl *)window_instance->impl;
    if (impl && impl->renderer) {
        SDL_SetRenderScale(impl->renderer, x_scale, y_scale);
    }
}

static void sdl_window_set_default_texture_scale_mode(window *window_instance, SDL_ScaleMode mode) {
    sdl_window_impl *impl = (sdl_window_impl *)window_instance->impl;
    if (impl && impl->renderer) {
        SDL_SetDefaultTextureScaleMode(impl->renderer, mode);
    }
}

static void sdl_window_set_draw_blend_mode(window *window_instance, SDL_BlendMode mode) {
    sdl_window_impl *impl = (sdl_window_impl *)window_instance->impl;
    if (impl && impl->renderer) {
        SDL_SetRenderDrawBlendMode(impl->renderer, mode);
    }
}

static void sdl_window_convert_event_to_render_coordinates(window *window_instance, SDL_Event *event) {
    sdl_window_impl *impl = (sdl_window_impl *)window_instance->impl;
    if (impl && impl->renderer && event) {
        SDL_ConvertEventToRenderCoordinates(impl->renderer, event);
    }
}

static SDL_Window *sdl_window_as_sdl_window(const window *window_instance) {
    sdl_window_impl *impl = (sdl_window_impl *)window_instance->impl;
    return impl ? impl->window : NULL;
}

static SDL_Renderer *sdl_window_as_sdl_renderer(const window *window_instance) {
    sdl_window_impl *impl = (sdl_window_impl *)window_instance->impl;
    return impl ? impl->renderer : NULL;
}

static const window_vtable g_sdl_window_vtable = {
    .destroy = sdl_window_destroy,
    .set_logical_presentation = sdl_window_set_logical_presentation,
    .set_scale = sdl_window_set_scale,
    .set_default_texture_scale_mode = sdl_window_set_default_texture_scale_mode,
    .set_draw_blend_mode = sdl_window_set_draw_blend_mode,
    .convert_event_to_render_coordinates = sdl_window_convert_event_to_render_coordinates,
    .as_sdl_window = sdl_window_as_sdl_window,
    .as_sdl_renderer = sdl_window_as_sdl_renderer,
};

window *window_create_sdl(const char *title, int width, int height, Uint32 flags) {
    window *window_instance = (window *)calloc(1u, sizeof(*window_instance));
    sdl_window_impl *impl = (sdl_window_impl *)calloc(1u, sizeof(*impl));

    if (!window_instance || !impl) {
        free(window_instance);
        free(impl);
        return NULL;
    }

    if (!SDL_CreateWindowAndRenderer(title,
                                     width,
                                     height,
                                     flags,
                                     &impl->window,
                                     &impl->renderer)) {
        free(impl);
        free(window_instance);
        return NULL;
    }

    window_instance->impl = impl;
    window_instance->vt = &g_sdl_window_vtable;
    return window_instance;
}
