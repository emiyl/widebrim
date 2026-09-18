#include "madhatter_bridge.h"

#include <stdio.h>
#include <stdlib.h>

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
