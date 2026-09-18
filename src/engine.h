#ifndef WIDEBRIM_ENGINE_H
#define WIDEBRIM_ENGINE_H

#include <stdbool.h>
#include <stdint.h>

#include <SDL3/SDL.h>

#include "madhatter_bridge.h"

#define WIDEBRIM_SCREEN_WIDTH 256
#define WIDEBRIM_SCREEN_HEIGHT 192
#define WIDEBRIM_COMBINED_HEIGHT 384
#define WIDEBRIM_TARGET_FPS 60u

typedef enum {
    WIDEBRIM_MODE_BOOT = 0,
    WIDEBRIM_MODE_TITLE = 1,
    WIDEBRIM_MODE_ROOM = 2,
    WIDEBRIM_MODE_EVENT = 3
} widebrim_mode_kind;

typedef struct widebrim_renderer {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *framebuffer;
    bool initialized;
} widebrim_renderer;

typedef struct widebrim_game_state {
    widebrim_madhatter madhatter;
    widebrim_mode_kind current_mode;
    uint32_t frame_counter;
    uint32_t last_tick_ms;
    float mode_elapsed_sec;
} widebrim_game_state;

typedef struct widebrim_mode {
    widebrim_mode_kind kind;
    void (*init)(struct widebrim_mode *mode, struct widebrim_game_state *state);
    void (*update)(struct widebrim_mode *mode, struct widebrim_game_state *state, float dt);
    void (*draw)(struct widebrim_mode *mode,
                 struct widebrim_game_state *state,
                 widebrim_renderer *renderer);
    void (*shutdown)(struct widebrim_mode *mode, struct widebrim_game_state *state);
} widebrim_mode;

typedef struct widebrim_mode_manager {
    widebrim_mode current;
    bool has_current;
} widebrim_mode_manager;

typedef struct widebrim_runtime {
    widebrim_renderer renderer;
    widebrim_game_state state;
    widebrim_mode_manager modes;
    bool running;
} widebrim_runtime;

void widebrim_runtime_init(widebrim_runtime *runtime);
void widebrim_runtime_destroy(widebrim_runtime *runtime);
void widebrim_runtime_run(widebrim_runtime *runtime);

void widebrim_renderer_init(widebrim_renderer *renderer, const char *title);
void widebrim_renderer_destroy(widebrim_renderer *renderer);
void widebrim_renderer_begin_frame(widebrim_renderer *renderer);
void widebrim_renderer_end_frame(widebrim_renderer *renderer);
void widebrim_renderer_draw_debug_screen(widebrim_renderer *renderer,
                                        widebrim_mode_kind mode,
                                        uint32_t frame_counter);

void widebrim_game_state_init(widebrim_game_state *state);
void widebrim_game_state_destroy(widebrim_game_state *state);

void widebrim_mode_manager_init(widebrim_mode_manager *manager);
void widebrim_mode_manager_set(widebrim_mode_manager *manager,
                              const widebrim_mode *mode,
                              widebrim_game_state *state);
void widebrim_mode_manager_update(widebrim_mode_manager *manager,
                                 widebrim_game_state *state,
                                 float dt);
void widebrim_mode_manager_draw(widebrim_mode_manager *manager,
                               widebrim_game_state *state,
                               widebrim_renderer *renderer);

#endif
