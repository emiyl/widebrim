#include "mode_room.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mh_datafiles.h>
#include <mh_place.h>

#include "bg_layer.h" /* WIDEBRIM_SCREEN_HEIGHT */
#include "bg_loader.h"

/* Simplified RoomPlayer: shows the room's top/bottom backgrounds and lets the
 * player click exits to move between rooms. NPCs, party members, tea events,
 * photo pieces and tobj popups are all deferred (they need event scripting). */
typedef struct {
    game_state *state;
    screen_controller *controller;
    mh_place_data place;
    int room_sub_index;
    int pending_place_num;
    bool done;
} mode_room_impl;

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

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 160);
    for (i = 0; i < impl->place.exit_count; ++i) {
        const mh_place_exit *exit = &impl->place.exits[i];
        SDL_FRect rect;
        rect.x = (float)exit->bounding.x;
        rect.y = (float)exit->bounding.y + (float)WIDEBRIM_SCREEN_HEIGHT;
        rect.w = (float)exit->bounding.width;
        rect.h = (float)exit->bounding.height;
        SDL_RenderRect(renderer, &rect);
    }
}

static bool mode_room_is_done(void *impl) {
    return ((mode_room_impl *)impl)->done;
}

static void mode_room_destroy(void *impl) {
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
    memset(&impl->place, 0, sizeof(impl->place));

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
