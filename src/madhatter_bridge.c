#include "madhatter_bridge.h"

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
