#include <stdio.h>
#include <string.h>

#include "runtime.h"

static void print_usage(const char *argv0) {
    printf("Usage: %s --data <path-to-data-root> [--language en]\n", argv0);
}

int main(int argc, char **argv) {
    const char *data_root = NULL;
    const char *language = "en";
    game_version version = WB_GAME_LAYTON2;
    wb_runtime runtime;
    int i;

    for (i = 1; i < argc; ++i) {
        if ((strcmp(argv[i], "--data") == 0) && i + 1 < argc) {
            data_root = argv[++i];
        } else if ((strcmp(argv[i], "--version") == 0) && i + 1 < argc) {
            const char *version_str = argv[++i];
            if (strcmp(version_str, "layton1") == 0) {
                version = WB_GAME_LAYTON1;
            } else if (strcmp(version_str, "layton2") == 0) {
                version = WB_GAME_LAYTON2;
            } else if (strcmp(version_str, "layton3") == 0) {
                version = WB_GAME_LAYTON3;
            } else if (strcmp(version_str, "layton4") == 0) {
                version = WB_GAME_LAYTON4;
            } else {
                fprintf(stderr, "widebrim: unknown version '%s'\n", version_str);
                print_usage(argv[0]);
                return 1;
            }
        } else if ((strcmp(argv[i], "--language") == 0) && i + 1 < argc) {
            language = argv[++i];
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }

    if (!data_root) {
        fprintf(stderr, "widebrim: --data <path> is required\n");
        print_usage(argv[0]);
        return 1;
    }

    if (wb_runtime_init(&runtime, version, data_root, language) != 0) {
        return 1;
    }

    wb_runtime_run(&runtime);

    fprintf(stderr, "widebrim: exiting (mode=%d, place_num=%d)\n",
            (int)game_state_get_mode(&runtime.state), game_state_get_place_num(&runtime.state));

    wb_runtime_destroy(&runtime);
    return 0;
}
