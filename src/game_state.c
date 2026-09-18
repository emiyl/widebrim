#include "engine.h"

#include <stdio.h>

static void widebrim_archive_normalize_name(char *out,
                                           size_t out_size,
                                           const char *in) {
    size_t pos = 0u;

    if (out == NULL || out_size == 0u) {
        return;
    }

    out[0] = '\0';
    if (in == NULL) {
        return;
    }

    while (*in != '\0' && pos + 1u < out_size) {
        char ch = *in;

        if (ch == '\\') {
            ch = '/';
        }

        if (ch == '/' && pos > 0u && out[pos - 1u] == '/') {
            ++in;
            continue;
        }

        out[pos++] = ch;
        ++in;
    }

    out[pos] = '\0';

    while (pos > 0u && out[pos - 1u] == '/') {
        out[pos - 1u] = '\0';
        --pos;
    }
}

static const char *widebrim_archive_leaf_name(const char *name) {
    const char *leaf = NULL;

    if (name == NULL) {
        return NULL;
    }

    leaf = strrchr(name, '/');
    if (leaf == NULL) {
        return name;
    }

    return leaf + 1;
}

static int widebrim_room_name_matches_archive(const char *archive_name,
                                              uint32_t room_id) {
    char normalized[256];
    const char *leaf = NULL;
    char expected_map[64];
    char expected_room[64];

    if (archive_name == NULL) {
        return 0;
    }

    widebrim_archive_normalize_name(normalized, sizeof(normalized), archive_name);
    leaf = widebrim_archive_leaf_name(normalized);
    if (leaf == NULL) {
        return 0;
    }

    snprintf(expected_map, sizeof(expected_map), "map%u.arc", room_id);
    snprintf(expected_room, sizeof(expected_room), "room%u.arc", room_id);

    if (strcmp(leaf, expected_map) == 0 || strcmp(normalized, expected_map) == 0) {
        return 1;
    }

    if (strcmp(leaf, expected_room) == 0 || strcmp(normalized, expected_room) == 0) {
        return 1;
    }

    return 0;
}

static int widebrim_scene_name_score(const char *archive_name, uint32_t room_id) {
    char normalized[256];
    char expected_map[64];
    char expected_room[64];
    char expected_relative_map[128];
    char expected_relative_room[128];
    int score = 0;
    const char *leaf = NULL;

    if (archive_name == NULL) {
        return -1;
    }

    widebrim_archive_normalize_name(normalized, sizeof(normalized), archive_name);
    leaf = widebrim_archive_leaf_name(normalized);
    if (leaf == NULL) {
        return -1;
    }

    snprintf(expected_map, sizeof(expected_map), "map%u.arc", room_id);
    snprintf(expected_room, sizeof(expected_room), "room%u.arc", room_id);
    snprintf(expected_relative_map, sizeof(expected_relative_map), "bg/map/%s", expected_map);
    snprintf(expected_relative_room, sizeof(expected_relative_room), "bg/map/%s", expected_room);

    if (strcmp(normalized, expected_map) == 0 || strcmp(leaf, expected_map) == 0) {
        return 100;
    }

    if (strcmp(normalized, expected_relative_map) == 0 ||
        strstr(normalized, expected_relative_map) != NULL) {
        score += 90;
    }

    if (strcmp(normalized, "data_lt2/bg/map/map1.arc") == 0 && room_id == 1u) {
        score += 5;
    }

    if (strcmp(normalized, "data_lt2/bg/map/map0.arc") == 0 && room_id == 0u) {
        score += 5;
    }

    if (strcmp(normalized, "data_lt2/bg/map/") == 0) {
        score += 0;
    }

    if (strcmp(normalized, "bg/map/map1.arc") == 0 && room_id == 1u) {
        score += 5;
    }

    if (strcmp(normalized, expected_relative_room) == 0 ||
        strstr(normalized, expected_relative_room) != NULL) {
        score += 80;
    }

    if (strcmp(normalized, "data_lt2/bg/map/map0.arc") == 0 ||
        strcmp(normalized, "data_lt2/bg/map/map1.arc") == 0) {
        score += 5;
    }

    if (strcmp(normalized, expected_room) == 0 || strcmp(leaf, expected_room) == 0) {
        score += 70;
    }

    if (strstr(normalized, "/bg/map/") != NULL && strstr(normalized, expected_map) != NULL) {
        score += 50;
    }

    if (strstr(normalized, "data_lt2/") != NULL && strstr(normalized, expected_map) != NULL) {
        score += 25;
    }

    if (strstr(normalized, expected_map) != NULL) {
        score += 30;
    }

    if (strstr(normalized, expected_room) != NULL) {
        score += 30;
    }

    return score;
}

void widebrim_game_state_resolve_scene_name(widebrim_game_state *state,
                                           uint32_t room_id,
                                           char *buffer,
                                           size_t buffer_size) {
    size_t i;
    const char *fallback = "map0";
    const char *preferred_names[] = {
        "data_lt2/bg/map/map%u.arc",
        "bg/map/map%u.arc",
        "data_lt2/map/map%u.arc",
        "map%u.arc",
        "room%u.arc",
        "data_lt2/place/map%u.arc"
    };
    size_t preferred_count = sizeof(preferred_names) / sizeof(preferred_names[0]);
    int best_score = -1;
    const char *best_name = NULL;

    if (state == NULL || buffer == NULL || buffer_size == 0u) {
        return;
    }

    if (!state->madhatter.ready) {
        snprintf(buffer, buffer_size, "%s", fallback);
        return;
    }

    for (i = 0u; i < state->madhatter.archive.count; ++i) {
        const mh_archive_entry *entry = &state->madhatter.archive.entries[i];
        const char *name = entry->name;
        int score = 0;

        if (name == NULL || name[0] == '\0') {
            continue;
        }

        score = widebrim_scene_name_score(name, room_id);
        if (score > best_score) {
            best_score = score;
            best_name = name;
        }

        if (widebrim_room_name_matches_archive(name, room_id)) {
            snprintf(buffer, buffer_size, "%s", name);
            return;
        }
    }

    if (best_name != NULL && best_score >= 30) {
        snprintf(buffer, buffer_size, "%s", best_name);
        return;
    }

    for (i = 0u; i < preferred_count; ++i) {
        char candidate[256];
        const char *name = NULL;

        snprintf(candidate, sizeof(candidate), preferred_names[i], room_id);
        name = mh_archive_get(&state->madhatter.archive, candidate) != NULL ? candidate : NULL;
        if (name != NULL) {
            snprintf(buffer, buffer_size, "%s", name);
            return;
        }
    }

    snprintf(buffer, buffer_size, "map%u", room_id);
}

void widebrim_room_init_default(widebrim_room *room, uint32_t id, const char *name) {
    if (room == NULL) {
        return;
    }

    memset(room, 0, sizeof(*room));
    room->id = id;
    if (name != NULL) {
        snprintf(room->name, sizeof(room->name), "%s", name);
    } else {
        snprintf(room->name, sizeof(room->name), "room_%u", (unsigned)id);
    }

    room->bg_r = 22;
    room->bg_g = 29;
    room->bg_b = 36;
    room->accent_r = 88;
    room->accent_g = 161;
    room->accent_b = 145;
    room->hotspot_x = WIDEBRIM_SCREEN_WIDTH / 2;
    room->hotspot_y = WIDEBRIM_SCREEN_HEIGHT / 2;
}

void widebrim_game_state_set_room(widebrim_game_state *state, uint32_t room_id) {
    if (state == NULL) {
        return;
    }

    state->current_room_id = room_id;
    state->room_loaded = false;
    state->current_room.id = room_id;
}

void widebrim_game_state_set_event(widebrim_game_state *state, uint32_t event_id) {
    if (state == NULL) {
        return;
    }

    state->current_event_id = event_id;
    if (event_id == 0u) {
        state->current_event_id = 1u;
    }
}

void widebrim_game_state_set_movie(widebrim_game_state *state, uint32_t movie_id) {
    if (state == NULL) {
        return;
    }

    state->current_movie_id = movie_id;
}

void widebrim_game_state_set_mode(widebrim_game_state *state,
                                 widebrim_mode_kind next_mode) {
    if (state == NULL) {
        return;
    }

    state->next_mode = next_mode;
    if (state->current_mode == next_mode) {
        state->mode_elapsed_sec = 0.0f;
        return;
    }

    state->current_mode = next_mode;
    state->mode_elapsed_sec = 0.0f;

    if (next_mode == WIDEBRIM_MODE_ROOM) {
        state->room_loaded = false;
        if (state->current_room_id == 0u) {
            state->current_room_id = 1u;
        }
    }
}

void widebrim_game_state_set_next_mode(widebrim_game_state *state,
                                      widebrim_mode_kind next_mode) {
    if (state == NULL) {
        return;
    }

    printf("Setting next mode: %s\n", widebrim_mode_kind_to_string(next_mode));

    state->next_mode = next_mode;
    if (state->current_mode == next_mode) {
        state->mode_elapsed_sec = 0.0f;
    }
}

void widebrim_game_state_load_scene(widebrim_game_state *state, uint32_t room_id) {
    char scene_name[128];

    if (state == NULL) {
        return;
    }

    state->current_room_id = room_id;
    widebrim_game_state_resolve_scene_name(state, room_id, scene_name, sizeof(scene_name));
    widebrim_room_init_default(&state->current_room, room_id, scene_name);
    state->current_room_id = room_id;
    state->room_loaded = true;
}

void widebrim_game_state_init(widebrim_game_state *state) {
    if (state == NULL) {
        return;
    }

    memset(state, 0, sizeof(*state));
    state->current_mode = WIDEBRIM_MODE_BOOT;
    state->next_mode = WIDEBRIM_MODE_BOOT;
    state->frame_counter = 0;
    state->last_tick_ms = 0;
    state->mode_elapsed_sec = 0.0f;
    state->current_room_id = 1u;
    state->current_event_id = 0u;
    state->current_movie_id = 0u;
    widebrim_madhatter_init(&state->madhatter);
    widebrim_game_state_load_scene(state, 1);
}

void widebrim_game_state_destroy(widebrim_game_state *state) {
    if (state == NULL) {
        return;
    }

    widebrim_madhatter_free(&state->madhatter);
    state->current_mode = WIDEBRIM_MODE_BOOT;
    state->frame_counter = 0;
    state->mode_elapsed_sec = 0.0f;
    state->room_loaded = false;
}
