#include "../window.h"

#include <stdlib.h>

typedef struct sdl_window_impl {
    SDL_Window *window;
    SDL_Renderer *renderer;
} sdl_window_impl;

static SDL_RendererLogicalPresentation sdl_logical_presentation_from_widebrim(widebrim_logical_presentation mode) {
    switch (mode) {
        case WIDEBRIM_LOGICAL_PRESENTATION_DISABLED:
            return SDL_LOGICAL_PRESENTATION_DISABLED;
        case WIDEBRIM_LOGICAL_PRESENTATION_STRETCH:
            return SDL_LOGICAL_PRESENTATION_STRETCH;
        case WIDEBRIM_LOGICAL_PRESENTATION_LETTERBOX:
            return SDL_LOGICAL_PRESENTATION_LETTERBOX;
        case WIDEBRIM_LOGICAL_PRESENTATION_OVERSCAN:
            return SDL_LOGICAL_PRESENTATION_OVERSCAN;
        case WIDEBRIM_LOGICAL_PRESENTATION_INTEGER_SCALE:
            return SDL_LOGICAL_PRESENTATION_INTEGER_SCALE;
        default:
            return SDL_LOGICAL_PRESENTATION_DISABLED;
    }
}

static SDL_ScaleMode sdl_scale_mode_from_widebrim(widebrim_scale_mode mode) {
    (void)mode;
    return SDL_SCALEMODE_NEAREST;
}

static SDL_BlendMode sdl_blend_mode_from_widebrim(widebrim_blend_mode mode) {
    switch (mode) {
        case WIDEBRIM_BLEND_MODE_NONE:
            return SDL_BLENDMODE_NONE;
        case WIDEBRIM_BLEND_MODE_BLEND:
            return SDL_BLENDMODE_BLEND;
        default:
            return SDL_BLENDMODE_BLEND;
    }
}

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

static void sdl_window_set_logical_presentation(window *window_instance, int w, int h, widebrim_logical_presentation mode) {
    sdl_window_impl *impl = (sdl_window_impl *)window_instance->impl;
    if (impl && impl->renderer) {
        SDL_SetRenderLogicalPresentation(impl->renderer, w, h, sdl_logical_presentation_from_widebrim(mode));
    }
}

static void sdl_window_set_scale(window *window_instance, float x_scale, float y_scale) {
    sdl_window_impl *impl = (sdl_window_impl *)window_instance->impl;
    if (impl && impl->renderer) {
        SDL_SetRenderScale(impl->renderer, x_scale, y_scale);
    }
}

static void sdl_window_set_default_texture_scale_mode(window *window_instance, widebrim_scale_mode mode) {
    sdl_window_impl *impl = (sdl_window_impl *)window_instance->impl;
    if (impl && impl->renderer) {
        SDL_SetDefaultTextureScaleMode(impl->renderer, sdl_scale_mode_from_widebrim(mode));
    }
}

static void sdl_window_set_draw_blend_mode(window *window_instance, widebrim_blend_mode mode) {
    sdl_window_impl *impl = (sdl_window_impl *)window_instance->impl;
    if (impl && impl->renderer) {
        SDL_SetRenderDrawBlendMode(impl->renderer, sdl_blend_mode_from_widebrim(mode));
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
