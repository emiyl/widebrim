#ifndef WIDEBRIM_SCREEN_H
#define WIDEBRIM_SCREEN_H

#include <stdbool.h>
#include <stddef.h>

#include <SDL3/SDL.h>

/* C equivalent of widebrim's ScreenLayer: a vtable operating on an opaque
 * impl pointer. Any function pointer may be NULL (treated as a no-op /
 * "event not absorbed" / "nothing to free"). */
typedef struct {
    void *impl;
    void (*update)(void *impl, float dt_ms);
    void (*draw)(void *impl, SDL_Renderer *renderer);
    bool (*handle_key)(void *impl, const SDL_Event *event);
    bool (*handle_touch)(void *impl, const SDL_Event *event);
    void (*on_quit)(void *impl);
    void (*destroy)(void *impl);
} screen_layer;

/* C equivalent of widebrim's ScreenCollection: layers are updated/dispatched
 * input in reverse (topmost/last-added first) order and drawn forward
 * (bottom-most first), matching the Python engine's priority rules. */
typedef struct {
    screen_layer *layers;
    size_t count;
    size_t capacity;
} screen_collection;

void screen_collection_init(screen_collection *sc);
void screen_collection_free(screen_collection *sc); /* also destroys all layers */
int screen_collection_add(screen_collection *sc, screen_layer layer);

/* Removes and returns the last layer without destroying its impl (used to
 * pop/reinstall the fader layer around a game-mode swap). */
screen_layer screen_collection_pop(screen_collection *sc);

/* Removes the layer at index without destroying its impl (caller takes ownership). */
screen_layer screen_collection_remove_at(screen_collection *sc, size_t index);

void screen_collection_update(screen_collection *sc, float dt_ms);
void screen_collection_draw(screen_collection *sc, SDL_Renderer *renderer);
bool screen_collection_handle_key(screen_collection *sc, const SDL_Event *event);
bool screen_collection_handle_touch(screen_collection *sc, const SDL_Event *event);
void screen_collection_on_quit(screen_collection *sc);

#endif
