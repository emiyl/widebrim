#include "mode_room.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mh_anim.h>
#include <mh_datafiles.h>
#include <mh_place.h>

#include "bg_layer.h"
#include "bg_loader.h"
#include "texture_util.h"

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
    SDL_Texture *texture;
    SDL_Texture *texture_on;
    SDL_Texture *texture_click;
    bool pressed;
    bool hovered;
    int release_frames;
    bool pending_mode;
} mode_room_icon_state;

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
    mode_room_icon_state move_button;
    mode_room_icon_state menu_button;
    mode_room_icon_state camera_button;
    int highlighted_exit_index;
    SDL_Texture *exit_sprites[MODE_ROOM_EXIT_IMAGE_COUNT];
    SDL_Texture *exit_sprites_highlighted[MODE_ROOM_EXIT_IMAGE_COUNT];
    SDL_Texture *title_texture;
    int title_width;
    int title_height;
} mode_room_impl;

static void mode_room_load_exit_sprites(mode_room_impl *impl) {
    SDL_Renderer *renderer = impl->controller->bg->renderer;
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
                impl->exit_sprites[i] = texture_from_rgba(renderer, frame->pixels, frame->width, frame->height);
            }
            if (highlight_frame) {
                impl->exit_sprites_highlighted[i] = texture_from_rgba(renderer, highlight_frame->pixels,
                                                                       highlight_frame->width,
                                                                       highlight_frame->height);
            } else if (frame) {
                impl->exit_sprites_highlighted[i] = texture_from_rgba(renderer, frame->pixels, frame->width, frame->height);
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

static void mode_room_get_button_size(SDL_Texture *texture, int fallback_w, int fallback_h,
                                      int *out_w, int *out_h) {
    float w = 0.0f;
    float h = 0.0f;

    if (texture && SDL_GetTextureSize(texture, &w, &h) == 0 && w > 0.0f && h > 0.0f) {
        *out_w = (int)w;
        *out_h = (int)h;
    } else {
        *out_w = fallback_w;
        *out_h = fallback_h;
    }
}

static void mode_room_set_button_draw_rect(SDL_Texture *texture, int x, int y, int fallback_w, int fallback_h,
                                          SDL_FRect *dst) {
    int w;
    int h;

    mode_room_get_button_size(texture, fallback_w, fallback_h, &w, &h);
    dst->x = (float)x;
    dst->y = (float)y;
    dst->w = (float)w;
    dst->h = (float)h;
}

static SDL_Texture *mode_room_load_button_texture(game_state *state, SDL_Renderer *renderer,
                                                 const char *path, const char *fallback_name) {
    mh_buffer data;
    mh_anim_image anim;
    const mh_anim_frame *frame = NULL;
    SDL_Texture *texture = NULL;
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
            texture = texture_from_rgba(renderer, frame->pixels, frame->width, frame->height);
        }
        if (!texture) {
            for (i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
                frame = mh_anim_get_frame_by_animation_name(&anim, names[i]);
                if (frame) {
                    texture = texture_from_rgba(renderer, frame->pixels, frame->width, frame->height);
                    break;
                }
            }
        }
        if (!texture && anim.frame_count > 0u) {
            texture = texture_from_rgba(renderer, anim.frames[0].pixels, anim.frames[0].width, anim.frames[0].height);
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
    float room_y = y - (float)WIDEBRIM_SCREEN_HEIGHT;

    mode_room_get_button_size(impl->move_button.texture, MODE_ROOM_MOVE_TOGGLE_FALLBACK_W,
                              MODE_ROOM_MOVE_TOGGLE_FALLBACK_H, &w, &h);
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
    float room_y = y - (float)WIDEBRIM_SCREEN_HEIGHT;

    mode_room_get_button_size(impl->menu_button.texture, MODE_ROOM_MENU_TOGGLE_FALLBACK_W,
                              MODE_ROOM_MENU_TOGGLE_FALLBACK_H, &w, &h);
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
    float room_y = y - (float)WIDEBRIM_SCREEN_HEIGHT;

    mode_room_get_button_size(impl->camera_button.texture, MODE_ROOM_CAMERA_TOGGLE_FALLBACK_W,
                              MODE_ROOM_CAMERA_TOGGLE_FALLBACK_H, &w, &h);
    rect_x = MODE_ROOM_CAMERA_TOGGLE_X;
    rect_y = MODE_ROOM_CAMERA_TOGGLE_Y;
    if (w != MODE_ROOM_CAMERA_TOGGLE_FALLBACK_W || h != MODE_ROOM_CAMERA_TOGGLE_FALLBACK_H) {
        rect_x = MODE_ROOM_MENU_TOGGLE_X;
        rect_y = SPACING + (MODE_ROOM_MENU_TOGGLE_FALLBACK_H + SPACING);
    }
    return mode_room_button_rect_contains_point((int)x, (int)room_y, rect_x, rect_y, w, h);
}

static void mode_room_set_move_mode(mode_room_impl *impl, bool enabled) {
    impl->in_move_mode = enabled;
    impl->highlighted_exit_index = -1;
}

static bool mode_room_handle_key(void *implp, const SDL_Event *event) {
    mode_room_impl *impl = (mode_room_impl *)implp;

    if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_M) {
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

    if (impl->title_texture) {
        SDL_DestroyTexture(impl->title_texture);
        impl->title_texture = NULL;
    }
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
        impl->title_texture = texture_from_rgba(impl->controller->bg->renderer, pixels, w, h);
        impl->title_width = w;
        impl->title_height = h;
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

static bool mode_room_handle_touch(void *implp, const SDL_Event *event) {
    mode_room_impl *impl = (mode_room_impl *)implp;
    float x, y;
    int exit_index;

    if (event->type == SDL_EVENT_MOUSE_MOTION) {
        if (!impl->in_move_mode) {
            bool move_hovered = mode_room_toggle_rect_contains_point(impl, event->motion.x, event->motion.y);
            bool menu_hovered = mode_room_menu_rect_contains_point(impl, event->motion.x, event->motion.y);
            bool camera_hovered = mode_room_camera_rect_contains_point(impl, event->motion.x, event->motion.y);

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
        x = event->motion.x;
        y = event->motion.y - (float)WIDEBRIM_SCREEN_HEIGHT;
        exit_index = mode_room_find_exit_index_at_point(impl, x, y);
        if (exit_index != impl->highlighted_exit_index) {
            impl->highlighted_exit_index = exit_index;
        }
        return true;
    }

    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        bool menu_hit = mode_room_menu_rect_contains_point(impl, event->button.x, event->button.y);
        bool camera_hit = mode_room_camera_rect_contains_point(impl, event->button.x, event->button.y);
        bool move_hit = mode_room_toggle_rect_contains_point(impl, event->button.x, event->button.y);
        x = event->button.x;
        y = event->button.y - (float)WIDEBRIM_SCREEN_HEIGHT;

        impl->move_button.release_frames = 0;
        impl->menu_button.release_frames = 0;
        impl->camera_button.release_frames = 0;

        if (move_hit) {
            impl->move_button.pressed = true;
            impl->move_button.hovered = true;
            return true;
        }
        impl->move_button.pressed = false;
        impl->move_button.hovered = false;

        if (menu_hit) {
            impl->menu_button.pressed = true;
            impl->menu_button.hovered = true;
            fprintf(stderr, "widebrim: room menu button pressed; bag mode is not implemented yet\n");
            return true;
        }
        impl->menu_button.pressed = false;
        impl->menu_button.hovered = false;
        if (camera_hit) {
            impl->camera_button.pressed = true;
            impl->camera_button.hovered = true;
            fprintf(stderr, "widebrim: room camera button pressed; camera mode is not implemented yet\n");
            return true;
        }
        impl->camera_button.pressed = false;
        impl->camera_button.hovered = false;
    }

    if (event->type == SDL_EVENT_MOUSE_BUTTON_UP) {
        bool menu_hit = mode_room_menu_rect_contains_point(impl, event->button.x, event->button.y);
        bool camera_hit = mode_room_camera_rect_contains_point(impl, event->button.x, event->button.y);
        bool move_hit = mode_room_toggle_rect_contains_point(impl, event->button.x, event->button.y);
        x = event->button.x;
        y = event->button.y - (float)WIDEBRIM_SCREEN_HEIGHT;

        if (impl->move_button.pressed || move_hit) {
            impl->move_button.release_frames = MODE_ROOM_BUTTON_RELEASE_COOLDOWN_FRAMES;
            impl->move_button.pending_mode = !impl->in_move_mode;
            impl->move_button.pressed = false;
            impl->move_button.hovered = false;
            return true;
        }

        impl->move_button.pressed = false;
        impl->move_button.hovered = false;
        impl->move_button.release_frames = 0;

        if (impl->menu_button.pressed || menu_hit) {
            impl->menu_button.release_frames = MODE_ROOM_BUTTON_RELEASE_COOLDOWN_FRAMES;
            impl->menu_button.pending_mode = false;
            impl->menu_button.pressed = false;
            impl->menu_button.hovered = false;
            fprintf(stderr, "widebrim: room menu button pressed; bag mode is not implemented yet\n");
            return true;
        }
        impl->menu_button.pressed = false;
        impl->menu_button.hovered = false;
        impl->menu_button.release_frames = 0;
        impl->menu_button.pending_mode = false;
        if (impl->camera_button.pressed || camera_hit) {
            impl->camera_button.release_frames = MODE_ROOM_BUTTON_RELEASE_COOLDOWN_FRAMES;
            impl->camera_button.pending_mode = false;
            impl->camera_button.pressed = false;
            impl->camera_button.hovered = false;
            fprintf(stderr, "widebrim: room camera button pressed; camera mode is not implemented yet\n");
            return true;
        }
        impl->camera_button.pressed = false;
        impl->camera_button.hovered = false;
        impl->camera_button.release_frames = 0;
        impl->camera_button.pending_mode = false;
    }

    if (event->type != SDL_EVENT_MOUSE_BUTTON_DOWN && event->type != SDL_EVENT_MOUSE_BUTTON_UP) {
        return false;
    }

    if (impl->in_move_mode) {
        exit_index = mode_room_find_exit_index_at_point(impl, x, y);
        if (exit_index >= 0) {
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
                return true;
            }
            impl->pending_place_num = exit->spawn_data;
            mode_room_set_move_mode(impl, false);
            screen_controller_fade_out(impl->controller, FADER_DEFAULT_DURATION_MS,
                                        mode_room_on_transition_fade_done, impl);
            return true;
        }
        mode_room_set_move_mode(impl, false);
        return true;
    }

    for (size_t i = 0; i < impl->place.exit_count; ++i) {
        const mh_place_exit *exit = &impl->place.exits[i];
        if (mode_room_point_in_rect(x, y, &exit->bounding)) {
            if (mh_place_exit_can_spawn_event(exit)) {
                fprintf(stderr,
                        "widebrim: exit %zu triggers a scripted event (mode_decoding=%u); "
                        "switching to DramaEvent\n",
                        i, exit->mode_decoding);
                game_state_set_event_id(impl->state, exit->spawn_data);
                game_state_set_mode_next(impl->state, GAME_MODE_DRAMA_EVENT);
                game_state_set_mode(impl->state, GAME_MODE_DRAMA_EVENT);
                impl->done = true;
                return true;
            }
            impl->pending_place_num = exit->spawn_data;
            mode_room_set_move_mode(impl, false);
            screen_controller_fade_out(impl->controller, FADER_DEFAULT_DURATION_MS,
                                        mode_room_on_transition_fade_done, impl);
            return true;
        }
    }
    return false;
}

static void mode_room_draw(void *implp, SDL_Renderer *renderer) {
    mode_room_impl *impl = (mode_room_impl *)implp;
    size_t i;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    SDL_FRect move_toggle_rect = { 0.0f, 0.0f, 0.0f, 0.0f };
    SDL_FRect menu_toggle_rect = { 0.0f, 0.0f, 0.0f, 0.0f };
    SDL_FRect camera_toggle_rect = { 0.0f, 0.0f, 0.0f, 0.0f };

    {
        int w;
        int h;
        mode_room_get_button_size(impl->move_button.texture, MODE_ROOM_MOVE_TOGGLE_FALLBACK_W,
                                  MODE_ROOM_MOVE_TOGGLE_FALLBACK_H, &w, &h);
        move_toggle_rect.x = (float)(SCREEN_W - (w + SPACING * 2));
        move_toggle_rect.y = (float)(SCREEN_H - (h + SPACING * 2) + (int)WIDEBRIM_SCREEN_HEIGHT);
        move_toggle_rect.w = (float)w;
        move_toggle_rect.h = (float)h;
    }
    {
        int w;
        int h;
        mode_room_get_button_size(impl->menu_button.texture, MODE_ROOM_MENU_TOGGLE_FALLBACK_W,
                                  MODE_ROOM_MENU_TOGGLE_FALLBACK_H, &w, &h);
        menu_toggle_rect.x = (float)(SCREEN_W - (w + SPACING));
        menu_toggle_rect.y = (float)(SPACING + (int)WIDEBRIM_SCREEN_HEIGHT);
        menu_toggle_rect.w = (float)w;
        menu_toggle_rect.h = (float)h;
    }
    {
        int w;
        int h;
        mode_room_get_button_size(impl->camera_button.texture, MODE_ROOM_CAMERA_TOGGLE_FALLBACK_W,
                                  MODE_ROOM_CAMERA_TOGGLE_FALLBACK_H, &w, &h);
        camera_toggle_rect.x = (float)MODE_ROOM_MENU_TOGGLE_X;
        camera_toggle_rect.y = (float)(SPACING + MODE_ROOM_MENU_TOGGLE_FALLBACK_H + SPACING + (int)WIDEBRIM_SCREEN_HEIGHT);
        camera_toggle_rect.w = (float)w;
        camera_toggle_rect.h = (float)h;
    }

    if (impl->in_move_mode) {
        for (i = 0; i < impl->place.exit_count; ++i) {
            const mh_place_exit *exit = &impl->place.exits[i];
            SDL_Texture *sprite = NULL;
            if (exit->id_image < MODE_ROOM_EXIT_IMAGE_COUNT) {
                if ((int)i == impl->highlighted_exit_index) {
                    sprite = impl->exit_sprites_highlighted[exit->id_image] ? impl->exit_sprites_highlighted[exit->id_image]
                                                                          : impl->exit_sprites[exit->id_image];
                } else {
                    sprite = impl->exit_sprites[exit->id_image];
                }
            }
            SDL_FRect rect;
            rect.x = (float)exit->bounding.x;
            rect.y = (float)exit->bounding.y + (float)WIDEBRIM_SCREEN_HEIGHT;
            rect.w = (float)exit->bounding.width;
            rect.h = (float)exit->bounding.height;

            if (sprite) {
                SDL_RenderTexture(renderer, sprite, NULL, &rect);
            } else {
                /* no decoded sprite for this id_image - fall back to an outline so the hotspot stays visible */
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 255, 255, 0, 160);
                SDL_RenderRect(renderer, &rect);
            }
        }
    } else {
        SDL_Texture *move_texture = impl->move_button.texture;
        SDL_Texture *menu_texture = impl->menu_button.texture;
        SDL_Texture *camera_texture = impl->camera_button.texture;

        if (impl->move_button.release_frames > 0 && impl->move_button.texture_click) {
            move_texture = impl->move_button.texture_click;
            impl->move_button.release_frames--;
            if (impl->move_button.release_frames == 0 && impl->move_button.pending_mode) {
                mode_room_set_move_mode(impl, impl->move_button.pending_mode);
                impl->move_button.pending_mode = false;
            }
        } else if ((impl->move_button.pressed || impl->move_button.hovered) && impl->move_button.texture_on) {
            move_texture = impl->move_button.texture_on;
        }
        if (impl->menu_button.release_frames > 0 && impl->menu_button.texture_click) {
            menu_texture = impl->menu_button.texture_click;
            impl->menu_button.release_frames--;
            if (impl->menu_button.release_frames == 0) {
                impl->menu_button.pending_mode = false;
            }
        } else if ((impl->menu_button.pressed || impl->menu_button.hovered) && impl->menu_button.texture_on) {
            menu_texture = impl->menu_button.texture_on;
        }
        if (impl->camera_button.release_frames > 0 && impl->camera_button.texture_click) {
            camera_texture = impl->camera_button.texture_click;
            impl->camera_button.release_frames--;
            if (impl->camera_button.release_frames == 0) {
                impl->camera_button.pending_mode = false;
            }
        } else if ((impl->camera_button.pressed || impl->camera_button.hovered) && impl->camera_button.texture_on) {
            camera_texture = impl->camera_button.texture_on;
        }

        if (move_texture) {
            SDL_RenderTexture(renderer, move_texture, NULL, &move_toggle_rect);
        } else {
            SDL_SetRenderDrawColor(renderer, 42, 255, 180, 220);
            SDL_RenderFillRect(renderer, &move_toggle_rect);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderRect(renderer, &move_toggle_rect);
        }
        if (menu_texture) {
            SDL_RenderTexture(renderer, menu_texture, NULL, &menu_toggle_rect);
        } else {
            SDL_SetRenderDrawColor(renderer, 90, 160, 255, 220);
            SDL_RenderFillRect(renderer, &menu_toggle_rect);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderRect(renderer, &menu_toggle_rect);
        }
        if (camera_texture) {
            SDL_RenderTexture(renderer, camera_texture, NULL, &camera_toggle_rect);
        } else {
            SDL_SetRenderDrawColor(renderer, 255, 170, 60, 220);
            SDL_RenderFillRect(renderer, &camera_toggle_rect);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderRect(renderer, &camera_toggle_rect);
        }

    }

    if (impl->title_texture) {
        SDL_FRect rect;
        rect.x = (float)(MODE_ROOM_TITLE_CENTER_X - impl->title_width / 2);
        rect.y = (float)MODE_ROOM_TITLE_Y;
        rect.w = (float)impl->title_width;
        rect.h = (float)impl->title_height;
        SDL_RenderTexture(renderer, impl->title_texture, NULL, &rect);
    }
}

static bool mode_room_is_done(void *impl) {
    return ((mode_room_impl *)impl)->done;
}

static void mode_room_destroy(void *implp) {
    mode_room_impl *impl = (mode_room_impl *)implp;
    int i;

    for (i = 0; i < MODE_ROOM_EXIT_IMAGE_COUNT; ++i) {
        if (impl->exit_sprites[i]) {
            SDL_DestroyTexture(impl->exit_sprites[i]);
        }
        if (impl->exit_sprites_highlighted[i]) {
            SDL_DestroyTexture(impl->exit_sprites_highlighted[i]);
        }
    }
    if (impl->move_button.texture) {
        SDL_DestroyTexture(impl->move_button.texture);
    }
    if (impl->move_button.texture_on) {
        SDL_DestroyTexture(impl->move_button.texture_on);
    }
    if (impl->move_button.texture_click) {
        SDL_DestroyTexture(impl->move_button.texture_click);
    }
    if (impl->menu_button.texture) {
        SDL_DestroyTexture(impl->menu_button.texture);
    }
    if (impl->menu_button.texture_on) {
        SDL_DestroyTexture(impl->menu_button.texture_on);
    }
    if (impl->menu_button.texture_click) {
        SDL_DestroyTexture(impl->menu_button.texture_click);
    }
    if (impl->camera_button.texture) {
        SDL_DestroyTexture(impl->camera_button.texture);
    }
    if (impl->camera_button.texture_on) {
        SDL_DestroyTexture(impl->camera_button.texture_on);
    }
    if (impl->camera_button.texture_click) {
        SDL_DestroyTexture(impl->camera_button.texture_click);
    }
    if (impl->title_texture) {
        SDL_DestroyTexture(impl->title_texture);
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
    impl->move_button.pressed = false;
    impl->move_button.hovered = false;
    impl->move_button.release_frames = 0;
    impl->move_button.pending_mode = false;
    impl->move_button.texture = NULL;
    impl->move_button.texture_on = NULL;
    impl->move_button.texture_click = NULL;
    impl->menu_button.pressed = false;
    impl->menu_button.hovered = false;
    impl->menu_button.release_frames = 0;
    impl->menu_button.pending_mode = false;
    impl->menu_button.texture = NULL;
    impl->menu_button.texture_on = NULL;
    impl->menu_button.texture_click = NULL;
    impl->camera_button.pressed = false;
    impl->camera_button.hovered = false;
    impl->camera_button.release_frames = 0;
    impl->camera_button.pending_mode = false;
    impl->camera_button.texture = NULL;
    impl->camera_button.texture_on = NULL;
    impl->camera_button.texture_click = NULL;
    impl->highlighted_exit_index = -1;
    impl->title_texture = NULL;
    impl->title_width = 0;
    impl->title_height = 0;
    memset(&impl->place, 0, sizeof(impl->place));
    memset(impl->exit_sprites, 0, sizeof(impl->exit_sprites));
    memset(impl->exit_sprites_highlighted, 0, sizeof(impl->exit_sprites_highlighted));

    mode_room_load_exit_sprites(impl);
    impl->move_button.texture = mode_room_load_button_texture(state, controller->bg->renderer,
                                                             "ani/map/movemode.arc", "off");
    impl->move_button.texture_on = mode_room_load_button_texture(state, controller->bg->renderer,
                                                                "ani/map/movemode.arc", "on");
    impl->move_button.texture_click = mode_room_load_button_texture(state, controller->bg->renderer,
                                                                    "ani/map/movemode.arc", "click");
    impl->menu_button.texture = mode_room_load_button_texture(state, controller->bg->renderer,
                                                             "ani/map/menu_icon.arc", "off");
    impl->menu_button.texture_on = mode_room_load_button_texture(state, controller->bg->renderer,
                                                                "ani/map/menu_icon.arc", "on");
    impl->menu_button.texture_click = mode_room_load_button_texture(state, controller->bg->renderer,
                                                                    "ani/map/menu_icon.arc", "click");
    impl->camera_button.texture = mode_room_load_button_texture(state, controller->bg->renderer,
                                                              "ani/map/camera_icon.arc", "off");
    impl->camera_button.texture_on = mode_room_load_button_texture(state, controller->bg->renderer,
                                                                 "ani/map/camera_icon.arc", "on");
    impl->camera_button.texture_click = mode_room_load_button_texture(state, controller->bg->renderer,
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
