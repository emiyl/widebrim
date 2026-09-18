#include "madhatter_bridge.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int widebrim_madhatter_has_magic(const uint8_t *data,
                                        size_t len,
                                        const char *magic,
                                        size_t magic_len) {
    size_t i;
    const size_t max_scan = len < 64u ? len : 64u;

    if (data == NULL || magic == NULL || magic_len == 0u) {
        return 0;
    }

    for (i = 0u; i + magic_len <= max_scan; ++i) {
        if (memcmp(data + i, magic, magic_len) == 0) {
            return 1;
        }
    }

    return 0;
}

int widebrim_madhatter_init(widebrim_madhatter *ctx) {
    if (ctx == NULL) {
        return -1;
    }

    if (mh_archive_init(&ctx->archive) != 0) {
        return -1;
    }

    ctx->ready = 1;
    return 0;
}

void widebrim_madhatter_free(widebrim_madhatter *ctx) {
    if (ctx == NULL) {
        return;
    }

    if (ctx->ready) {
        mh_archive_free(&ctx->archive);
        ctx->ready = 0;
    }
}

int widebrim_madhatter_load_file(widebrim_madhatter *ctx,
                                const char *name,
                                const uint8_t *data,
                                size_t len) {
    if (ctx == NULL || name == NULL || data == NULL || len == 0) {
        return -1;
    }

    if (!ctx->ready) {
        if (widebrim_madhatter_init(ctx) != 0) {
            return -1;
        }
    }

    return mh_archive_add_file(&ctx->archive, name, data, len);
}

int widebrim_madhatter_load_file_from_path(widebrim_madhatter *ctx,
                                         const char *path,
                                         const char *archive_name) {
    FILE *fp = NULL;
    long file_size = 0;
    uint8_t *buffer = NULL;
    size_t bytes_read = 0;
    int result = -1;

    if (ctx == NULL || path == NULL || archive_name == NULL) {
        return -1;
    }

    fp = fopen(path, "rb");
    if (fp == NULL) {
        return -1;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return -1;
    }

    file_size = ftell(fp);
    if (file_size < 0) {
        fclose(fp);
        return -1;
    }

    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return -1;
    }

    buffer = (uint8_t *)malloc((size_t)file_size);
    if (buffer == NULL) {
        fclose(fp);
        return -1;
    }

    bytes_read = fread(buffer, 1, (size_t)file_size, fp);
    if (bytes_read != (size_t)file_size) {
        free(buffer);
        fclose(fp);
        return -1;
    }

    result = widebrim_madhatter_load_file(ctx, archive_name, buffer, bytes_read);

    free(buffer);
    fclose(fp);
    return result;
}

int widebrim_madhatter_load_layton_pack(widebrim_madhatter *ctx,
                                       const uint8_t *data,
                                       size_t len,
                                       int version) {
    if (ctx == NULL || data == NULL || len == 0) {
        return -1;
    }

    if (!ctx->ready) {
        if (widebrim_madhatter_init(ctx) != 0) {
            return -1;
        }
    }

    return mh_archive_load_layton_pack(&ctx->archive, data, len, version);
}

int widebrim_madhatter_load_layton_pack2(widebrim_madhatter *ctx,
                                        const uint8_t *data,
                                        size_t len) {
    if (ctx == NULL || data == NULL || len == 0) {
        return -1;
    }

    if (!ctx->ready) {
        if (widebrim_madhatter_init(ctx) != 0) {
            return -1;
        }
    }

    return mh_archive_load_layton_pack2(&ctx->archive, data, len);
}

int widebrim_madhatter_load_pack(widebrim_madhatter *ctx,
                               const uint8_t *data,
                               size_t len,
                               int version) {
    int pack2_magic = 0;
    int legacy_magic = 0;

    if (ctx == NULL || data == NULL || len == 0) {
        return -1;
    }

    pack2_magic = widebrim_madhatter_has_magic(data, len, "LPC2", 4u) ||
                  widebrim_madhatter_has_magic(data, len, "PCK2", 4u);
    legacy_magic = widebrim_madhatter_has_magic(data, len, "LPCK", 4u);

    if (pack2_magic) {
        return widebrim_madhatter_load_layton_pack2(ctx, data, len);
    }

    if (legacy_magic || version == 0 || version == 1) {
        int legacy_version = version;
        if (legacy_version != 0 && legacy_version != 1) {
            legacy_version = 1;
        }
        return widebrim_madhatter_load_layton_pack(ctx, data, len, legacy_version);
    }

    return -1;
}

const mh_archive_entry *widebrim_madhatter_get_file(widebrim_madhatter *ctx,
                                                   const char *name) {
    if (ctx == NULL || name == NULL) {
        return NULL;
    }

    return mh_archive_get(&ctx->archive, name);
}
