#ifndef WIDEBRIM_MADHATTER_BRIDGE_H
#define WIDEBRIM_MADHATTER_BRIDGE_H

#include <stddef.h>
#include <stdint.h>

#include <madhatter/madhatter.h>

typedef struct {
    mh_archive archive;
    int ready;
} widebrim_madhatter;

int widebrim_madhatter_init(widebrim_madhatter *ctx);
void widebrim_madhatter_free(widebrim_madhatter *ctx);
int widebrim_madhatter_load_file(widebrim_madhatter *ctx,
                                const char *name,
                                const uint8_t *data,
                                size_t len);
int widebrim_madhatter_load_file_from_path(widebrim_madhatter *ctx,
                                         const char *path,
                                         const char *archive_name);
int widebrim_madhatter_load_pack(widebrim_madhatter *ctx,
                               const uint8_t *data,
                               size_t len,
                               int version);
int widebrim_madhatter_load_layton_pack(widebrim_madhatter *ctx,
                                       const uint8_t *data,
                                       size_t len,
                                       int version);
int widebrim_madhatter_load_layton_pack2(widebrim_madhatter *ctx,
                                        const uint8_t *data,
                                        size_t len);
const mh_archive_entry *widebrim_madhatter_get_file(widebrim_madhatter *ctx,
                                                   const char *name);

#endif
