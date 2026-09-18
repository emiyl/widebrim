#include "engine.h"

#include <string.h>

static void widebrim_mode_boot_init(widebrim_mode *mode, widebrim_game_state *state) {
    (void)mode;
    (void)state;
}

static void widebrim_mode_boot_update(widebrim_mode *mode, widebrim_game_state *state, float dt) {
    (void)mode;
    (void)dt;
    state->current_mode = WIDEBRIM_MODE_TITLE;
}

static void widebrim_mode_boot_draw(widebrim_mode *mode,
                                   widebrim_game_state *state,
                                   widebrim_renderer *renderer) {
    (void)mode;
    (void)state;
    widebrim_renderer_draw_debug_screen(renderer, WIDEBRIM_MODE_BOOT, state->frame_counter);
}

static void widebrim_mode_boot_shutdown(widebrim_mode *mode, widebrim_game_state *state) {
    (void)mode;
    (void)state;
}

static void widebrim_mode_title_init(widebrim_mode *mode, widebrim_game_state *state) {
    (void)mode;
    (void)state;
}

static void widebrim_mode_title_update(widebrim_mode *mode, widebrim_game_state *state, float dt) {
    (void)mode;
    (void)dt;
    state->current_mode = WIDEBRIM_MODE_ROOM;
}

static void widebrim_mode_title_draw(widebrim_mode *mode,
                                    widebrim_game_state *state,
                                    widebrim_renderer *renderer) {
    (void)mode;
    widebrim_renderer_draw_debug_screen(renderer, WIDEBRIM_MODE_TITLE, state->frame_counter);
}

static void widebrim_mode_title_shutdown(widebrim_mode *mode, widebrim_game_state *state) {
    (void)mode;
    (void)state;
}

static void widebrim_mode_room_init(widebrim_mode *mode, widebrim_game_state *state) {
    (void)mode;
    (void)state;
}

static void widebrim_mode_room_update(widebrim_mode *mode, widebrim_game_state *state, float dt) {
    (void)mode;
    (void)dt;
    state->current_mode = WIDEBRIM_MODE_EVENT;
}

static void widebrim_mode_room_draw(widebrim_mode *mode,
                                   widebrim_game_state *state,
                                   widebrim_renderer *renderer) {
    (void)mode;
    widebrim_renderer_draw_debug_screen(renderer, WIDEBRIM_MODE_ROOM, state->frame_counter);
}

static void widebrim_mode_room_shutdown(widebrim_mode *mode, widebrim_game_state *state) {
    (void)mode;
    (void)state;
}

static void widebrim_mode_event_init(widebrim_mode *mode, widebrim_game_state *state) {
    (void)mode;
    (void)state;
}

static void widebrim_mode_event_update(widebrim_mode *mode, widebrim_game_state *state, float dt) {
    (void)mode;
    (void)dt;
    state->current_mode = WIDEBRIM_MODE_TITLE;
}

static void widebrim_mode_event_draw(widebrim_mode *mode,
                                    widebrim_game_state *state,
                                    widebrim_renderer *renderer) {
    (void)mode;
    widebrim_renderer_draw_debug_screen(renderer, WIDEBRIM_MODE_EVENT, state->frame_counter);
}

static void widebrim_mode_event_shutdown(widebrim_mode *mode, widebrim_game_state *state) {
    (void)mode;
    (void)state;
}

static void widebrim_mode_set_kind(widebrim_mode *mode, widebrim_mode_kind kind) {
    if (mode == NULL) {
        return;
    }

    memset(mode, 0, sizeof(*mode));
    mode->kind = kind;

    switch (kind) {
        case WIDEBRIM_MODE_BOOT:
            mode->init = widebrim_mode_boot_init;
            mode->update = widebrim_mode_boot_update;
            mode->draw = widebrim_mode_boot_draw;
            mode->shutdown = widebrim_mode_boot_shutdown;
            break;
        case WIDEBRIM_MODE_TITLE:
            mode->init = widebrim_mode_title_init;
            mode->update = widebrim_mode_title_update;
            mode->draw = widebrim_mode_title_draw;
            mode->shutdown = widebrim_mode_title_shutdown;
            break;
        case WIDEBRIM_MODE_ROOM:
            mode->init = widebrim_mode_room_init;
            mode->update = widebrim_mode_room_update;
            mode->draw = widebrim_mode_room_draw;
            mode->shutdown = widebrim_mode_room_shutdown;
            break;
        case WIDEBRIM_MODE_EVENT:
        default:
            mode->init = widebrim_mode_event_init;
            mode->update = widebrim_mode_event_update;
            mode->draw = widebrim_mode_event_draw;
            mode->shutdown = widebrim_mode_event_shutdown;
            break;
    }
}

void widebrim_mode_manager_init(widebrim_mode_manager *manager) {
    if (manager == NULL) {
        return;
    }

    memset(manager, 0, sizeof(*manager));
    widebrim_mode_set_kind(&manager->current, WIDEBRIM_MODE_BOOT);
    manager->has_current = true;
    if (manager->current.init != NULL) {
        manager->current.init(&manager->current, NULL);
    }
}

void widebrim_mode_manager_set(widebrim_mode_manager *manager,
                              const widebrim_mode *mode,
                              widebrim_game_state *state) {
    if (manager == NULL || mode == NULL || state == NULL) {
        return;
    }

    if (manager->has_current && manager->current.shutdown != NULL) {
        manager->current.shutdown(&manager->current, state);
    }

    manager->current = *mode;
    manager->has_current = true;

    if (manager->current.init != NULL) {
        manager->current.init(&manager->current, state);
    }
}

void widebrim_mode_manager_update(widebrim_mode_manager *manager,
                                 widebrim_game_state *state,
                                 float dt) {
    if (manager == NULL || state == NULL || !manager->has_current) {
        return;
    }

    if (manager->current.update != NULL) {
        manager->current.update(&manager->current, state, dt);
    }
}

void widebrim_mode_manager_draw(widebrim_mode_manager *manager,
                               widebrim_game_state *state,
                               widebrim_renderer *renderer) {
    if (manager == NULL || state == NULL || !manager->has_current || renderer == NULL) {
        return;
    }

    if (manager->current.draw != NULL) {
        manager->current.draw(&manager->current, state, renderer);
    }
}
