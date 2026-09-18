#include <SDL3/SDL.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "engine.h"

void widebrim_runtime_init(widebrim_runtime *runtime) {
    if (runtime == NULL) {
        return;
    }

    memset(runtime, 0, sizeof(*runtime));
    widebrim_renderer_init(&runtime->renderer, "widebrim-c");
    widebrim_game_state_init(&runtime->state);
    widebrim_mode_manager_init(&runtime->modes);
    runtime->running = true;
}

void widebrim_runtime_destroy(widebrim_runtime *runtime) {
    if (runtime == NULL) {
        return;
    }

    widebrim_game_state_destroy(&runtime->state);
    widebrim_renderer_destroy(&runtime->renderer);
    runtime->running = false;
}

void widebrim_runtime_run(widebrim_runtime *runtime) {
    if (runtime == NULL) {
        return;
    }

    Uint64 last_ticks = SDL_GetTicks();
    while (runtime->running) {
        Uint64 now = SDL_GetTicks();
        float dt = (float)(now - last_ticks) / 1000.0f;
        last_ticks = now;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    runtime->running = false;
                    break;
                case SDL_EVENT_KEY_DOWN:
                    if (event.key.key == SDL_SCANCODE_ESCAPE) {
                        runtime->running = false;
                    }
                    break;
                default:
                    break;
            }
        }

        widebrim_mode_manager_update(&runtime->modes, &runtime->state, dt);

        widebrim_renderer_begin_frame(&runtime->renderer);
        widebrim_mode_manager_draw(&runtime->modes, &runtime->state, &runtime->renderer);
        widebrim_renderer_end_frame(&runtime->renderer);

        runtime->state.frame_counter++;
    }
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
        return 1;
    }

    widebrim_runtime runtime;
    widebrim_runtime_init(&runtime);

    printf("widebrim C port initialized with SDL3 and Madhatter\n");
    printf("SDL version: %d.%d.%d\n",
           SDL_MAJOR_VERSION,
           SDL_MINOR_VERSION,
           SDL_MICRO_VERSION);

    widebrim_runtime_run(&runtime);

    widebrim_runtime_destroy(&runtime);
    SDL_Quit();
    return 0;
}
