#include <SDL3/SDL.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "engine.h"

int widebrim_runtime_load_pack_data(widebrim_runtime *runtime,
                                   const uint8_t *data,
                                   size_t len,
                                   int version) {
    if (runtime == NULL || data == NULL || len == 0) {
        return -1;
    }

    return widebrim_madhatter_load_layton_pack(&runtime->state.madhatter, data, len, version);
}

int widebrim_runtime_load_pack_from_path(widebrim_runtime *runtime,
                                       const char *path,
                                       int version) {
    FILE *fp = NULL;
    long file_size = 0;
    uint8_t *buffer = NULL;
    size_t bytes_read = 0;
    int result = -1;

    if (runtime == NULL || path == NULL) {
        return -1;
    }

    fp = fopen(path, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Unable to open pack: %s\n", path);
        return -1;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return -1;
    }

    file_size = ftell(fp);
    if (file_size < 0) {
        fclose(fp);
        return -1;
    }

    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return -1;
    }

    buffer = (uint8_t *)malloc((size_t)file_size);
    if (buffer == NULL) {
        fclose(fp);
        return -1;
    }

    bytes_read = fread(buffer, 1, (size_t)file_size, fp);
    if (bytes_read != (size_t)file_size) {
        free(buffer);
        fclose(fp);
        return -1;
    }

    result = widebrim_runtime_load_pack_data(runtime, buffer, bytes_read, version);
    free(buffer);
    fclose(fp);

    if (result == 0) {
        printf("Loaded Layton pack: %s\n", path);
    } else {
        fprintf(stderr, "Pack load failed: %s\n", path);
    }

    return result;
}

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
    const char *pack_path = NULL;
    int version = 0;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--pack") == 0 && i + 1 < argc) {
            pack_path = argv[++i];
        } else if (strcmp(argv[i], "--version") == 0 && i + 1 < argc) {
            version = atoi(argv[++i]);
        }
    }

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

    if (pack_path != NULL) {
        widebrim_runtime_load_pack_from_path(&runtime, pack_path, version);
    } else {
        printf("No Layton pack supplied; running in debug bootstrap mode.\n");
    }

    widebrim_runtime_run(&runtime);

    widebrim_runtime_destroy(&runtime);
    SDL_Quit();
    return 0;
}
