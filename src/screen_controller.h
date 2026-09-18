#ifndef WIDEBRIM_SCREEN_CONTROLLER_H
#define WIDEBRIM_SCREEN_CONTROLLER_H

#include "bg_layer.h"
#include "fader_layer.h"

typedef struct {
    bg_layer *bg;
    fader_layer *fader;
} screen_controller;

static inline void screen_controller_set_bg_main(screen_controller *sc, const uint8_t *rgba, int w, int h) {
    bg_layer_set_main_rgba(sc->bg, rgba, w, h);
    bg_layer_modify_palette_main(sc->bg, 0);
}

static inline void screen_controller_set_bg_sub(screen_controller *sc, const uint8_t *rgba, int w, int h) {
    bg_layer_set_sub_rgba(sc->bg, rgba, w, h);
    bg_layer_modify_palette_sub(sc->bg, 0);
}

static inline void screen_controller_fade_in(screen_controller *sc, float duration_ms, fader_callback cb, void *user) {
    fader_layer_fade_in(sc->fader, duration_ms, cb, user);
}

static inline void screen_controller_fade_out(screen_controller *sc, float duration_ms, fader_callback cb, void *user) {
    fader_layer_fade_out(sc->fader, duration_ms, cb, user);
}

static inline bool screen_controller_is_view_obscured(const screen_controller *sc) {
    return fader_layer_is_view_obscured(sc->fader);
}

#endif
