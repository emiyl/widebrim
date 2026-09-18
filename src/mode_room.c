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
    SDL_Texture *exit_sprites[MODE_ROOM_EXIT_IMAGE_COUNT];
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

        snprintf(path, sizeof(path), "data_lt2/ani/map/exit_%d.arc", i);
        mh_buffer_init(&data);
        if (mh_datafiles_get_data(&impl->state->datafiles, path, &data) != 0) {
            continue;
        }
        if (mh_anim_decode_arc(data.data, data.len, &anim) == 0) {
            const mh_anim_frame *frame = mh_anim_get_frame_by_animation_name(&anim, "gfx");
            if (frame) {
                impl->exit_sprites[i] = texture_from_rgba(renderer, frame->pixels, frame->width, frame->height);
            }
            mh_anim_free(&anim);
        }
        mh_buffer_free(&data);
    }
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

    snprintf(pack_path, sizeof(pack_path), "data_lt2/nazo/%s/jiten.plz", impl->state->datafiles.language);
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

    snprintf(pack_path, sizeof(pack_path), "data_lt2/place/%s",
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

    snprintf(bg_main_path, sizeof(bg_main_path), "data_lt2/bg/map/main%u.arc", impl->place.bg_main_id);
    snprintf(bg_map_path, sizeof(bg_map_path), "data_lt2/bg/map/map%u.arc", impl->place.bg_map_id);
    bg_loader_load(impl->state, impl->controller, bg_main_path, screen_controller_set_bg_main);
    bg_loader_load(impl->state, impl->controller, bg_map_path, screen_controller_set_bg_sub);
    mode_room_load_title_text(impl);
    return true;
}

static void mode_room_on_transition_fade_done(void *user) {
    mode_room_impl *impl = (mode_room_impl *)user;
    game_state_set_place_num(impl->state, impl->pending_place_num);
    mode_room_load_current(impl);
    screen_controller_fade_in(impl->controller, FADER_DEFAULT_DURATION_MS, NULL, NULL);
}

static bool mode_room_handle_touch(void *implp, const SDL_Event *event) {
    mode_room_impl *impl = (mode_room_impl *)implp;
    float x, y;
    size_t i;

    if (event->type != SDL_EVENT_MOUSE_BUTTON_DOWN) {
        return false;
    }
    x = event->button.x;
    y = event->button.y - (float)WIDEBRIM_SCREEN_HEIGHT; /* exits are given in top-screen-local coordinates */

    for (i = 0; i < impl->place.exit_count; ++i) {
        const mh_place_exit *exit = &impl->place.exits[i];
        if (x >= exit->bounding.x && x < exit->bounding.x + exit->bounding.width &&
            y >= exit->bounding.y && y < exit->bounding.y + exit->bounding.height) {
            if (mh_place_exit_can_spawn_event(exit)) {
                fprintf(stderr,
                        "widebrim: exit %zu triggers a scripted event (mode_decoding=%u); "
                        "scripting is not implemented yet, ignoring\n",
                        i, exit->mode_decoding);
                return true;
            }
            impl->pending_place_num = exit->spawn_data;
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

    for (i = 0; i < impl->place.exit_count; ++i) {
        const mh_place_exit *exit = &impl->place.exits[i];
        SDL_Texture *sprite = exit->id_image < MODE_ROOM_EXIT_IMAGE_COUNT ? impl->exit_sprites[exit->id_image] : NULL;
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
    impl->title_texture = NULL;
    impl->title_width = 0;
    impl->title_height = 0;
    memset(&impl->place, 0, sizeof(impl->place));
    memset(impl->exit_sprites, 0, sizeof(impl->exit_sprites));

    mode_room_load_exit_sprites(impl);
    if (!mode_room_load_current(impl)) {
        fprintf(stderr, "widebrim: room mode failed to load place_num=%d\n", game_state_get_place_num(state));
    }
    screen_controller_fade_in(controller, FADER_DEFAULT_DURATION_MS, NULL, NULL);

    handler.layer.impl = impl;
    handler.layer.update = NULL;
    handler.layer.draw = mode_room_draw;
    handler.layer.handle_key = NULL;
    handler.layer.handle_touch = mode_room_handle_touch;
    handler.layer.on_quit = NULL;
    handler.layer.destroy = mode_room_destroy;
    handler.is_done = mode_room_is_done;
    handler.valid = true;
    return handler;
}
