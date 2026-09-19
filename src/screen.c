#include "screen.h"

#include <stdlib.h>
#include <string.h>

void screen_collection_init(screen_collection *sc) {
    sc->layers = NULL;
    sc->count = 0;
    sc->capacity = 0;
}

void screen_collection_free(screen_collection *sc) {
    size_t i;
    for (i = 0; i < sc->count; ++i) {
        if (sc->layers[i].destroy) {
            sc->layers[i].destroy(sc->layers[i].impl);
        }
    }
    free(sc->layers);
    sc->layers = NULL;
    sc->count = 0;
    sc->capacity = 0;
}

int screen_collection_add(screen_collection *sc, screen_layer layer) {
    if (sc->count == sc->capacity) {
        size_t new_capacity = sc->capacity == 0 ? 4 : sc->capacity * 2;
        screen_layer *grown = (screen_layer *)realloc(sc->layers, new_capacity * sizeof(screen_layer));
        if (!grown) {
            return -1;
        }
        sc->layers = grown;
        sc->capacity = new_capacity;
    }
    sc->layers[sc->count++] = layer;
    return 0;
}

screen_layer screen_collection_remove_at(screen_collection *sc, size_t index) {
    screen_layer removed = sc->layers[index];
    size_t i;
    for (i = index; i + 1 < sc->count; ++i) {
        sc->layers[i] = sc->layers[i + 1];
    }
    sc->count -= 1;
    return removed;
}

screen_layer screen_collection_pop(screen_collection *sc) {
    return screen_collection_remove_at(sc, sc->count - 1);
}

void screen_collection_update(screen_collection *sc, float dt_ms) {
    size_t i = sc->count;
    while (i-- > 0) {
        if (sc->layers[i].update) {
            sc->layers[i].update(sc->layers[i].impl, dt_ms);
        }
    }
}

void screen_collection_draw(screen_collection *sc, renderer *renderer_instance) {
    size_t i;
    for (i = 0; i < sc->count; ++i) {
        if (sc->layers[i].draw) {
            sc->layers[i].draw(sc->layers[i].impl, renderer_instance);
        }
    }
}

bool screen_collection_handle_key(screen_collection *sc, const wb_input_event *event) {
    size_t i = sc->count;
    while (i-- > 0) {
        if (sc->layers[i].handle_key && sc->layers[i].handle_key(sc->layers[i].impl, event)) {
            return true;
        }
    }
    return false;
}

bool screen_collection_handle_touch(screen_collection *sc, const wb_input_event *event) {
    size_t i = sc->count;
    while (i-- > 0) {
        if (sc->layers[i].handle_touch && sc->layers[i].handle_touch(sc->layers[i].impl, event)) {
            return true;
        }
    }
    return false;
}

void screen_collection_on_quit(screen_collection *sc) {
    size_t i;
    for (i = 0; i < sc->count; ++i) {
        if (sc->layers[i].on_quit) {
            sc->layers[i].on_quit(sc->layers[i].impl);
        }
    }
}
