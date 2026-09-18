#include <stdio.h>
#include <string.h>

#include "runtime.h"

static void print_usage(const char *argv0) {
    printf("Usage: %s --datafiles <path-to-Datafiles-root> [--language en]\n", argv0);
}

int main(int argc, char **argv) {
    const char *datafiles_root = NULL;
    const char *language = "en";
    widebrim_runtime runtime;
    int i;

    for (i = 1; i < argc; ++i) {
        if ((strcmp(argv[i], "--datafiles") == 0) && i + 1 < argc) {
            datafiles_root = argv[++i];
        } else if ((strcmp(argv[i], "--language") == 0) && i + 1 < argc) {
            language = argv[++i];
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }

    if (!datafiles_root) {
        fprintf(stderr, "widebrim: --datafiles <path> is required\n");
        print_usage(argv[0]);
        return 1;
    }

    if (widebrim_runtime_init(&runtime, datafiles_root, language) != 0) {
        return 1;
    }

    widebrim_runtime_run(&runtime);

    /* Matches launcher.py's crash reporter (mode/place snapshot), printed
     * unconditionally here on the normal quit path since there is no
     * exception mechanism in C to hook a crash handler onto. */
    fprintf(stderr, "widebrim: exiting (mode=%d, place_num=%d)\n",
            (int)game_state_get_mode(&runtime.state), game_state_get_place_num(&runtime.state));

    widebrim_runtime_destroy(&runtime);
    return 0;
}
