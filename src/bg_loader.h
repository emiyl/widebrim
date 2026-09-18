#ifndef WIDEBRIM_BG_LOADER_H
#define WIDEBRIM_BG_LOADER_H

#include <stdbool.h>
#include <stdint.h>

#include "game_state.h"
#include "screen_controller.h"

/* Reads rel_path from Datafiles, decodes it as a static background image and
 * hands the RGBA pixels to the given screen_controller setter. */
bool bg_loader_load(game_state *state,
                     screen_controller *controller,
                     const char *rel_path,
                     void (*setter)(screen_controller *, const uint8_t *, int, int));

#endif
