#include "engine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void widebrim_draw_screen_region(SDL_Renderer *renderer,
                                       int x,
                                       int y,
                                       int w,
                                       int h,
                                       Uint8 r,
                                       Uint8 g,
                                       Uint8 b,
                                       Uint8 a) {
    SDL_FRect rect = { (float)x, (float)y, (float)w, (float)h };
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    SDL_RenderFillRect(renderer, &rect);
}

static uint32_t widebrim_hash_string(const char *text) {
    uint32_t hash = 2166136261u;
    const unsigned char *ptr = (const unsigned char *)text;

    if (text == NULL) {
        return hash;
    }

    while (*ptr != '\0') {
        hash ^= (uint32_t)(*ptr++);
        hash *= 16777619u;
    }

    return hash;
}

static uint32_t widebrim_hash_bytes(const uint8_t *data, size_t len) {
    uint32_t hash = 2166136261u;
    size_t i;

    if (data == NULL || len == 0u) {
        return hash;
    }

    for (i = 0u; i < len; ++i) {
        hash ^= (uint32_t)data[i];
        hash *= 16777619u;
    }

    return hash;
}

static uint32_t widebrim_hash_archive_entry(const mh_archive_entry *entry) {
    if (entry == NULL || entry->asset.data == NULL || entry->asset.len == 0u) {
        return 2166136261u;
    }

    return widebrim_hash_bytes(entry->asset.data, entry->asset.len);
}

static uint16_t widebrim_read_u16_le(const uint8_t *ptr) {
    return (uint16_t)((uint16_t)ptr[0] | ((uint16_t)ptr[1] << 8));
}

static uint32_t widebrim_read_u32_le(const uint8_t *ptr) {
    return (uint32_t)ptr[0] |
           ((uint32_t)ptr[1] << 8) |
           ((uint32_t)ptr[2] << 16) |
           ((uint32_t)ptr[3] << 24);
}

static uint8_t widebrim_unpack_5bit_to_8bit(uint8_t value) {
    return (uint8_t)(((uint32_t)value * 255u + 15u) / 31u);
}

static int widebrim_decode_arc_background(const uint8_t *payload,
                                         size_t payload_len,
                                         int *out_w,
                                         int *out_h,
                                         uint32_t **out_pixels) {
    const uint8_t *src = payload;
    size_t pos = 0u;
    uint32_t palette_count = 0u;
    uint32_t tile_count = 0u;
    uint16_t tile_w_count = 0u;
    uint16_t tile_h_count = 0u;
    uint8_t *palette = NULL;
    uint8_t *tiles = NULL;
    uint8_t *image = NULL;
    uint32_t *pixels = NULL;
    size_t image_size = 0u;
    size_t tile_map_count = 0u;
    size_t i;

    if (payload == NULL || payload_len == 0u || out_w == NULL || out_h == NULL || out_pixels == NULL) {
        fprintf(stderr, "DEBUG decode_arc: invalid input payload=%p len=%zu\n", (const void *)payload, payload_len);
        return -1;
    }

    if (payload_len < 8u) {
        fprintf(stderr, "DEBUG decode_arc: payload too small (%zu bytes)\n", payload_len);
        return -1;
    }

    palette_count = widebrim_read_u32_le(src + pos);
    pos += 4u;
    fprintf(stderr, "DEBUG decode_arc: palette_count=%u payload_len=%zu\n", palette_count, payload_len);
    if (palette_count > 512u) {
        fprintf(stderr, "DEBUG decode_arc: palette_count too high: %u\n", palette_count);
        return -1;
    }

    if (payload_len - pos < (size_t)palette_count * 2u) {
        fprintf(stderr, "DEBUG decode_arc: palette section truncated\n");
        return -1;
    }

    palette = (uint8_t *)calloc((size_t)palette_count * 3u, sizeof(*palette));
    if (palette == NULL) {
        return -1;
    }

    for (i = 0u; i < palette_count; ++i) {
        uint16_t packed = widebrim_read_u16_le(src + pos);
        uint8_t r5 = (uint8_t)(packed & 0x1Fu);
        uint8_t g5 = (uint8_t)((packed >> 5u) & 0x1Fu);
        uint8_t b5 = (uint8_t)((packed >> 10u) & 0x1Fu);
        palette[i * 3u + 0u] = widebrim_unpack_5bit_to_8bit(r5);
        palette[i * 3u + 1u] = widebrim_unpack_5bit_to_8bit(g5);
        palette[i * 3u + 2u] = widebrim_unpack_5bit_to_8bit(b5);
        pos += 2u;
    }

    if (payload_len - pos < 4u) {
        free(palette);
        return -1;
    }

    tile_count = widebrim_read_u32_le(src + pos);
    pos += 4u;
    fprintf(stderr, "DEBUG decode_arc: tile_count=%u\n", tile_count);
    if (tile_count == 0u || tile_count > 4096u) {
        fprintf(stderr, "DEBUG decode_arc: invalid tile_count=%u\n", tile_count);
        free(palette);
        return -1;
    }

    if (payload_len - pos < (size_t)tile_count * 64u) {
        fprintf(stderr, "DEBUG decode_arc: tile section truncated\n");
        free(palette);
        return -1;
    }

    tiles = (uint8_t *)malloc((size_t)tile_count * 64u * sizeof(*tiles));
    if (tiles == NULL) {
        free(palette);
        return -1;
    }
    memcpy(tiles, src + pos, (size_t)tile_count * 64u);
    pos += (size_t)tile_count * 64u;

    if (payload_len - pos < 4u) {
        free(tiles);
        free(palette);
        return -1;
    }

    tile_w_count = widebrim_read_u16_le(src + pos);
    pos += 2u;
    tile_h_count = widebrim_read_u16_le(src + pos);
    pos += 2u;

    *out_w = (int)tile_w_count * 8;
    *out_h = (int)tile_h_count * 8;
    if (*out_w <= 0 || *out_h <= 0) {
        free(tiles);
        free(palette);
        return -1;
    }

    image_size = (size_t)(*out_w) * (size_t)(*out_h);
    image = (uint8_t *)calloc(image_size, sizeof(*image));
    if (image == NULL) {
        free(tiles);
        free(palette);
        return -1;
    }

    tile_map_count = ((size_t)(*out_w) * (size_t)(*out_h)) / 64u;
    if (tile_map_count == 0u) {
        free(image);
        free(tiles);
        free(palette);
        return -1;
    }

    for (i = 0u; i < tile_map_count; ++i) {
        uint16_t map_entry = 0u;
        uint16_t tile_index = 0u;
        bool flip_x = false;
        bool flip_y = false;
        size_t map_x = 0u;
        size_t map_y = 0u;
        size_t px = 0u;
        size_t py = 0u;

        if (payload_len - pos < 2u) {
            break;
        }

        map_entry = widebrim_read_u16_le(src + pos);
        pos += 2u;
        tile_index = map_entry & 0x03FFu;
        flip_x = (map_entry & 0x0800u) != 0u;
        flip_y = (map_entry & 0x0400u) != 0u;
        if (tile_index >= tile_count) {
            tile_index = 0u;
        }

        map_x = (i % (size_t)tile_w_count);
        map_y = (i / (size_t)tile_w_count);
        for (py = 0u; py < 8u; ++py) {
            const size_t y_src = flip_y ? (8u - 1u - py) : py;
            for (px = 0u; px < 8u; ++px) {
                const size_t x_src = flip_x ? (8u - 1u - px) : px;
                const size_t tile_offset = (size_t)tile_index * 64u + y_src * 8u + x_src;
                const size_t out_x = map_x * 8u + px;
                const size_t out_y = map_y * 8u + py;
                if (out_x < (size_t)(*out_w) && out_y < (size_t)(*out_h)) {
                    image[out_y * (size_t)(*out_w) + out_x] = tiles[tile_offset];
                }
            }
        }
    }

    pixels = (uint32_t *)calloc(image_size, sizeof(*pixels));
    if (pixels == NULL) {
        free(image);
        free(tiles);
        free(palette);
        return -1;
    }

    for (i = 0u; i < image_size; ++i) {
        const uint8_t pixel = image[i];
        const uint8_t r = pixel < palette_count ? palette[pixel * 3u + 0u] : 0u;
        const uint8_t g = pixel < palette_count ? palette[pixel * 3u + 1u] : 0u;
        const uint8_t b = pixel < palette_count ? palette[pixel * 3u + 2u] : 0u;
        pixels[i] = 0xFF000000u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
    }

    free(image);
    free(tiles);
    free(palette);

    fprintf(stderr,
            "DEBUG decode_arc: success res=%dx%d tilemap_entries=%zu\n",
            *out_w,
            *out_h,
            tile_map_count);
    *out_pixels = pixels;
    return 0;
}

static bool widebrim_render_asset_texture(SDL_Renderer *renderer,
                                          const uint8_t *payload,
                                          size_t payload_len,
                                          int texture_w,
                                          int texture_h) {
    uint32_t *pixels = NULL;
    SDL_Texture *texture = NULL;
    SDL_FRect dst = { 0.0f, 0.0f, (float)texture_w, (float)texture_h };
    int decoded_w = 0;
    int decoded_h = 0;
    int render_w = texture_w;
    int render_h = texture_h;
    bool success = false;

    if (renderer == NULL || payload == NULL || payload_len == 0u) {
        return false;
    }

    if (widebrim_decode_arc_background(payload, payload_len, &decoded_w, &decoded_h, &pixels) == 0 &&
        pixels != NULL && decoded_w > 0 && decoded_h > 0) {
        render_w = decoded_w;
        render_h = decoded_h;
        success = true;
        fprintf(stderr,
                "DEBUG render_asset: decoded image %dx%d from payload %zu bytes\n",
                decoded_w,
                decoded_h,
                payload_len);
    } else {
        fprintf(stderr,
                "DEBUG render_asset: decode failed for payload %zu bytes, falling back to flat color\n",
                payload_len);
        pixels = (uint32_t *)calloc((size_t)texture_w * (size_t)texture_h, sizeof(*pixels));
        if (pixels == NULL) {
            return false;
        }
        for (size_t i = 0u; i < (size_t)texture_w * (size_t)texture_h; ++i) {
            pixels[i] = 0xFF1A2430u;
        }
    }

    texture = SDL_CreateTexture(renderer,
                               SDL_PIXELFORMAT_ARGB8888,
                               SDL_TEXTUREACCESS_STREAMING,
                               render_w,
                               render_h);
    if (texture == NULL) {
        free(pixels);
        return false;
    }

    SDL_UpdateTexture(texture, NULL, pixels, render_w * (int)sizeof(uint32_t));
    SDL_RenderTexture(renderer, texture, NULL, &dst);
    SDL_DestroyTexture(texture);
    free(pixels);
    return success;
}

static void widebrim_draw_room_scene_layer(SDL_Renderer *renderer,
                                          int x,
                                          int y,
                                          int w,
                                          int h,
                                          Uint8 r,
                                          Uint8 g,
                                          Uint8 b,
                                          Uint8 accent_r,
                                          Uint8 accent_g,
                                          Uint8 accent_b,
                                          const char *scene_name,
                                          uint32_t scene_hash,
                                          const uint8_t *payload,
                                          size_t payload_len) {
    const SDL_FRect base = { (float)x, (float)y, (float)w, (float)h };
    const char *leaf = scene_name != NULL ? strrchr(scene_name, '/') : NULL;
    const char *asset_name = leaf != NULL ? leaf + 1 : (scene_name != NULL ? scene_name : "scene");
    const Uint8 key_r = (Uint8)((scene_hash >> 8) & 0xFFu);
    const Uint8 key_g = (Uint8)((scene_hash >> 16) & 0xFFu);
    const Uint8 key_b = (Uint8)((scene_hash >> 24) & 0xFFu);
    const int stripe_count = 8;
    int i;
    bool rendered_background = false;

    if (payload != NULL && payload_len > 0u) {
        rendered_background = widebrim_render_asset_texture(renderer, payload, payload_len, w, h);
    }

    if (!rendered_background) {
        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
        SDL_RenderFillRect(renderer, &base);

        for (i = 0; i < stripe_count; ++i) {
            const int band_x = x + (i * w) / stripe_count;
            const int band_w = w / stripe_count;
            SDL_FRect stripe = { (float)band_x, (float)y, (float)band_w, (float)h };
            Uint8 stripe_r = (Uint8)((r + key_r + (Uint8)(i * 16u)) / 2u);
            Uint8 stripe_g = (Uint8)((g + key_g + (Uint8)(i * 12u)) / 2u);
            Uint8 stripe_b = (Uint8)((b + key_b + (Uint8)(i * 10u)) / 2u);

            SDL_SetRenderDrawColor(renderer, stripe_r, stripe_g, stripe_b, 180);
            SDL_RenderFillRect(renderer, &stripe);
        }
    }

    SDL_SetRenderDrawColor(renderer, accent_r, accent_g, accent_b, 170);
    SDL_FRect panel = { (float)x + 18.0f, (float)y + 18.0f, (float)w - 36.0f, (float)h / 3.0f };
    SDL_RenderFillRect(renderer, &panel);

    SDL_SetRenderDrawColor(renderer, key_r, key_g, key_b, 210);
    SDL_FRect frame = { (float)x + 30.0f, (float)y + 30.0f, (float)w - 60.0f, (float)h - 60.0f };
    SDL_RenderRect(renderer, &frame);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 180);
    SDL_FRect highlight = { (float)x + 26.0f, (float)y + 24.0f, (float)w - 52.0f, 10.0f };
    SDL_RenderFillRect(renderer, &highlight);

    SDL_SetRenderDrawColor(renderer, 245, 240, 220, 255);
    SDL_RenderDebugText(renderer, x + 30, y + 30, asset_name);
    SDL_RenderDebugText(renderer, x + 30, y + 46, "room background");
}

void widebrim_renderer_init(widebrim_renderer *renderer, const char *title) {
    if (renderer == NULL || title == NULL) {
        return;
    }

    renderer->window = SDL_CreateWindow(title, 640, 768, 0);
    if (renderer->window == NULL) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        return;
    }

    renderer->renderer = SDL_CreateRenderer(renderer->window, NULL);
    if (renderer->renderer == NULL) {
        fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(renderer->window);
        renderer->window = NULL;
        return;
    }

    renderer->framebuffer = SDL_CreateTexture(renderer->renderer,
                                              SDL_PIXELFORMAT_ARGB8888,
                                              SDL_TEXTUREACCESS_STREAMING,
                                              WIDEBRIM_SCREEN_WIDTH,
                                              WIDEBRIM_COMBINED_HEIGHT);
    if (renderer->framebuffer == NULL) {
        fprintf(stderr, "Framebuffer texture creation failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer->renderer);
        renderer->renderer = NULL;
        SDL_DestroyWindow(renderer->window);
        renderer->window = NULL;
        return;
    }

    renderer->initialized = true;
}

void widebrim_renderer_destroy(widebrim_renderer *renderer) {
    if (renderer == NULL) {
        return;
    }

    if (renderer->framebuffer != NULL) {
        SDL_DestroyTexture(renderer->framebuffer);
        renderer->framebuffer = NULL;
    }

    if (renderer->renderer != NULL) {
        SDL_DestroyRenderer(renderer->renderer);
        renderer->renderer = NULL;
    }

    if (renderer->window != NULL) {
        SDL_DestroyWindow(renderer->window);
        renderer->window = NULL;
    }

    renderer->initialized = false;
}

void widebrim_renderer_begin_frame(widebrim_renderer *renderer) {
    if (renderer == NULL || !renderer->initialized || renderer->renderer == NULL) {
        return;
    }

    SDL_SetRenderTarget(renderer->renderer, renderer->framebuffer);
    SDL_SetRenderDrawColor(renderer->renderer, 10, 12, 18, 255);
    SDL_RenderClear(renderer->renderer);
}

void widebrim_renderer_end_frame(widebrim_renderer *renderer) {
    if (renderer == NULL || !renderer->initialized || renderer->renderer == NULL) {
        return;
    }

    SDL_SetRenderTarget(renderer->renderer, NULL);
    SDL_RenderClear(renderer->renderer);

    SDL_FRect dst = { 0.0f, 0.0f, 640.0f, 768.0f };
    SDL_RenderTexture(renderer->renderer, renderer->framebuffer, NULL, &dst);
    SDL_RenderPresent(renderer->renderer);
}

void widebrim_renderer_draw_debug_screen(widebrim_renderer *renderer,
                                        widebrim_mode_kind mode,
                                        uint32_t frame_counter) {
    if (renderer == NULL || !renderer->initialized || renderer->renderer == NULL) {
        return;
    }

    const int screen_top = 0;
    const int screen_bottom = WIDEBRIM_SCREEN_HEIGHT;
    const int screen_w = WIDEBRIM_SCREEN_WIDTH;
    const int screen_h = WIDEBRIM_SCREEN_HEIGHT;

    widebrim_draw_screen_region(renderer->renderer, 0, screen_top, screen_w, screen_h, 24, 28, 38, 255);
    widebrim_draw_screen_region(renderer->renderer, 0, screen_bottom, screen_w, screen_h, 30, 40, 52, 255);

    SDL_SetRenderDrawColor(renderer->renderer, 255, 255, 255, 255);
    SDL_FRect divider = { 0.0f, (float)screen_h, (float)screen_w, 2.0f };
    SDL_RenderFillRect(renderer->renderer, &divider);

    char label[96];
    const char *mode_name = widebrim_mode_kind_to_string(mode);

    snprintf(label,
             sizeof(label),
             "mode:%s frame:%u",
             mode_name != NULL ? mode_name : "UNKNOWN",
             (unsigned)frame_counter);

    SDL_SetRenderDrawColor(renderer->renderer, 235, 220, 160, 255);
    SDL_RenderDebugText(renderer->renderer, 8, 10, label);
    SDL_RenderDebugText(renderer->renderer, 8, screen_h + 44, "main screen");
    SDL_RenderDebugText(renderer->renderer, 8, screen_h + 120, "sub screen");
}

void widebrim_renderer_draw_room(widebrim_renderer *renderer,
                                const widebrim_room *room,
                                const widebrim_madhatter *madhatter,
                                uint32_t frame_counter) {
    uint32_t scene_hash = 2166136261u;
    const mh_archive_entry *entry = NULL;
    mh_buffer decompressed = {0};
    const uint8_t *scene_payload = NULL;
    size_t scene_payload_len = 0u;

    if (renderer == NULL || !renderer->initialized || renderer->renderer == NULL || room == NULL) {
        return;
    }

    const int room_w = WIDEBRIM_SCREEN_WIDTH;
    const int room_h = WIDEBRIM_SCREEN_HEIGHT;
    const int screen_gap = 0;

    if (madhatter != NULL && madhatter->ready) {
        const char *scene_name = room->name;
        const char *leaf = strrchr(scene_name, '/');
        const char *basename = leaf != NULL ? leaf + 1 : scene_name;

        fprintf(stderr, "DEBUG room: resolving scene=%s\n", scene_name);
        entry = widebrim_madhatter_get_file((widebrim_madhatter *)madhatter, scene_name);
        if (entry == NULL && basename != NULL && basename != scene_name) {
            entry = widebrim_madhatter_get_file((widebrim_madhatter *)madhatter, basename);
        }

        if (entry != NULL) {
            fprintf(stderr,
                    "DEBUG room: found entry=%s asset_len=%zu\n",
                    basename != NULL ? basename : scene_name,
                    entry->asset.len);
            scene_hash = widebrim_hash_archive_entry(entry);
            if (mh_file_decompress_detected(entry->asset.data, entry->asset.len, &decompressed) == 0 &&
                decompressed.data != NULL && decompressed.len > 0u) {
                scene_hash = widebrim_hash_bytes(decompressed.data, decompressed.len);
                scene_payload = decompressed.data;
                scene_payload_len = decompressed.len;
                fprintf(stderr,
                        "DEBUG room: decompressed scene to %zu bytes\n",
                        scene_payload_len);
            } else {
                scene_payload = entry->asset.data;
                scene_payload_len = entry->asset.len;
                fprintf(stderr,
                        "DEBUG room: using raw asset bytes (%zu bytes, decompression failed)\n",
                        scene_payload_len);
            }
        } else {
            fprintf(stderr, "DEBUG room: no matching asset entry found for %s\n", scene_name);
        }
    } else {
        fprintf(stderr, "DEBUG room: madhatter not ready\n");
    }

    if (entry == NULL) {
        scene_hash = widebrim_hash_string(room->name);
    }

    widebrim_draw_room_scene_layer(renderer->renderer,
                                  0,
                                  0,
                                  room_w,
                                  room_h,
                                  room->bg_r,
                                  room->bg_g,
                                  room->bg_b,
                                  room->accent_r,
                                  room->accent_g,
                                  room->accent_b,
                                  room->name,
                                  scene_hash,
                                  scene_payload,
                                  scene_payload_len);

    mh_buffer_free(&decompressed);

    SDL_SetRenderDrawColor(renderer->renderer, 255, 255, 255, 255);
    SDL_FRect hotspot = { (float)room->hotspot_x - 12.0f,
                          (float)room->hotspot_y - 12.0f,
                          24.0f,
                          24.0f };
    SDL_RenderFillRect(renderer->renderer, &hotspot);

    SDL_SetRenderDrawColor(renderer->renderer, 18, 18, 18, 220);
    SDL_FRect bottom_panel = { 0.0f, (float)(room_h + screen_gap), (float)room_w, (float)room_h };
    SDL_RenderFillRect(renderer->renderer, &bottom_panel);

    SDL_SetRenderDrawColor(renderer->renderer, room->accent_r, room->accent_g, room->accent_b, 190);
    SDL_FRect bottom_accent = { 20.0f, (float)(room_h + 20), 216.0f, 84.0f };
    SDL_RenderFillRect(renderer->renderer, &bottom_accent);

    SDL_SetRenderDrawColor(renderer->renderer, 245, 240, 220, 255);
    const char *leaf = strrchr(room->name, '/');
    const char *scene_asset = leaf != NULL ? leaf + 1 : room->name;
    char label[128];
    snprintf(label, sizeof(label), "asset:%s frame:%u", scene_asset, (unsigned)frame_counter);
    SDL_RenderDebugText(renderer->renderer, 12, 12, label);
    SDL_RenderDebugText(renderer->renderer, 12, room_h + 28, "top screen");
    SDL_RenderDebugText(renderer->renderer, 12, room_h + 112, scene_asset);
    SDL_RenderDebugText(renderer->renderer, 12, room_h + 136, "bottom screen");

    if (entry != NULL) {
        char payload_note[160];
        snprintf(payload_note,
                 sizeof(payload_note),
                 "payload:%zu bytes",
                 entry->asset.len);
        SDL_RenderDebugText(renderer->renderer, 12, room_h + 152, payload_note);
    }
}
