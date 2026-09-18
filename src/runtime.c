#include "runtime.h"

#include <stdio.h>

#include "bg_layer.h"

#define WIDEBRIM_TARGET_FRAMERATE 60.0
#define WIDEBRIM_WINDOW_SCALE 2

int widebrim_runtime_init(widebrim_runtime *rt, const char *datafiles_root, const char *language) {
    rt->window = NULL;
    rt->renderer = NULL;
    rt->running = false;
    rt->speed_modifier = false;

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        fprintf(stderr, "widebrim: SDL_Init failed: %s\n", SDL_GetError());
        return -1;
    }

    if (!SDL_CreateWindowAndRenderer("widebrim",
                                      WIDEBRIM_SCREEN_WIDTH * WIDEBRIM_WINDOW_SCALE,
                                      WIDEBRIM_SCREEN_HEIGHT * 2 * WIDEBRIM_WINDOW_SCALE,
                                      0,
                                      &rt->window,
                                      &rt->renderer)) {
        fprintf(stderr, "widebrim: SDL_CreateWindowAndRenderer failed: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }
    SDL_SetRenderLogicalPresentation(rt->renderer, WIDEBRIM_SCREEN_WIDTH, WIDEBRIM_SCREEN_HEIGHT * 2,
                                      SDL_LOGICAL_PRESENTATION_LETTERBOX);

    if (game_state_init(&rt->state, datafiles_root, language) != 0) {
        fprintf(stderr, "widebrim: failed to initialize Datafiles access at '%s'\n", datafiles_root);
        SDL_DestroyRenderer(rt->renderer);
        SDL_DestroyWindow(rt->window);
        SDL_Quit();
        return -1;
    }

    mode_spawner_init(&rt->spawner, &rt->state, rt->renderer);
    game_state_set_mode(&rt->state, GAME_MODE_RESET);

    rt->engine_skip_clock_event_type = SDL_RegisterEvents(1);

    rt->running = true;
    return 0;
}

void widebrim_runtime_destroy(widebrim_runtime *rt) {
    mode_spawner_destroy(&rt->spawner);
    game_state_destroy(&rt->state);
    if (rt->renderer) {
        SDL_DestroyRenderer(rt->renderer);
    }
    if (rt->window) {
        SDL_DestroyWindow(rt->window);
    }
    SDL_Quit();
}

void widebrim_runtime_run(widebrim_runtime *rt) {
    const double interval_sec = 1.0 / WIDEBRIM_TARGET_FRAMERATE;
    const double interval_ms = interval_sec * 1000.0;
    double dt_ms = 0.0;

    widebrim_clock_init(&rt->clock);

    while (rt->running && !rt->spawner.should_quit) {
        bool bypass_clock = false;
        SDL_Event event;

        mode_spawner_update(&rt->spawner, (float)dt_ms);

        SDL_SetRenderDrawColor(rt->renderer, 0, 0, 0, 255);
        SDL_RenderClear(rt->renderer);
        mode_spawner_draw(&rt->spawner, rt->renderer);
        SDL_RenderPresent(rt->renderer);

        while (SDL_PollEvent(&event)) {
            SDL_ConvertEventToRenderCoordinates(rt->renderer, &event);

            if (event.type == SDL_EVENT_QUIT) {
                rt->running = false;
                mode_spawner_on_quit(&rt->spawner);
            } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
                       event.type == SDL_EVENT_MOUSE_BUTTON_UP ||
                       event.type == SDL_EVENT_MOUSE_MOTION) {
                mode_spawner_handle_touch(&rt->spawner, &event);
            } else if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_TAB) {
                rt->speed_modifier = true;
            } else if (event.type == SDL_EVENT_KEY_UP && event.key.key == SDLK_TAB) {
                rt->speed_modifier = false;
            } else if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
                mode_spawner_handle_key(&rt->spawner, &event);
            } else if (event.type == rt->engine_skip_clock_event_type) {
                bypass_clock = true;
            }
        }

        dt_ms = widebrim_clock_tick(&rt->clock, interval_sec);
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
