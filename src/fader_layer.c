#include "fader_layer.h"

#include "bg_layer.h"

static void fader_timeline_start(fader_timeline *tl, float target, float duration_ms, bool flash_white,
                                  fader_callback cb, void *user) {
    tl->start = tl->alpha;
    tl->target = target;
    tl->duration_ms = duration_ms > 0.0f ? duration_ms : 1.0f;
    tl->elapsed_ms = 0.0f;
    tl->active = true;
    tl->flash_white = flash_white;
    tl->callback = cb;
    tl->callback_user = user;
}

static void fader_timeline_update(fader_timeline *tl, float dt_ms) {
    float t;
    fader_callback cb;
    void *user;

    if (!tl->active) {
        return;
    }

    tl->elapsed_ms += dt_ms;
    t = tl->elapsed_ms / tl->duration_ms;
    if (t >= 1.0f) {
        tl->alpha = tl->target;
        tl->active = false;
        cb = tl->callback;
        user = tl->callback_user;
        tl->callback = NULL;
        tl->callback_user = NULL;
        if (cb) {
            cb(user);
        }
    } else {
        tl->alpha = tl->start + (tl->target - tl->start) * t;
    }
}

void fader_layer_init(fader_layer *fader) {
    fader->main_fade.alpha = 0.0f;
    fader->main_fade.active = false;
    fader->main_fade.callback = NULL;
    fader->main_fade.callback_user = NULL;
    fader->sub_fade = fader->main_fade;
    fader->wait_remaining_ms = 0.0f;
    fader->wait_can_be_skipped = false;
}

void fader_layer_fade_out_main(fader_layer *fader, float duration_ms, fader_callback cb, void *user) {
    fader_timeline_start(&fader->main_fade, 255.0f, duration_ms, false, cb, user);
}

void fader_layer_fade_in_main(fader_layer *fader, float duration_ms, fader_callback cb, void *user) {
    fader_timeline_start(&fader->main_fade, 0.0f, duration_ms, false, cb, user);
}

void fader_layer_flash_main(fader_layer *fader, float duration_ms, fader_callback cb, void *user) {
    fader_timeline_start(&fader->main_fade, 255.0f, duration_ms, true, cb, user);
}

void fader_layer_fade_out_sub(fader_layer *fader, float duration_ms, fader_callback cb, void *user) {
    fader_timeline_start(&fader->sub_fade, 255.0f, duration_ms, false, cb, user);
}

void fader_layer_fade_in_sub(fader_layer *fader, float duration_ms, fader_callback cb, void *user) {
    fader_timeline_start(&fader->sub_fade, 0.0f, duration_ms, false, cb, user);
}

void fader_layer_fade_out(fader_layer *fader, float duration_ms, fader_callback cb, void *user) {
    fader_layer_fade_out_sub(fader, duration_ms, NULL, NULL);
    fader_layer_fade_out_main(fader, duration_ms, cb, user);
}

void fader_layer_fade_in(fader_layer *fader, float duration_ms, fader_callback cb, void *user) {
    fader_layer_fade_in_sub(fader, duration_ms, NULL, NULL);
    fader_layer_fade_in_main(fader, duration_ms, cb, user);
}

void fader_layer_set_wait_duration(fader_layer *fader, float duration_ms, bool can_be_skipped) {
    fader->wait_remaining_ms = duration_ms;
    fader->wait_can_be_skipped = can_be_skipped;
}

bool fader_layer_is_fading(const fader_layer *fader) {
    return fader->main_fade.active || fader->sub_fade.active;
}

bool fader_layer_is_view_obscured(const fader_layer *fader) {
    return fader->main_fade.alpha >= 255.0f && fader->sub_fade.alpha >= 255.0f;
}

static void fader_layer_update_impl(void *impl, float dt_ms) {
    fader_layer *fader = (fader_layer *)impl;
    fader_timeline_update(&fader->main_fade, dt_ms);
    fader_timeline_update(&fader->sub_fade, dt_ms);
    if (fader->wait_remaining_ms > 0.0f) {
        fader->wait_remaining_ms -= dt_ms;
    }
}

static void fader_layer_draw_rect(renderer *renderer_instance, const fader_timeline *tl, int y_offset) {
    SDL_FRect rect;
    uint8_t alpha;

    if (tl->alpha <= 0.0f) {
        return;
    }

    alpha = tl->alpha >= 255.0f ? 255 : (uint8_t)tl->alpha;
    rect.x = 0.0f;
    rect.y = (float)y_offset;
    rect.w = (float)WIDEBRIM_SCREEN_WIDTH;
    rect.h = (float)WIDEBRIM_SCREEN_HEIGHT;

    renderer_set_blend_mode(renderer_instance, WIDEBRIM_BLEND_MODE_BLEND);
    if (tl->flash_white) {
        renderer_fill_rect(renderer_instance, &rect, 255, 255, 255, alpha);
    } else {
        renderer_fill_rect(renderer_instance, &rect, 0, 0, 0, alpha);
    }
}

static void fader_layer_draw_impl(void *impl, renderer *renderer_instance) {
    fader_layer *fader = (fader_layer *)impl;
    fader_layer_draw_rect(renderer_instance, &fader->sub_fade, 0);
    fader_layer_draw_rect(renderer_instance, &fader->main_fade, WIDEBRIM_SCREEN_HEIGHT);
}

static bool fader_layer_handle_touch_impl(void *impl, const SDL_Event *event) {
    fader_layer *fader = (fader_layer *)impl;
    if (fader->wait_remaining_ms > 0.0f && fader->wait_can_be_skipped &&
        event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        fader->wait_remaining_ms = 0.0f;
        return true;
    }
    return false;
}

screen_layer fader_layer_as_screen_layer(fader_layer *fader) {
    screen_layer layer;
    layer.impl = fader;
    layer.update = fader_layer_update_impl;
    layer.draw = fader_layer_draw_impl;
    layer.handle_key = NULL;
    layer.handle_touch = fader_layer_handle_touch_impl;
    layer.on_quit = NULL;
    layer.destroy = NULL;
    return layer;
}
