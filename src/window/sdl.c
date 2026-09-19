#include "../window.h"

#include <SDL3/SDL.h>

#include <stdlib.h>
#include <string.h>

typedef struct sdl_window_impl {
    SDL_Window *window;
    SDL_Renderer *renderer;
} sdl_window_impl;

static SDL_RendererLogicalPresentation sdl_logical_presentation_from_widebrim(wb_logical_presentation mode) {
    switch (mode) {
        case WB_LOGICAL_PRESENTATION_DISABLED:
            return SDL_LOGICAL_PRESENTATION_DISABLED;
        case WB_LOGICAL_PRESENTATION_STRETCH:
            return SDL_LOGICAL_PRESENTATION_STRETCH;
        case WB_LOGICAL_PRESENTATION_LETTERBOX:
            return SDL_LOGICAL_PRESENTATION_LETTERBOX;
        case WB_LOGICAL_PRESENTATION_OVERSCAN:
            return SDL_LOGICAL_PRESENTATION_OVERSCAN;
        case WB_LOGICAL_PRESENTATION_INTEGER_SCALE:
            return SDL_LOGICAL_PRESENTATION_INTEGER_SCALE;
        default:
            return SDL_LOGICAL_PRESENTATION_DISABLED;
    }
}

static SDL_ScaleMode sdl_scale_mode_from_widebrim(wb_scale_mode mode) {
    (void)mode;
    return SDL_SCALEMODE_NEAREST;
}

static SDL_BlendMode sdl_blend_mode_from_widebrim(wb_blend_mode mode) {
    switch (mode) {
        case WB_BLEND_MODE_NONE:
            return SDL_BLENDMODE_NONE;
        case WB_BLEND_MODE_BLEND:
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

static void sdl_window_set_logical_presentation(window *window_instance, int w, int h, wb_logical_presentation mode) {
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

static void sdl_window_set_default_texture_scale_mode(window *window_instance, wb_scale_mode mode) {
    sdl_window_impl *impl = (sdl_window_impl *)window_instance->impl;
    if (impl && impl->renderer) {
        SDL_SetDefaultTextureScaleMode(impl->renderer, sdl_scale_mode_from_widebrim(mode));
    }
}

static void sdl_window_set_draw_blend_mode(window *window_instance, wb_blend_mode mode) {
    sdl_window_impl *impl = (sdl_window_impl *)window_instance->impl;
    if (impl && impl->renderer) {
        SDL_SetRenderDrawBlendMode(impl->renderer, sdl_blend_mode_from_widebrim(mode));
    }
}

static void sdl_window_convert_event_to_render_coordinates(window *window_instance, wb_input_event *event) {
    sdl_window_impl *impl = (sdl_window_impl *)window_instance->impl;
    SDL_Event sdl_event;

    if (!impl || !impl->renderer || !event) {
        return;
    }

    memset(&sdl_event, 0, sizeof(sdl_event));
    sdl_event.button.windowID = SDL_GetWindowID(impl->window);
    sdl_event.motion.windowID = SDL_GetWindowID(impl->window);
    sdl_event.key.windowID = SDL_GetWindowID(impl->window);

    switch (event->type) {
        case WB_INPUT_EVENT_QUIT:
            sdl_event.type = SDL_EVENT_QUIT;
            break;
        case WB_INPUT_EVENT_KEY_DOWN:
            sdl_event.type = SDL_EVENT_KEY_DOWN;
            sdl_event.key.key = (SDL_Keycode)event->data.key.key;
            break;
        case WB_INPUT_EVENT_KEY_UP:
            sdl_event.type = SDL_EVENT_KEY_UP;
            sdl_event.key.key = (SDL_Keycode)event->data.key.key;
            break;
        case WB_INPUT_EVENT_MOUSE_MOTION:
            sdl_event.type = SDL_EVENT_MOUSE_MOTION;
            sdl_event.motion.x = (float)event->data.mouse_motion.x;
            sdl_event.motion.y = (float)event->data.mouse_motion.y;
            sdl_event.motion.xrel = (float)event->data.mouse_motion.dx;
            sdl_event.motion.yrel = (float)event->data.mouse_motion.dy;
            break;
        case WB_INPUT_EVENT_MOUSE_BUTTON_DOWN:
            sdl_event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
            sdl_event.button.x = (float)event->data.mouse_button.x;
            sdl_event.button.y = (float)event->data.mouse_button.y;
            sdl_event.button.button = (Uint8)event->data.mouse_button.button;
            break;
        case WB_INPUT_EVENT_MOUSE_BUTTON_UP:
            sdl_event.type = SDL_EVENT_MOUSE_BUTTON_UP;
            sdl_event.button.x = (float)event->data.mouse_button.x;
            sdl_event.button.y = (float)event->data.mouse_button.y;
            sdl_event.button.button = (Uint8)event->data.mouse_button.button;
            break;
        case WB_INPUT_EVENT_CUSTOM:
            sdl_event.type = (Uint32)event->data.custom.type;
            break;
        default:
            return;
    }

    SDL_ConvertEventToRenderCoordinates(impl->renderer, &sdl_event);
    switch (sdl_event.type) {
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
            event->data.key.key = (wb_key)sdl_event.key.key;
            break;
        case SDL_EVENT_MOUSE_MOTION:
            event->data.mouse_motion.x = (int)sdl_event.motion.x;
            event->data.mouse_motion.y = (int)sdl_event.motion.y;
            event->data.mouse_motion.dx = (int)sdl_event.motion.xrel;
            event->data.mouse_motion.dy = (int)sdl_event.motion.yrel;
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
            event->data.mouse_button.x = (int)sdl_event.button.x;
            event->data.mouse_button.y = (int)sdl_event.button.y;
            event->data.mouse_button.button = (int)sdl_event.button.button;
            break;
        default:
            break;
    }
}

static void *sdl_window_as_native_window(const window *window_instance) {
    sdl_window_impl *impl = (sdl_window_impl *)window_instance->impl;
    return impl ? impl->window : NULL;
}

static void *sdl_window_as_native_renderer(const window *window_instance) {
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
    .as_native_window = sdl_window_as_native_window,
    .as_native_renderer = sdl_window_as_native_renderer,
};

window *window_create_sdl(const char *title, int width, int height, unsigned int flags) {
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
