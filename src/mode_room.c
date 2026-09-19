#include "mode_room.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mh_anim.h>
#include <mh_datafiles.h>
#include <mh_place.h>

#include "bg_layer.h"
#include "bg_loader.h"
#include "platform_time.h"

#define MODE_ROOM_EXIT_IMAGE_COUNT 8
#define MODE_ROOM_TITLE_CENTER_X 170
#define MODE_ROOM_TITLE_Y 7

#define SCREEN_W 256
#define SCREEN_H 192
#define SPACING 2

#define MODE_ROOM_BUTTON_RELEASE_COOLDOWN_FRAMES 6

#define MODE_ROOM_MOVE_TOGGLE_FALLBACK_W 24
#define MODE_ROOM_MOVE_TOGGLE_FALLBACK_H 30
#define MODE_ROOM_MOVE_TOGGLE_X SCREEN_W - (MODE_ROOM_MOVE_TOGGLE_FALLBACK_W + SPACING * 2)
#define MODE_ROOM_MOVE_TOGGLE_Y SCREEN_H - (MODE_ROOM_MOVE_TOGGLE_FALLBACK_H + SPACING * 2)

#define MODE_ROOM_MENU_TOGGLE_FALLBACK_W 28
#define MODE_ROOM_MENU_TOGGLE_FALLBACK_H 30
#define MODE_ROOM_MENU_TOGGLE_X SCREEN_W - (MODE_ROOM_MENU_TOGGLE_FALLBACK_W + SPACING)
#define MODE_ROOM_MENU_TOGGLE_Y SPACING

#define MODE_ROOM_CAMERA_TOGGLE_FALLBACK_W 24
#define MODE_ROOM_CAMERA_TOGGLE_FALLBACK_H 24
#define MODE_ROOM_CAMERA_TOGGLE_X MODE_ROOM_MENU_TOGGLE_X
#define MODE_ROOM_CAMERA_TOGGLE_Y MODE_ROOM_MENU_TOGGLE_Y + (MODE_ROOM_MENU_TOGGLE_FALLBACK_H + SPACING)

typedef struct {
    renderer_texture *texture;
    renderer_texture *texture_on;
    renderer_texture *texture_click;
    bool pressed;
    bool hovered;
    int release_frames;
    bool pending_mode;
} mode_room_icon_state;

typedef struct {
    renderer_texture *texture;
    renderer_texture *highlighted_texture;
} mode_room_exit_sprite_state;

typedef struct {
    renderer_texture **textures;
    int *frame_widths;
    int *frame_heights;
    uint64_t *frame_durations_ms;
    size_t frame_count;
    uint64_t started_ms;
    int x;
    int y;
} mode_room_bg_anim_state;

typedef struct {
    renderer_texture *texture;
    int width;
    int height;
} mode_room_title_state;

// simplified roomplayer, shows room's top/bottom background and lets the player
// click through rooms. NPCs, party members, tea events, photo pieces and 
// tobj popups are all deferred as they need event scripting
typedef struct {
    game_state *state;
    screen_controller *controller;
    mh_place_data place;
    int room_sub_index;
    int pending_place_num;
    bool done;
    bool in_move_mode;
    uint64_t exit_breath_start_ms;
    mode_room_icon_state move_button;
    mode_room_icon_state menu_button;
    mode_room_icon_state camera_button;
    int highlighted_exit_index;
    mode_room_bg_anim_state bg_animations[MH_PLACE_BGANI_COUNT];
    size_t bg_animation_count;
    mode_room_exit_sprite_state exit_sprites[MODE_ROOM_EXIT_IMAGE_COUNT];
    mode_room_title_state title;
} mode_room_impl;

static void mode_room_load_exit_sprites(mode_room_impl *impl) {
    renderer *renderer = impl->controller->renderer;
    int i;

    for (i = 0; i < MODE_ROOM_EXIT_IMAGE_COUNT; ++i) {
        char path[64];
        mh_buffer data;
        mh_anim_image anim;

        snprintf(path, sizeof(path), "ani/map/exit_%d.arc", i);
        mh_buffer_init(&data);
        if (mh_datafiles_get_data(&impl->state->datafiles, path, &data) != 0) {
            continue;
        }
        if (mh_anim_decode_arc(data.data, data.len, &anim) == 0) {
            const mh_anim_frame *frame = mh_anim_get_frame_by_animation_name(&anim, "gfx");
            const mh_anim_frame *highlight_frame = mh_anim_get_frame_by_animation_name(&anim, "gfx2");
            if (frame) {
                impl->exit_sprites[i].texture = renderer_create_texture_from_rgba(renderer, frame->pixels,
                                                                                 frame->width, frame->height);
            }
            if (highlight_frame) {
                impl->exit_sprites[i].highlighted_texture = renderer_create_texture_from_rgba(renderer,
                                                                                             highlight_frame->pixels,
                                                                                             highlight_frame->width,
                                                                                             highlight_frame->height);
            } else if (frame) {
                impl->exit_sprites[i].highlighted_texture = renderer_create_texture_from_rgba(renderer,
                                                                                             frame->pixels,
                                                                                             frame->width,
                                                                                             frame->height);
            }
            mh_anim_free(&anim);
        }
        mh_buffer_free(&data);
    }
}

static bool mode_room_point_in_rect(float x, float y, const mh_bounding_box *box) {
    return x >= box->x && x < box->x + box->width &&
           y >= box->y && y < box->y + box->height;
}

static int mode_room_find_exit_index_at_point(mode_room_impl *impl, float x, float y) {
    size_t i;

    for (i = 0; i < impl->place.exit_count; ++i) {
        const mh_place_exit *exit = &impl->place.exits[i];
        if (mode_room_point_in_rect(x, y, &exit->bounding)) {
            return (int)i;
        }
    }
    return -1;
}

static bool mode_room_button_rect_contains_point(int x, int y, int rect_x, int rect_y, int rect_w, int rect_h) {
    return x >= rect_x && x < rect_x + rect_w &&
           y >= rect_y && y < rect_y + rect_h;
}

static void mode_room_get_button_size(renderer *renderer_instance, renderer_texture *texture,
                                      int fallback_w, int fallback_h, int *out_w, int *out_h) {
    int w = 0;
    int h = 0;

    renderer_get_texture_size(renderer_instance, texture, &w, &h);
    if (texture && w > 0 && h > 0) {
        *out_w = w;
        *out_h = h;
    } else {
        *out_w = fallback_w;
        *out_h = fallback_h;
    }
}

static void mode_room_set_button_draw_rect(renderer *renderer_instance, renderer_texture *texture,
                                          int x, int y, int fallback_w, int fallback_h, wb_rect *dst) {
    int w;
    int h;

    mode_room_get_button_size(renderer_instance, texture, fallback_w, fallback_h, &w, &h);
    dst->x = (float)x;
    dst->y = (float)y;
    dst->w = (float)w;
    dst->h = (float)h;
}

static renderer_texture *mode_room_load_button_texture(game_state *state, renderer *renderer,
                                                      const char *path, const char *fallback_name) {
    mh_buffer data;
    mh_anim_image anim;
    const mh_anim_frame *frame = NULL;
    renderer_texture *texture = NULL;
    static const char *names[] = { "off", "on", "click", "default", "0", "1", "2" };
    size_t i;

    mh_buffer_init(&data);
    if (mh_datafiles_get_data(&state->datafiles, path, &data) != 0) {
        fprintf(stderr, "widebrim: room button asset '%s' not found\n", path);
        mh_buffer_free(&data);
        return NULL;
    }

    if (mh_anim_decode_arc(data.data, data.len, &anim) == 0) {
        if (fallback_name && (frame = mh_anim_get_frame_by_animation_name(&anim, fallback_name)) != NULL) {
            texture = renderer_create_texture_from_rgba(renderer, frame->pixels, frame->width, frame->height);
        }
        if (!texture) {
            for (i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
                frame = mh_anim_get_frame_by_animation_name(&anim, names[i]);
                if (frame) {
                    texture = renderer_create_texture_from_rgba(renderer, frame->pixels, frame->width, frame->height);
                    break;
                }
            }
        }
        if (!texture && anim.frame_count > 0u) {
            texture = renderer_create_texture_from_rgba(renderer, anim.frames[0].pixels, anim.frames[0].width,
                                                       anim.frames[0].height);
        }
        mh_anim_free(&anim);
    } else {
        fprintf(stderr, "widebrim: room button asset '%s' failed to decode as ARC\n", path);
    }
    mh_buffer_free(&data);
    return texture;
}

static bool mode_room_toggle_rect_contains_point(mode_room_impl *impl, float x, float y) {
    int w;
    int h;
    int rect_x;
    int rect_y;
    float room_y = y - (float)WB_SCREEN_HEIGHT;

    mode_room_get_button_size(impl->controller->renderer, impl->move_button.texture,
                              MODE_ROOM_MOVE_TOGGLE_FALLBACK_W, MODE_ROOM_MOVE_TOGGLE_FALLBACK_H, &w, &h);
    rect_x = MODE_ROOM_MOVE_TOGGLE_X;
    rect_y = MODE_ROOM_MOVE_TOGGLE_Y;
    if (w != MODE_ROOM_MOVE_TOGGLE_FALLBACK_W || h != MODE_ROOM_MOVE_TOGGLE_FALLBACK_H) {
        rect_x = SCREEN_W - (w + SPACING * 2);
        rect_y = SCREEN_H - (h + SPACING * 2);
    }
    return mode_room_button_rect_contains_point((int)x, (int)room_y, rect_x, rect_y, w, h);
}

static bool mode_room_menu_rect_contains_point(mode_room_impl *impl, float x, float y) {
    int w;
    int h;
    int rect_x;
    int rect_y;
    float room_y = y - (float)WB_SCREEN_HEIGHT;

    mode_room_get_button_size(impl->controller->renderer, impl->menu_button.texture,
                              MODE_ROOM_MENU_TOGGLE_FALLBACK_W, MODE_ROOM_MENU_TOGGLE_FALLBACK_H, &w, &h);
    rect_x = MODE_ROOM_MENU_TOGGLE_X;
    rect_y = MODE_ROOM_MENU_TOGGLE_Y;
    if (w != MODE_ROOM_MENU_TOGGLE_FALLBACK_W || h != MODE_ROOM_MENU_TOGGLE_FALLBACK_H) {
        rect_x = SCREEN_W - (w + SPACING);
        rect_y = SPACING;
    }
    return mode_room_button_rect_contains_point((int)x, (int)room_y, rect_x, rect_y, w, h);
}

static bool mode_room_camera_rect_contains_point(mode_room_impl *impl, float x, float y) {
    int w;
    int h;
    int rect_x;
    int rect_y;
    float room_y = y - (float)WB_SCREEN_HEIGHT;

    mode_room_get_button_size(impl->controller->renderer, impl->camera_button.texture,
                              MODE_ROOM_CAMERA_TOGGLE_FALLBACK_W, MODE_ROOM_CAMERA_TOGGLE_FALLBACK_H, &w, &h);
    rect_x = MODE_ROOM_CAMERA_TOGGLE_X;
    rect_y = MODE_ROOM_CAMERA_TOGGLE_Y;
    if (w != MODE_ROOM_CAMERA_TOGGLE_FALLBACK_W || h != MODE_ROOM_CAMERA_TOGGLE_FALLBACK_H) {
        rect_x = MODE_ROOM_MENU_TOGGLE_X;
        rect_y = SPACING + (MODE_ROOM_MENU_TOGGLE_FALLBACK_H + SPACING);
    }
    return mode_room_button_rect_contains_point((int)x, (int)room_y, rect_x, rect_y, w, h);
}

static uint8_t mode_room_exit_sprite_alpha(uint64_t animation_start_ms, bool pressed) {
    if (pressed) {
        return 255u;
    }

    if (animation_start_ms == 0u) {
        animation_start_ms = platform_time_get_ticks();
    }

    int minimum_alpha = 30;
    int maximum_alpha = 255;
    double frequency = 6.0;
    double elapsed_sec = (double)(platform_time_get_ticks() - animation_start_ms) / 1000.0;
    double breath = 0.5 + 0.5 * sin(elapsed_sec * frequency + M_PI_2);
    int alpha = (int)(minimum_alpha + breath * (maximum_alpha - minimum_alpha));
    if (alpha < minimum_alpha) {
        alpha = minimum_alpha;
    } else if (alpha > maximum_alpha) {
        alpha = maximum_alpha;
    }
    return (uint8_t)alpha;
}

static void mode_room_destroy_bg_animations(mode_room_impl *impl) {
    size_t i;

    for (i = 0; i < impl->bg_animation_count; ++i) {
        mode_room_bg_anim_state *anim = &impl->bg_animations[i];
        size_t j;

        for (j = 0; j < anim->frame_count; ++j) {
            if (anim->textures && anim->textures[j]) {
                renderer_destroy_texture(impl->controller->renderer, anim->textures[j]);
            }
        }
        free(anim->textures);
        free(anim->frame_widths);
        free(anim->frame_heights);
        free(anim->frame_durations_ms);
        anim->textures = NULL;
        anim->frame_widths = NULL;
        anim->frame_heights = NULL;
        anim->frame_durations_ms = NULL;
        anim->frame_count = 0u;
        anim->started_ms = 0u;
        anim->x = 0;
        anim->y = 0;
    }
    impl->bg_animation_count = 0u;
}

static const mh_anim_animation *mode_room_select_bg_animation(const mh_anim_image *image) {
    const mh_anim_animation *selected = NULL;
    const mh_anim_animation *fallback = NULL;
    size_t i;

    if (!image) {
        return NULL;
    }
    for (i = 0; i < image->animation_count; ++i) {
        const mh_anim_animation *candidate = &image->animations[i];
        if (candidate->keyframe_count == 0u && candidate->first_frame_index < 0) {
            continue;
        }
        if (strcmp(candidate->name, "default") == 0) {
            return candidate;
        }
        if (strcmp(candidate->name, "0") == 0 || strcmp(candidate->name, "1") == 0 || strcmp(candidate->name, "2") == 0) {
            if (!selected) {
                selected = candidate;
            }
            continue;
        }
        if (!fallback) {
            fallback = candidate;
        }
    }
    return selected ? selected : fallback;
}

static void mode_room_bg_anim_asset_name(char *out, size_t out_size, const char *raw_name) {
    size_t len;
    char cleaned[64];

    if (!out || out_size == 0u || !raw_name) {
        return;
    }
    snprintf(cleaned, sizeof(cleaned), "%s", raw_name);
    len = strlen(cleaned);
    while (len > 0u && (cleaned[len - 1u] == ' ' || cleaned[len - 1u] == '\t' || cleaned[len - 1u] == '\n' ||
                       cleaned[len - 1u] == '\r' || cleaned[len - 1u] == '\0')) {
        cleaned[--len] = '\0';
    }
    if (len >= 4u && strcmp(cleaned + len - 4u, ".spr") == 0) {
        cleaned[len - 4u] = '\0';
    }
    if (len >= 4u && strcmp(cleaned + len - 4u, ".arc") == 0) {
        cleaned[len - 4u] = '\0';
    }
    snprintf(out, out_size, "%s", cleaned);
}

static void mode_room_load_bg_animations(mode_room_impl *impl) {
    size_t i;

    mode_room_destroy_bg_animations(impl);
    for (i = 0; i < impl->place.bg_ani_count; ++i) {
        const mh_place_bg_ani *entry = &impl->place.bg_ani[i];
        char asset_name[64];
        char path[64];
        mh_buffer data;
        mh_anim_image anim;
        const mh_anim_animation *selected = NULL;
        size_t frame_count = 0u;
        size_t frame_index;
        mode_room_bg_anim_state *bg;

        memset(&anim, 0, sizeof(anim));
        mode_room_bg_anim_asset_name(asset_name, sizeof(asset_name), entry->name);
        snprintf(path, sizeof(path), "ani/bgani/%s.arc", asset_name);

        mh_buffer_init(&data);
        if (mh_datafiles_get_data(&impl->state->datafiles, path, &data) != 0) {
            continue;
        }
        if (mh_anim_decode_arc(data.data, data.len, &anim) != 0) {
            mh_buffer_free(&data);
            continue;
        }
        mh_buffer_free(&data);

        selected = mode_room_select_bg_animation(&anim);
        if (!selected) {
            mh_anim_free(&anim);
            continue;
        }
        if (selected->keyframe_count > 0u) {
            frame_count = selected->keyframe_count;
        } else if (selected->first_frame_index >= 0) {
            frame_count = 1u;
        } else {
            mh_anim_free(&anim);
            continue;
        }

        bg = &impl->bg_animations[impl->bg_animation_count++];
        bg->textures = (renderer_texture **)calloc(frame_count ? frame_count : 1u, sizeof(*bg->textures));
        bg->frame_widths = (int *)calloc(frame_count ? frame_count : 1u, sizeof(*bg->frame_widths));
        bg->frame_heights = (int *)calloc(frame_count ? frame_count : 1u, sizeof(*bg->frame_heights));
        bg->frame_durations_ms = (uint64_t *)calloc(frame_count ? frame_count : 1u, sizeof(*bg->frame_durations_ms));
        bg->frame_count = frame_count;
        bg->started_ms = platform_time_get_ticks();
        bg->x = entry->x;
        bg->y = entry->y;

        for (frame_index = 0; frame_index < frame_count; ++frame_index) {
            int frame_id = 0;
            const mh_anim_frame *frame = NULL;
            uint32_t duration_frames = 0u;
            if (selected->keyframe_count > 0u) {
                frame_id = selected->keyframes[frame_index].frame_index;
                duration_frames = selected->keyframes[frame_index].duration_frames;
            } else {
                frame_id = selected->first_frame_index;
            }
            bg->frame_durations_ms[frame_index] = (duration_frames == 0u) ? 16u : (((uint64_t)duration_frames * 1000u) / 60u);
            if (frame_id >= 0 && (size_t)frame_id < anim.frame_count) {
                frame = &anim.frames[frame_id];
            }
            if (frame) {
                bg->textures[frame_index] = renderer_create_texture_from_rgba(impl->controller->renderer,
                                                                             frame->pixels, frame->width, frame->height);
                bg->frame_widths[frame_index] = frame->width;
                bg->frame_heights[frame_index] = frame->height;
            } else {
                bg->frame_widths[frame_index] = 0;
                bg->frame_heights[frame_index] = 0;
            }
        }
        mh_anim_free(&anim);
    }
}

static size_t mode_room_bg_animation_frame_index(const mode_room_bg_anim_state *state, uint64_t now_ms) {
    size_t i;
    uint64_t elapsed_ms;
    uint64_t total_duration_ms = 0u;

    if (!state || state->frame_count == 0u || !state->textures) {
        return 0u;
    }
    for (i = 0; i < state->frame_count; ++i) {
        total_duration_ms += state->frame_durations_ms ? state->frame_durations_ms[i] : 16u;
    }
    if (total_duration_ms == 0u) {
        return 0u;
    }
    elapsed_ms = (now_ms - state->started_ms) % total_duration_ms;
    for (i = 0; i < state->frame_count; ++i) {
        uint64_t duration_ms = state->frame_durations_ms ? state->frame_durations_ms[i] : 16u;
        if (elapsed_ms < duration_ms) {
            return i;
        }
        elapsed_ms -= duration_ms;
    }
    return 0u;
}

static void mode_room_set_move_mode(mode_room_impl *impl, bool enabled) {
    impl->in_move_mode = enabled;
    impl->highlighted_exit_index = -1;
    impl->exit_breath_start_ms = platform_time_get_ticks();
}

static bool mode_room_handle_key(void *implp, const wb_input_event *event) {
    mode_room_impl *impl = (mode_room_impl *)implp;

    if (event && event->type == WB_INPUT_EVENT_KEY_DOWN && event->data.key.key == WB_KEY_M) {
        mode_room_set_move_mode(impl, !impl->in_move_mode);
        return true;
    }
    return false;
}

static void mode_room_load_title_text(mode_room_impl *impl) {
    char pack_path[64];
    char entry_name[32];
    mh_buffer text_data;
    char *text_cstr;
    uint8_t *pixels;
    int w, h;

    if (impl->title.texture) {
        renderer_destroy_texture(impl->controller->renderer, impl->title.texture);
        impl->title.texture = NULL;
    }
    impl->title.width = 0;
    impl->title.height = 0;
    if (!impl->state->font_event_loaded) {
        return;
    }

    snprintf(pack_path, sizeof(pack_path), "nazo/%s/jiten.plz", impl->state->datafiles.language);
    snprintf(entry_name, sizeof(entry_name), "p_%u.txt", (unsigned)impl->place.id_name_place);

    mh_buffer_init(&text_data);
    if (mh_datafiles_get_packed_data(&impl->state->datafiles, pack_path, entry_name, &text_data) != 0) {
        return;
    }

    text_cstr = (char *)malloc(text_data.len + 1u);
    if (!text_cstr) {
        mh_buffer_free(&text_data);
        return;
    }
    memcpy(text_cstr, text_data.data, text_data.len);
    text_cstr[text_data.len] = '\0';
    mh_buffer_free(&text_data);

    if (mh_font_render_string(&impl->state->font_event, text_cstr, &pixels, &w, &h) == 0) {
        impl->title.texture = renderer_create_texture_from_rgba(impl->controller->renderer, pixels, w, h);
        impl->title.width = w;
        impl->title.height = h;
        free(pixels);
    }
    free(text_cstr);
}

static bool mode_room_load_current(mode_room_impl *impl) {
    char pack_path[64];
    char entry_name[64];
    char bg_main_path[64];
    char bg_map_path[64];
    mh_buffer place_bytes;
    int place_num = game_state_get_place_num(impl->state);

    snprintf(pack_path, sizeof(pack_path), "place/%s",
             place_num < 40 ? "plc_data1.plz" : "plc_data2.plz");
    snprintf(entry_name, sizeof(entry_name), "n_place%d_%d.dat", place_num, impl->room_sub_index);

    mh_buffer_init(&place_bytes);
    if (mh_datafiles_get_packed_data(&impl->state->datafiles, pack_path, entry_name, &place_bytes) != 0) {
        fprintf(stderr, "widebrim: failed to load place data '%s' from '%s'\n", entry_name, pack_path);
        return false;
    }
    if (mh_place_load_nds(&impl->place, place_bytes.data, place_bytes.len) != 0) {
        fprintf(stderr, "widebrim: failed to parse place data '%s'\n", entry_name);
        mh_buffer_free(&place_bytes);
        return false;
    }
    mh_buffer_free(&place_bytes);

    snprintf(bg_main_path, sizeof(bg_main_path), "bg/map/main%u.arc", impl->place.bg_main_id);
    snprintf(bg_map_path, sizeof(bg_map_path), "bg/map/map%u.arc", impl->place.bg_map_id);
    bg_loader_load(impl->state, impl->controller, bg_main_path, screen_controller_set_bg_main);
    bg_loader_load(impl->state, impl->controller, bg_map_path, screen_controller_set_bg_sub);
    mode_room_load_bg_animations(impl);
    mode_room_load_title_text(impl);
    return true;
}

static void mode_room_on_transition_fade_done(void *user) {
    mode_room_impl *impl = (mode_room_impl *)user;
    game_state_set_place_num(impl->state, impl->pending_place_num);
    mode_room_set_move_mode(impl, false);
    mode_room_load_current(impl);
    screen_controller_fade_in(impl->controller, FADER_DEFAULT_DURATION_MS, NULL, NULL);
}

static bool mode_room_handle_touch(void *implp, const wb_input_event *event) {
    mode_room_impl *impl = (mode_room_impl *)implp;
    float x, y;
    int exit_index;

    if (!event) {
        return false;
    }

    switch (event->type) {
        case WB_INPUT_EVENT_MOUSE_MOTION:
            if (impl->in_move_mode) {
                if (impl->highlighted_exit_index >= 0) {
                    return true;
                }
                return false;
            }

            {
                bool move_hovered = mode_room_toggle_rect_contains_point(impl, event->data.mouse_motion.x,
                                                                        event->data.mouse_motion.y);
                bool menu_hovered = mode_room_menu_rect_contains_point(impl, event->data.mouse_motion.x,
                                                                        event->data.mouse_motion.y);
                bool camera_hovered = mode_room_camera_rect_contains_point(impl, event->data.mouse_motion.x,
                                                                          event->data.mouse_motion.y);

                impl->move_button.hovered = move_hovered;
                impl->menu_button.hovered = menu_hovered;
                impl->camera_button.hovered = camera_hovered;

                if (impl->move_button.pressed || impl->menu_button.pressed || impl->camera_button.pressed ||
                    impl->move_button.release_frames > 0 || impl->menu_button.release_frames > 0 ||
                    impl->camera_button.release_frames > 0 || move_hovered || menu_hovered || camera_hovered) {
                    return true;
                }
                return false;
            }
        case WB_INPUT_EVENT_MOUSE_BUTTON_DOWN:
            if (impl->in_move_mode) {
                x = (float)event->data.mouse_button.x;
                y = (float)event->data.mouse_button.y - (float)WB_SCREEN_HEIGHT;
                exit_index = mode_room_find_exit_index_at_point(impl, x, y);
                if (exit_index >= 0) {
                    impl->highlighted_exit_index = exit_index;
                    impl->exit_breath_start_ms = platform_time_get_ticks();
                    return true;
                }

                impl->highlighted_exit_index = -1;
                mode_room_set_move_mode(impl, false);
                return false;
            }

            {
                bool menu_hit = mode_room_menu_rect_contains_point(impl, event->data.mouse_button.x,
                                                                   event->data.mouse_button.y);
                bool camera_hit = mode_room_camera_rect_contains_point(impl, event->data.mouse_button.x,
                                                                      event->data.mouse_button.y);
                bool move_hit = mode_room_toggle_rect_contains_point(impl, event->data.mouse_button.x,
                                                                    event->data.mouse_button.y);

                impl->move_button.release_frames = 0;
                impl->menu_button.release_frames = 0;
                impl->camera_button.release_frames = 0;

                if (move_hit) {
                    impl->move_button.pressed = true;
                    return true;
                }
                impl->move_button.pressed = false;

                if (menu_hit) {
                    impl->menu_button.pressed = true;
                    fprintf(stderr, "widebrim: room menu button pressed; bag mode is not implemented yet\n");
                    return true;
                }
                impl->menu_button.pressed = false;

                if (camera_hit) {
                    impl->camera_button.pressed = true;
                    fprintf(stderr, "widebrim: room camera button pressed; camera mode is not implemented yet\n");
                    return true;
                }
                impl->camera_button.pressed = false;
                return false;
            }
        case WB_INPUT_EVENT_MOUSE_BUTTON_UP:
            if (impl->in_move_mode) {
                x = (float)event->data.mouse_button.x;
                y = (float)event->data.mouse_button.y - (float)WB_SCREEN_HEIGHT;
                exit_index = mode_room_find_exit_index_at_point(impl, x, y);

                if (impl->highlighted_exit_index >= 0 && exit_index == impl->highlighted_exit_index) {
                    const mh_place_exit *exit = &impl->place.exits[exit_index];
                    if (mh_place_exit_can_spawn_event(exit)) {
                        fprintf(stderr,
                                "widebrim: exit %d triggers a scripted event (mode_decoding=%u); "
                                "switching to DramaEvent\n",
                                exit_index, exit->mode_decoding);
                        game_state_set_event_id(impl->state, exit->spawn_data);
                        game_state_set_mode_next(impl->state, GAME_MODE_DRAMA_EVENT);
                        game_state_set_mode(impl->state, GAME_MODE_DRAMA_EVENT);
                        impl->done = true;
                    } else {
                        impl->pending_place_num = exit->spawn_data;
                        screen_controller_fade_out(impl->controller, FADER_DEFAULT_DURATION_MS,
                                                    mode_room_on_transition_fade_done, impl);
                    }
                    return true;
                }

                impl->highlighted_exit_index = -1;
                return false;
            }

            {
                bool menu_hit = mode_room_menu_rect_contains_point(impl, event->data.mouse_button.x,
                                                                   event->data.mouse_button.y);
                bool camera_hit = mode_room_camera_rect_contains_point(impl, event->data.mouse_button.x,
                                                                      event->data.mouse_button.y);
                bool move_hit = mode_room_toggle_rect_contains_point(impl, event->data.mouse_button.x,
                                                                    event->data.mouse_button.y);

                if (impl->move_button.pressed || move_hit) {
                    impl->move_button.release_frames = MODE_ROOM_BUTTON_RELEASE_COOLDOWN_FRAMES;
                    impl->move_button.pending_mode = !impl->in_move_mode;
                    impl->move_button.pressed = false;
                    return true;
                }
                impl->move_button.pressed = false;
                impl->move_button.release_frames = 0;

                if (impl->menu_button.pressed || menu_hit) {
                    impl->menu_button.release_frames = MODE_ROOM_BUTTON_RELEASE_COOLDOWN_FRAMES;
                    impl->menu_button.pending_mode = false;
                    impl->menu_button.pressed = false;
                    fprintf(stderr, "widebrim: room menu button pressed; bag mode is not implemented yet\n");
                    return true;
                }
                impl->menu_button.pressed = false;
                impl->menu_button.release_frames = 0;
                impl->menu_button.pending_mode = false;

                if (impl->camera_button.pressed || camera_hit) {
                    impl->camera_button.release_frames = MODE_ROOM_BUTTON_RELEASE_COOLDOWN_FRAMES;
                    impl->camera_button.pending_mode = false;
                    impl->camera_button.pressed = false;
                    fprintf(stderr, "widebrim: room camera button pressed; camera mode is not implemented yet\n");
                    return true;
                }
                impl->camera_button.pressed = false;
                impl->camera_button.release_frames = 0;
                impl->camera_button.pending_mode = false;
                return false;
            }
        default:
            break;
    }

    return false;
}

static void mode_room_draw(void *implp, renderer *renderer_instance) {
    mode_room_impl *impl = (mode_room_impl *)implp;
    size_t i;

    renderer_set_blend_mode(renderer_instance, WB_BLEND_MODE_BLEND);

    wb_rect move_toggle_rect = { 0.0f, 0.0f, 0.0f, 0.0f };
    wb_rect menu_toggle_rect = { 0.0f, 0.0f, 0.0f, 0.0f };
    wb_rect camera_toggle_rect = { 0.0f, 0.0f, 0.0f, 0.0f };

    {
        int w;
        int h;
        mode_room_get_button_size(impl->controller->renderer, impl->move_button.texture,
                                  MODE_ROOM_MOVE_TOGGLE_FALLBACK_W, MODE_ROOM_MOVE_TOGGLE_FALLBACK_H, &w, &h);
        move_toggle_rect.x = (float)(SCREEN_W - (w + SPACING * 2));
        move_toggle_rect.y = (float)(SCREEN_H - (h + SPACING * 2) + (int)WB_SCREEN_HEIGHT);
        move_toggle_rect.w = (float)w;
        move_toggle_rect.h = (float)h;
    }
    {
        int w;
        int h;
        mode_room_get_button_size(impl->controller->renderer, impl->menu_button.texture,
                                  MODE_ROOM_MENU_TOGGLE_FALLBACK_W, MODE_ROOM_MENU_TOGGLE_FALLBACK_H, &w, &h);
        menu_toggle_rect.x = (float)(SCREEN_W - (w + SPACING));
        menu_toggle_rect.y = (float)(SPACING + (int)WB_SCREEN_HEIGHT);
        menu_toggle_rect.w = (float)w;
        menu_toggle_rect.h = (float)h;
    }
    {
        int w;
        int h;
        mode_room_get_button_size(impl->controller->renderer, impl->camera_button.texture,
                                  MODE_ROOM_CAMERA_TOGGLE_FALLBACK_W, MODE_ROOM_CAMERA_TOGGLE_FALLBACK_H, &w, &h);
        camera_toggle_rect.x = (float)MODE_ROOM_MENU_TOGGLE_X;
        camera_toggle_rect.y = (float)(SPACING + MODE_ROOM_MENU_TOGGLE_FALLBACK_H + SPACING + (int)WB_SCREEN_HEIGHT);
        camera_toggle_rect.w = (float)w;
        camera_toggle_rect.h = (float)h;
    }

    for (i = 0; i < impl->bg_animation_count; ++i) {
        const mode_room_bg_anim_state *bg = &impl->bg_animations[i];
        size_t frame_index = mode_room_bg_animation_frame_index(bg, platform_time_get_ticks());
        renderer_texture *frame_texture = bg->textures ? bg->textures[frame_index] : NULL;
        wb_rect rect;

        if (!frame_texture) {
            continue;
        }
        rect.x = (float)bg->x;
        rect.y = (float)(bg->y + (int)WB_SCREEN_HEIGHT);
        rect.w = (float)(bg->frame_widths ? bg->frame_widths[frame_index] : 0);
        rect.h = (float)(bg->frame_heights ? bg->frame_heights[frame_index] : 0);
        if (rect.w <= 0.0f || rect.h <= 0.0f) {
            continue;
        }
        renderer_draw_texture(impl->controller->renderer, frame_texture, &rect);
    }

    if (impl->in_move_mode) {
        if (impl->highlighted_exit_index >= 0) {
            const mh_place_exit *exit = &impl->place.exits[impl->highlighted_exit_index];
            renderer_texture *sprite = NULL;
            if (exit->id_image < MODE_ROOM_EXIT_IMAGE_COUNT) {
                mode_room_exit_sprite_state *exit_sprite = &impl->exit_sprites[exit->id_image];
                sprite = exit_sprite->highlighted_texture ? exit_sprite->highlighted_texture : exit_sprite->texture;
            }
            wb_rect rect;
            rect.x = (float)exit->bounding.x;
            rect.y = (float)exit->bounding.y + (float)WB_SCREEN_HEIGHT;
            rect.w = (float)exit->bounding.width;
            rect.h = (float)exit->bounding.height;

            if (sprite) {
                renderer_set_texture_alpha(impl->controller->renderer, sprite, 255u);
                renderer_draw_texture(impl->controller->renderer, sprite, &rect);
                renderer_set_texture_alpha(impl->controller->renderer, sprite, 255u);
            } else {
                renderer_set_blend_mode(impl->controller->renderer, WB_BLEND_MODE_BLEND);
                renderer_draw_rect(impl->controller->renderer, &rect, 255, 255, 0, 255);
            }
        } else {
            for (i = 0; i < impl->place.exit_count; ++i) {
                const mh_place_exit *exit = &impl->place.exits[i];
                renderer_texture *sprite = NULL;
                if (exit->id_image < MODE_ROOM_EXIT_IMAGE_COUNT) {
                    mode_room_exit_sprite_state *exit_sprite = &impl->exit_sprites[exit->id_image];
                    sprite = exit_sprite->texture;
                }
                wb_rect rect;
                rect.x = (float)exit->bounding.x;
                rect.y = (float)exit->bounding.y + (float)WB_SCREEN_HEIGHT;
                rect.w = (float)exit->bounding.width;
                rect.h = (float)exit->bounding.height;

                if (sprite) {
                    uint8_t sprite_alpha = mode_room_exit_sprite_alpha(impl->exit_breath_start_ms, false);
                    renderer_set_texture_alpha(impl->controller->renderer, sprite, sprite_alpha);
                    renderer_draw_texture(impl->controller->renderer, sprite, &rect);
                    renderer_set_texture_alpha(impl->controller->renderer, sprite, 255u);
                } else {
                    renderer_set_blend_mode(impl->controller->renderer, WB_BLEND_MODE_BLEND);
                    renderer_draw_rect(impl->controller->renderer, &rect, 255, 255, 0,
                                       mode_room_exit_sprite_alpha(impl->exit_breath_start_ms, false));
                }
            }
        }
    } else {
        renderer_texture *move_texture = impl->move_button.texture;
        renderer_texture *menu_texture = impl->menu_button.texture;
        renderer_texture *camera_texture = impl->camera_button.texture;

        if (impl->move_button.release_frames > 0 && impl->move_button.texture_click) {
            move_texture = impl->move_button.texture_click;
            impl->move_button.release_frames--;
            if (impl->move_button.release_frames == 0 && impl->move_button.pending_mode) {
                mode_room_set_move_mode(impl, impl->move_button.pending_mode);
                impl->move_button.pending_mode = false;
            }
        } else if ((impl->move_button.pressed) && impl->move_button.texture_on) {
            move_texture = impl->move_button.texture_on;
        }
        if (impl->menu_button.release_frames > 0 && impl->menu_button.texture_click) {
            menu_texture = impl->menu_button.texture_click;
            impl->menu_button.release_frames--;
            if (impl->menu_button.release_frames == 0) {
                impl->menu_button.pending_mode = false;
            }
        } else if ((impl->menu_button.pressed) && impl->menu_button.texture_on) {
            menu_texture = impl->menu_button.texture_on;
        }
        if (impl->camera_button.release_frames > 0 && impl->camera_button.texture_click) {
            camera_texture = impl->camera_button.texture_click;
            impl->camera_button.release_frames--;
            if (impl->camera_button.release_frames == 0) {
                impl->camera_button.pending_mode = false;
            }
        } else if ((impl->camera_button.pressed) && impl->camera_button.texture_on) {
            camera_texture = impl->camera_button.texture_on;
        }

        if (move_texture) {
            renderer_draw_texture(impl->controller->renderer, move_texture, &move_toggle_rect);
        } else {
            renderer_fill_rect(impl->controller->renderer, &move_toggle_rect, 42, 255, 180, 220);
            renderer_draw_rect(impl->controller->renderer, &move_toggle_rect, 0, 0, 0, 255);
        }
        if (menu_texture) {
            renderer_draw_texture(impl->controller->renderer, menu_texture, &menu_toggle_rect);
        } else {
            renderer_fill_rect(impl->controller->renderer, &menu_toggle_rect, 90, 160, 255, 220);
            renderer_draw_rect(impl->controller->renderer, &menu_toggle_rect, 0, 0, 0, 255);
        }
        if (camera_texture) {
            renderer_draw_texture(impl->controller->renderer, camera_texture, &camera_toggle_rect);
        } else {
            renderer_fill_rect(impl->controller->renderer, &camera_toggle_rect, 255, 170, 60, 220);
            renderer_draw_rect(impl->controller->renderer, &camera_toggle_rect, 0, 0, 0, 255);
        }

    }

    if (impl->title.texture) {
        wb_rect rect;
        rect.x = (float)(MODE_ROOM_TITLE_CENTER_X - impl->title.width / 2);
        rect.y = (float)MODE_ROOM_TITLE_Y;
        rect.w = (float)impl->title.width;
        rect.h = (float)impl->title.height;
        renderer_draw_texture(impl->controller->renderer, impl->title.texture, &rect);
    }
}

static bool mode_room_is_done(void *impl) {
    return ((mode_room_impl *)impl)->done;
}

static void mode_room_destroy(void *implp) {
    mode_room_impl *impl = (mode_room_impl *)implp;
    int i;

    mode_room_destroy_bg_animations(impl);
    for (i = 0; i < MODE_ROOM_EXIT_IMAGE_COUNT; ++i) {
        if (impl->exit_sprites[i].texture) {
            renderer_destroy_texture(impl->controller->renderer, impl->exit_sprites[i].texture);
        }
        if (impl->exit_sprites[i].highlighted_texture) {
            renderer_destroy_texture(impl->controller->renderer, impl->exit_sprites[i].highlighted_texture);
        }
    }
    if (impl->move_button.texture) {
        renderer_destroy_texture(impl->controller->renderer, impl->move_button.texture);
    }
    if (impl->move_button.texture_on) {
        renderer_destroy_texture(impl->controller->renderer, impl->move_button.texture_on);
    }
    if (impl->move_button.texture_click) {
        renderer_destroy_texture(impl->controller->renderer, impl->move_button.texture_click);
    }
    if (impl->menu_button.texture) {
        renderer_destroy_texture(impl->controller->renderer, impl->menu_button.texture);
    }
    if (impl->menu_button.texture_on) {
        renderer_destroy_texture(impl->controller->renderer, impl->menu_button.texture_on);
    }
    if (impl->menu_button.texture_click) {
        renderer_destroy_texture(impl->controller->renderer, impl->menu_button.texture_click);
    }
    if (impl->camera_button.texture) {
        renderer_destroy_texture(impl->controller->renderer, impl->camera_button.texture);
    }
    if (impl->camera_button.texture_on) {
        renderer_destroy_texture(impl->controller->renderer, impl->camera_button.texture_on);
    }
    if (impl->camera_button.texture_click) {
        renderer_destroy_texture(impl->controller->renderer, impl->camera_button.texture_click);
    }
    if (impl->title.texture) {
        renderer_destroy_texture(impl->controller->renderer, impl->title.texture);
    }
    free(impl);
}

mode_handler mode_room_create(game_state *state, screen_controller *controller) {
    mode_handler handler;
    mode_room_impl *impl = (mode_room_impl *)malloc(sizeof(mode_room_impl));

    impl->state = state;
    impl->controller = controller;
    impl->room_sub_index = 0;
    impl->pending_place_num = 0;
    impl->done = false;
    impl->in_move_mode = false;
    impl->exit_breath_start_ms = 0u;
    impl->move_button.pressed = false;
    impl->move_button.release_frames = 0;
    impl->move_button.pending_mode = false;
    impl->move_button.texture = NULL;
    impl->move_button.texture_on = NULL;
    impl->move_button.texture_click = NULL;
    impl->menu_button.pressed = false;
    impl->menu_button.release_frames = 0;
    impl->menu_button.pending_mode = false;
    impl->menu_button.texture = NULL;
    impl->menu_button.texture_on = NULL;
    impl->menu_button.texture_click = NULL;
    impl->camera_button.pressed = false;
    impl->camera_button.release_frames = 0;
    impl->camera_button.pending_mode = false;
    impl->camera_button.texture = NULL;
    impl->camera_button.texture_on = NULL;
    impl->camera_button.texture_click = NULL;
    impl->highlighted_exit_index = -1;
    impl->bg_animation_count = 0u;
    memset(impl->bg_animations, 0, sizeof(impl->bg_animations));
    impl->title.texture = NULL;
    impl->title.width = 0;
    impl->title.height = 0;
    memset(&impl->place, 0, sizeof(impl->place));
    memset(impl->exit_sprites, 0, sizeof(impl->exit_sprites));

    mode_room_load_exit_sprites(impl);
    impl->move_button.texture = mode_room_load_button_texture(state, controller->renderer,
                                                             "ani/map/movemode.arc", "off");
    impl->move_button.texture_on = mode_room_load_button_texture(state, controller->renderer,
                                                                "ani/map/movemode.arc", "on");
    impl->move_button.texture_click = mode_room_load_button_texture(state, controller->renderer,
                                                                    "ani/map/movemode.arc", "click");
    impl->menu_button.texture = mode_room_load_button_texture(state, controller->renderer,
                                                             "ani/map/menu_icon.arc", "off");
    impl->menu_button.texture_on = mode_room_load_button_texture(state, controller->renderer,
                                                                "ani/map/menu_icon.arc", "on");
    impl->menu_button.texture_click = mode_room_load_button_texture(state, controller->renderer,
                                                                    "ani/map/menu_icon.arc", "click");
    impl->camera_button.texture = mode_room_load_button_texture(state, controller->renderer,
                                                              "ani/map/camera_icon.arc", "off");
    impl->camera_button.texture_on = mode_room_load_button_texture(state, controller->renderer,
                                                                 "ani/map/camera_icon.arc", "on");
    impl->camera_button.texture_click = mode_room_load_button_texture(state, controller->renderer,
                                                                     "ani/map/camera_icon.arc", "click");
    if (!mode_room_load_current(impl)) {
        fprintf(stderr, "widebrim: room mode failed to load place_num=%d\n", game_state_get_place_num(state));
    }
    screen_controller_fade_in(controller, FADER_DEFAULT_DURATION_MS, NULL, NULL);

    handler.layer.impl = impl;
    handler.layer.update = NULL;
    handler.layer.draw = mode_room_draw;
    handler.layer.handle_key = mode_room_handle_key;
    handler.layer.handle_touch = mode_room_handle_touch;
    handler.layer.on_quit = NULL;
    handler.layer.destroy = mode_room_destroy;
    handler.is_done = mode_room_is_done;
    handler.valid = true;
    return handler;
}
