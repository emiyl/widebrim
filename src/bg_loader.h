#ifndef WIDEBRIM_BG_LOADER_H
#define WIDEBRIM_BG_LOADER_H

#include <stdbool.h>
#include <stdint.h>

#include "game_state.h"
#include "screen_controller.h"

bool bg_loader_load(game_state *state,
                     screen_controller *controller,
                     const char *rel_path,
                     void (*setter)(screen_controller *, const uint8_t *, int, int));

#endif
