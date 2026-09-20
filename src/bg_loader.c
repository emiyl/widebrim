#include "bg_loader.h"

#include <stdlib.h>
#include <stdio.h>

#include <mh_datafiles.h>
#include <mh_image.h>
#include <mh_result.h>

bool bg_loader_load(game_state *state,
                     screen_controller *controller,
                     const char *rel_path,
                     void (*setter)(screen_controller *, const uint8_t *, int, int)) {
    mh_buffer data;
    uint8_t *rgba = NULL;
    int width = 0, height = 0;
    bool ok = false;

    mh_buffer_init(&data);
    result_code result = mh_datafiles_get_data(&state->datafiles, rel_path, &data);
    if (result != RESULT_OK) {
        char* str = "widebrim: failed to load background data";
        switch (result) {
            case RESULT_OK:
                break;
            case RESULT_ERR_NOT_FOUND:
                fprintf(stderr, "%s from %s (file not found)\n", str, rel_path);
                break;
            default:
                fprintf(stderr, "%s from %s (result code: %d)\n", str, rel_path, result);
                break;
        }
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
