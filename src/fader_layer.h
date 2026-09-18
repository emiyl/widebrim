#ifndef WIDEBRIM_FADER_LAYER_H
#define WIDEBRIM_FADER_LAYER_H

#include <stdbool.h>

#include <SDL3/SDL.h>

#include "screen.h"

typedef void (*fader_callback)(void *user);

typedef struct {
    float alpha;
    float start;
    float target;
    float duration_ms;
    float elapsed_ms;
    bool active;
    bool flash_white;
    fader_callback callback;
    void *callback_user;
} fader_timeline;

/* C port of FaderLayer: independent main/sub fade overlays plus a skippable wait timer. */
typedef struct {
    fader_timeline main_fade;
    fader_timeline sub_fade;
    float wait_remaining_ms;
    bool wait_can_be_skipped;
} fader_layer;

#define FADER_DEFAULT_DURATION_MS 250.0f

void fader_layer_init(fader_layer *fader);

void fader_layer_fade_out_main(fader_layer *fader, float duration_ms, fader_callback cb, void *user);
void fader_layer_fade_in_main(fader_layer *fader, float duration_ms, fader_callback cb, void *user);
void fader_layer_flash_main(fader_layer *fader, float duration_ms, fader_callback cb, void *user);
void fader_layer_fade_out_sub(fader_layer *fader, float duration_ms, fader_callback cb, void *user);
void fader_layer_fade_in_sub(fader_layer *fader, float duration_ms, fader_callback cb, void *user);
void fader_layer_fade_out(fader_layer *fader, float duration_ms, fader_callback cb, void *user);
void fader_layer_fade_in(fader_layer *fader, float duration_ms, fader_callback cb, void *user);

void fader_layer_set_wait_duration(fader_layer *fader, float duration_ms, bool can_be_skipped);

bool fader_layer_is_fading(const fader_layer *fader);
bool fader_layer_is_view_obscured(const fader_layer *fader);

screen_layer fader_layer_as_screen_layer(fader_layer *fader);

#endif
