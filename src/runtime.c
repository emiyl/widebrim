#include "runtime.h"

#include <stdio.h>

#include "bg_layer.h"

#define WIDEBRIM_TARGET_FRAMERATE 60.0
#define WIDEBRIM_WINDOW_SCALE 2

int wb_runtime_init(wb_runtime *rt, const char *datafiles_root, const char *language) {
    rt->window = NULL;
    rt->input = NULL;
    rt->running = false;
    rt->speed_modifier = false;
    rt->alpha_blend_enabled = true;

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        fprintf(stderr, "widebrim: SDL_Init failed: %s\n", SDL_GetError());
        return -1;
    }

    rt->input = input_create_sdl();
    if (!rt->input) {
        fprintf(stderr, "widebrim: input_create_sdl failed\n");
        SDL_Quit();
        return -1;
    }

    rt->window = window_create_sdl("widebrim",
                                  WB_SCREEN_WIDTH * WB_WINDOW_SCALE,
                                  WB_SCREEN_HEIGHT * 2 * WB_WINDOW_SCALE,
                                  0);
    if (!rt->window) {
        fprintf(stderr, "widebrim: window_create_sdl failed: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }
    window_set_logical_presentation(rt->window, WB_SCREEN_WIDTH, WB_SCREEN_HEIGHT * 2,
                                   WB_LOGICAL_PRESENTATION_INTEGER_SCALE);
    window_set_scale(rt->window, 1.0f, 1.0f);
    window_set_default_texture_scale_mode(rt->window, WB_SCALE_MODE_NEAREST);
    window_set_draw_blend_mode(rt->window, WB_BLEND_MODE_BLEND);

    renderer *backend_renderer = renderer_create_sdl(window_get_native_renderer(rt->window));
    if (!backend_renderer) {
        fprintf(stderr, "widebrim: renderer_create_sdl failed\n");
        window_destroy(rt->window);
        rt->window = NULL;
        SDL_Quit();
        return -1;
    }

    if (game_state_init(&rt->state, datafiles_root, language) != 0) {
        fprintf(stderr, "widebrim: failed to initialize Datafiles access at '%s'\n", datafiles_root);
        input_destroy(rt->input);
        rt->input = NULL;
        window_destroy(rt->window);
        rt->window = NULL;
        SDL_Quit();
        return -1;
    }

    mode_spawner_init(&rt->spawner, &rt->state, backend_renderer);
    renderer_set_global_texture_blend_mode(rt->spawner.controller.renderer, WB_BLEND_MODE_BLEND);
    game_state_set_mode(&rt->state, GAME_MODE_RESET);

    rt->engine_skip_clock_event_type = SDL_RegisterEvents(1);

    rt->running = true;
    return 0;
}

void wb_runtime_destroy(wb_runtime *rt) {
    mode_spawner_destroy(&rt->spawner);
    game_state_destroy(&rt->state);
    if (rt->input) {
        input_destroy(rt->input);
        rt->input = NULL;
    }
    if (rt->window) {
        window_destroy(rt->window);
        rt->window = NULL;
    }
    SDL_Quit();
}

void wb_runtime_run(wb_runtime *rt) {
    const double interval_sec = 1.0 / WB_TARGET_FRAMERATE;
    const double interval_ms = interval_sec * 1000.0;
    double dt_ms = 0.0;

    wb_clock_init(&rt->clock);

    while (rt->running && !rt->spawner.should_quit) {
        bool bypass_clock = false;
        wb_input_event event;

        mode_spawner_update(&rt->spawner, (float)dt_ms);

        renderer_clear(rt->spawner.controller.renderer, 0, 0, 0, 255);
        mode_spawner_draw(&rt->spawner, rt->spawner.controller.renderer);
        renderer_present(rt->spawner.controller.renderer);

        while (input_poll_event(rt->input, &event)) {
            window_convert_event_to_render_coordinates(rt->window, &event);

            if (event.type == WB_INPUT_EVENT_QUIT) {
                rt->running = false;
                mode_spawner_on_quit(&rt->spawner);
            } else if (event.type == WB_INPUT_EVENT_MOUSE_BUTTON_DOWN ||
                       event.type == WB_INPUT_EVENT_MOUSE_BUTTON_UP ||
                       event.type == WB_INPUT_EVENT_MOUSE_MOTION) {
                mode_spawner_handle_touch(&rt->spawner, &event);
            } else if (event.type == WB_INPUT_EVENT_KEY_DOWN && event.data.key.key == WB_KEY_TAB) {
                rt->alpha_blend_enabled = !rt->alpha_blend_enabled;
                window_set_draw_blend_mode(rt->window,
                                          rt->alpha_blend_enabled ? WB_BLEND_MODE_BLEND : WB_BLEND_MODE_NONE);
                renderer_set_global_texture_blend_mode(rt->spawner.controller.renderer,
                                                      rt->alpha_blend_enabled ? WB_BLEND_MODE_BLEND : WB_BLEND_MODE_NONE);
            } else if (event.type == WB_INPUT_EVENT_KEY_DOWN || event.type == WB_INPUT_EVENT_KEY_UP) {
                mode_spawner_handle_key(&rt->spawner, &event);
            } else if (event.type == WB_INPUT_EVENT_CUSTOM &&
                       event.data.custom.type == rt->engine_skip_clock_event_type) {
                bypass_clock = true;
            }
        }

        dt_ms = wb_clock_tick(&rt->clock, interval_sec);
        if (bypass_clock) {
            dt_ms = interval_ms;
        }
        if (dt_ms / interval_ms > 1.25) {
            dt_ms = interval_ms;
        }
        if (rt->speed_modifier) {
            dt_ms *= 4.0;
        }
    }
}
