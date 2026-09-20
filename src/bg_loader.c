#include "bg_loader.h"

#include <stdlib.h>
#include <stdio.h>

#include <mh_datafiles.h>
#include <mh_image.h>

bool bg_loader_load(game_state *state,
                     screen_controller *controller,
                     const char *rel_path,
                     void (*setter)(screen_controller *, const uint8_t *, int, int)) {
    mh_buffer data;
    uint8_t *rgba = NULL;
    int width = 0, height = 0;
    bool ok = false;

    mh_buffer_init(&data);
    if (mh_datafiles_get_data(&state->datafiles, rel_path, &data) != 0) {
        fprintf(stderr, "widebrim: failed to load background data from %s\n", rel_path);
        return false;
    }
    if (mh_image_decode_static_arc(data.data, data.len, &rgba, &width, &height) == 0) {
        setter(controller, rgba, width, height);
        free(rgba);
        ok = true;
    }
    mh_buffer_free(&data);
    return ok;
}
