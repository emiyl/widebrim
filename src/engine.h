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
    WIDEBRIM_MODE_RESET = 0,
    WIDEBRIM_MODE_ROOM = 1,
    WIDEBRIM_MODE_EVENT = 3,
    WIDEBRIM_MODE_DRAMA_EVENT = 3,
    WIDEBRIM_MODE_MOVIE = 6,
    WIDEBRIM_MODE_START_PUZZLE = 7,
    WIDEBRIM_MODE_END_PUZZLE = 8,
    WIDEBRIM_MODE_STAY_PUZZLE = 9,
    WIDEBRIM_MODE_PUZZLE = 10,
    WIDEBRIM_MODE_TITLE = 12,
    WIDEBRIM_MODE_NARRATION = 13,
    WIDEBRIM_MODE_BAG = 17,
    WIDEBRIM_MODE_NAME = 18,
    WIDEBRIM_MODE_MEMO = 23,
    WIDEBRIM_MODE_EVENT_TEA = 25,
    WIDEBRIM_MODE_SECRET_MENU = 28,
    WIDEBRIM_MODE_TOP_SECRET_MENU = 30,
    WIDEBRIM_MODE_ART_MODE = 32,
    WIDEBRIM_MODE_CHR_VIEW_MODE = 33,
    WIDEBRIM_MODE_MOVIE_VIEW_MODE = 36,
    WIDEBRIM_MODE_HAMSTER_NAME = 37,
    WIDEBRIM_MODE_NINTENDO_WFC_SETUP = 38,
    WIDEBRIM_MODE_WIFI_DOWNLOAD_PUZZLE = 39,
    WIDEBRIM_MODE_PASSCODE = 40,
    WIDEBRIM_MODE_CODE_INPUT_PANDORA = 41,
    WIDEBRIM_MODE_CODE_INPUT_FUTURE = 42,
    WIDEBRIM_MODE_DIARY = 43,
    WIDEBRIM_MODE_NAZOBA = 44,
    WIDEBRIM_MODE_INVALID = 255
} widebrim_mode_kind;

typedef struct widebrim_renderer {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *framebuffer;
    bool initialized;
} widebrim_renderer;

typedef struct widebrim_room {
    uint32_t id;
    char name[32];
    Uint8 bg_r;
    Uint8 bg_g;
    Uint8 bg_b;
    Uint8 accent_r;
    Uint8 accent_g;
    Uint8 accent_b;
    int hotspot_x;
    int hotspot_y;
} widebrim_room;

typedef struct widebrim_game_state {
    widebrim_madhatter madhatter;
    widebrim_mode_kind current_mode;
    widebrim_mode_kind next_mode;
    uint32_t frame_counter;
    uint32_t last_tick_ms;
    float mode_elapsed_sec;
    uint32_t current_room_id;
    uint32_t current_event_id;
    uint32_t current_movie_id;
    widebrim_room current_room;
    bool room_loaded;
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

int widebrim_runtime_load_pack_data(widebrim_runtime *runtime,
                                   const uint8_t *data,
                                   size_t len,
                                   int version);
int widebrim_runtime_load_pack_from_path(widebrim_runtime *runtime,
                                       const char *path,
                                       int version);
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
void widebrim_renderer_draw_room(widebrim_renderer *renderer,
                                const widebrim_room *room,
                                uint32_t frame_counter);

void widebrim_room_init_default(widebrim_room *room, uint32_t id, const char *name);
void widebrim_game_state_resolve_scene_name(widebrim_game_state *state,
                                           uint32_t room_id,
                                           char *buffer,
                                           size_t buffer_size);
void widebrim_game_state_set_room(widebrim_game_state *state, uint32_t room_id);
void widebrim_game_state_set_mode(widebrim_game_state *state,
                                 widebrim_mode_kind next_mode);
void widebrim_game_state_set_next_mode(widebrim_game_state *state,
                                      widebrim_mode_kind next_mode);
void widebrim_game_state_load_scene(widebrim_game_state *state, uint32_t room_id);
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
