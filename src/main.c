#include <SDL3/SDL.h>

#include <dirent.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "engine.h"

static int widebrim_runtime_try_magic_pack_load(widebrim_runtime *runtime,
                                              const uint8_t *data,
                                              size_t len,
                                              int version) {
    size_t i;
    int pack2_seen = 0;
    int legacy_seen = 0;
    const size_t max_header_scan = len < 64u ? len : 64u;

    if (runtime == NULL || data == NULL || len == 0) {
        return -1;
    }

    for (i = 0; i + 4u <= max_header_scan; ++i) {
        if ((i == 0u || i == 12u || i == 14u || i == 16u || i == 20u) &&
            (memcmp(data + i, "PCK2", 4u) == 0 || memcmp(data + i, "LPC2", 4u) == 0)) {
            pack2_seen = 1;
            break;
        }
    }

    if (pack2_seen) {
        if (widebrim_madhatter_load_layton_pack2(&runtime->state.madhatter, data, len) == 0) {
            return 0;
        }
        fprintf(stderr, "Detected a pack2-style header in the Datafiles payload, but the native Madhatter parser rejected it. Fallback to raw registration is still enabled for compatibility.\n");
    }

    for (i = 0; i + 4u <= max_header_scan; ++i) {
        if (memcmp(data + i, "LPCK", 4u) == 0 && (i == 0u || i == 8u || i == 12u || i == 16u)) {
            legacy_seen = 1;
            break;
        }
    }

    if (legacy_seen || version == 0 || version == 1) {
        int legacy_version = version;
        if (legacy_version != 0 && legacy_version != 1) {
            legacy_version = 1;
        }

        if (widebrim_madhatter_load_layton_pack(&runtime->state.madhatter, data, len, legacy_version) == 0) {
            return 0;
        }
    }

    return -1;
}

int widebrim_runtime_load_pack_data(widebrim_runtime *runtime,
                                   const uint8_t *data,
                                   size_t len,
                                   int version) {
    if (runtime == NULL || data == NULL || len == 0) {
        return -1;
    }

    return widebrim_runtime_try_magic_pack_load(runtime, data, len, version);
}

int widebrim_runtime_load_pack_from_path(widebrim_runtime *runtime,
                                       const char *path,
                                       int version) {
    FILE *fp = NULL;
    long file_size = 0;
    uint8_t *buffer = NULL;
    size_t bytes_read = 0;
    int result = -1;

    if (runtime == NULL || path == NULL) {
        return -1;
    }

    fp = fopen(path, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Unable to open pack: %s\n", path);
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

    result = widebrim_runtime_load_pack_data(runtime, buffer, bytes_read, version);
    if (result != 0) {
        const char *basename = strrchr(path, '/');
        const char *entry_name = basename != NULL ? basename + 1 : path;
        result = widebrim_madhatter_load_file(&runtime->state.madhatter,
                                            entry_name,
                                            buffer,
                                            bytes_read);
    }

    free(buffer);
    fclose(fp);

    if (result == 0) {
        printf("Loaded asset payload: %s\n", path);
    } else {
        fprintf(stderr, "Pack load failed: %s\n", path);
    }

    return result;
}

void widebrim_runtime_init(widebrim_runtime *runtime) {
    if (runtime == NULL) {
        return;
    }

    memset(runtime, 0, sizeof(*runtime));
    widebrim_renderer_init(&runtime->renderer, "widebrim-c");
    widebrim_game_state_init(&runtime->state);
    widebrim_mode_manager_init(&runtime->modes);
    if (runtime->modes.has_current && runtime->modes.current.init != NULL) {
        runtime->modes.current.init(&runtime->modes.current, &runtime->state);
    }
    runtime->running = true;
}

void widebrim_runtime_destroy(widebrim_runtime *runtime) {
    if (runtime == NULL) {
        return;
    }

    widebrim_game_state_destroy(&runtime->state);
    widebrim_renderer_destroy(&runtime->renderer);
    runtime->running = false;
}

void widebrim_runtime_run(widebrim_runtime *runtime) {
    if (runtime == NULL) {
        return;
    }

    Uint64 last_ticks = SDL_GetTicks();
    while (runtime->running) {
        Uint64 now = SDL_GetTicks();
        float dt = (float)(now - last_ticks) / 1000.0f;
        last_ticks = now;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    runtime->running = false;
                    break;
                case SDL_EVENT_KEY_DOWN:
                    if (event.key.key == SDL_SCANCODE_ESCAPE) {
                        runtime->running = false;
                    } else if (event.key.key == SDL_SCANCODE_RETURN ||
                               event.key.key == SDL_SCANCODE_SPACE) {
                        switch (runtime->state.current_mode) {
                            case WIDEBRIM_MODE_TITLE:
                                widebrim_game_state_set_mode(&runtime->state, WIDEBRIM_MODE_ROOM);
                                break;
                            case WIDEBRIM_MODE_ROOM:
                                widebrim_game_state_set_mode(&runtime->state, WIDEBRIM_MODE_EVENT);
                                break;
                            case WIDEBRIM_MODE_EVENT:
                                widebrim_game_state_set_mode(&runtime->state, WIDEBRIM_MODE_TITLE);
                                break;
                            default:
                                break;
                        }
                    }
                    break;
                default:
                    break;
            }
        }

        widebrim_mode_manager_update(&runtime->modes, &runtime->state, dt);

        widebrim_renderer_begin_frame(&runtime->renderer);
        widebrim_mode_manager_draw(&runtime->modes, &runtime->state, &runtime->renderer);
        widebrim_renderer_end_frame(&runtime->renderer);

        runtime->state.frame_counter++;
    }
}

static int widebrim_runtime_contains_magic(const unsigned char *data,
                                          size_t len,
                                          const char *magic,
                                          size_t magic_len) {
    size_t i;
    const size_t max_scan = len < 64u ? len : 64u;

    if (data == NULL || magic == NULL || magic_len == 0) {
        return 0;
    }

    for (i = 0; i + magic_len <= max_scan; ++i) {
        if ((i == 0u || i == 8u || i == 12u || i == 14u || i == 16u || i == 20u) &&
            memcmp(data + i, magic, magic_len) == 0) {
            return 1;
        }
    }

    return 0;
}

static int widebrim_runtime_has_pack_extension(const char *path) {
    const char *ext = NULL;

    if (path == NULL) {
        return 0;
    }

    ext = strrchr(path, '.');
    if (ext == NULL) {
        return 0;
    }

    return (strcmp(ext, ".plz") == 0 || strcmp(ext, ".lt2") == 0 ||
            strcmp(ext, ".pack") == 0 || strcmp(ext, ".pck") == 0 ||
            strcmp(ext, ".pck2") == 0 || strcmp(ext, ".lpc2") == 0);
}

static int widebrim_runtime_file_is_pack_candidate(const char *path) {
    FILE *fp = NULL;
    unsigned char header[64];
    size_t bytes_read = 0;
    const char *magic_names[] = {"LPCK", "PCK2", "LPC2"};
    size_t magic_count = sizeof(magic_names) / sizeof(magic_names[0]);

    if (path == NULL || widebrim_runtime_has_pack_extension(path) == 0) {
        return 0;
    }

    fp = fopen(path, "rb");
    if (fp == NULL) {
        return 0;
    }

    bytes_read = fread(header, 1, sizeof(header), fp);
    fclose(fp);

    if (bytes_read < 4) {
        return 0;
    }

    for (size_t i = 0; i < magic_count; ++i) {
        if (widebrim_runtime_contains_magic(header, bytes_read, magic_names[i], strlen(magic_names[i]))) {
            return 1;
        }
    }

    return 0;
}

static int widebrim_runtime_load_pack_candidate(widebrim_runtime *runtime,
                                              const char *candidate,
                                              int version) {
    if (candidate == NULL) {
        return -1;
    }

    if (widebrim_runtime_file_is_pack_candidate(candidate) == 0 &&
        (strstr(candidate, ".plz") == NULL && strstr(candidate, ".lt2") == NULL &&
         strstr(candidate, ".pack") == NULL && strstr(candidate, ".pck") == NULL)) {
        fprintf(stderr, "Candidate is not a pack file: %s\n", candidate);
        return -1;
    }

    if (widebrim_runtime_load_pack_from_path(runtime, candidate, version) == 0) {
        return 0;
    }

    fprintf(stderr, "Candidate pack file rejected: %s\n", candidate);
    return -1;
}

static int widebrim_runtime_load_asset_file(widebrim_runtime *runtime,
                                          const char *archive_name,
                                          const char *file_path) {
    FILE *fp = NULL;
    long file_size = 0;
    uint8_t *buffer = NULL;
    size_t bytes_read = 0;
    int result = -1;

    if (runtime == NULL || archive_name == NULL || file_path == NULL) {
        return -1;
    }

    fp = fopen(file_path, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Unable to open asset file: %s\n", file_path);
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

    buffer = (uint8_t *)malloc((size_t)file_size + 1u);
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

    buffer[file_size] = '\0';
    result = widebrim_madhatter_load_file(&runtime->state.madhatter,
                                         archive_name,
                                         buffer,
                                         bytes_read);
    free(buffer);
    fclose(fp);

    if (result == 0) {
        printf("Loaded asset: %s\n", archive_name);
    }

    return result;
}

static int widebrim_runtime_load_datafiles_tree(widebrim_runtime *runtime,
                                              const char *root_path,
                                              const char *scan_root) {
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    struct stat st;
    char child_path[PATH_MAX];
    char archive_name[PATH_MAX];
    size_t root_len = 0;
    int saw_file = 0;

    if (runtime == NULL || root_path == NULL || scan_root == NULL) {
        return -1;
    }

    if (stat(root_path, &st) != 0 || !S_ISDIR(st.st_mode)) {
        return -1;
    }

    root_len = strlen(scan_root);
    dir = opendir(root_path);
    if (dir == NULL) {
        fprintf(stderr, "Unable to open Datafiles tree: %s\n", root_path);
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        const char *name = entry->d_name;

        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            continue;
        }

        if (strcmp(name, ".DS_Store") == 0) {
            continue;
        }

        snprintf(child_path, sizeof(child_path), "%s/%s", root_path, name);
        if (stat(child_path, &st) != 0) {
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            if (widebrim_runtime_load_datafiles_tree(runtime, child_path, scan_root) == 0) {
                saw_file = 1;
            }
            continue;
        }

        if (S_ISREG(st.st_mode)) {
            if (strstr(name, ".DS_Store") != NULL) {
                continue;
            }

            if (widebrim_runtime_file_is_pack_candidate(child_path)) {
                snprintf(archive_name,
                         sizeof(archive_name),
                         "%s",
                         child_path + root_len + 1);
                if (archive_name[0] != '\0' &&
                    widebrim_runtime_load_pack_candidate(runtime, child_path, 1) == 0) {
                    saw_file = 1;
                    return 0;
                }
            }

            snprintf(archive_name,
                     sizeof(archive_name),
                     "%s",
                     child_path + root_len + 1);
            if (archive_name[0] == '\0') {
                continue;
            }

            if (widebrim_runtime_load_asset_file(runtime, archive_name, child_path) == 0) {
                saw_file = 1;
            }
        }
    }

    closedir(dir);
    return saw_file ? 0 : -1;
}

static int widebrim_runtime_load_pack_arg(widebrim_runtime *runtime,
                                         const char *path,
                                         int version) {
    struct stat st;
    char candidate_path[PATH_MAX];
    char datafiles_path[PATH_MAX];
    DIR *dir = NULL;
    struct dirent *entry = NULL;

    if (runtime == NULL || path == NULL) {
        return -1;
    }

    if (stat(path, &st) != 0) {
        fprintf(stderr, "Pack path does not exist: %s\n", path);
        return -1;
    }

    if (S_ISDIR(st.st_mode)) {
        const char *scan_root = path;

        snprintf(datafiles_path, sizeof(datafiles_path), "%s/Datafiles", path);
        if (stat(datafiles_path, &st) == 0 && S_ISDIR(st.st_mode)) {
            scan_root = datafiles_path;
        }

        if (scan_root == path || strcmp(path, scan_root) == 0 || strcmp(path, datafiles_path) != 0) {
            printf("Loading extracted Datafiles tree: %s\n", scan_root);
            if (widebrim_runtime_load_datafiles_tree(runtime, scan_root, scan_root) == 0) {
                return 0;
            }
        }

        dir = opendir(path);
        if (dir == NULL) {
            fprintf(stderr, "Unable to open pack directory: %s\n", path);
            return -1;
        }

        while ((entry = readdir(dir)) != NULL) {
            const char *name = entry->d_name;
            const char *ext = strrchr(name, '.');

            if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
                continue;
            }

            if (ext != NULL && strcmp(ext, ".DS_Store") == 0) {
                continue;
            }

            if (ext == NULL) {
                continue;
            }

            if (strcmp(ext, ".bin") == 0 || strcmp(ext, ".pack") == 0 ||
                strcmp(ext, ".dat") == 0 || strcmp(ext, ".lt2") == 0 ||
                strcmp(ext, ".plz") == 0 || strcmp(ext, ".pck") == 0 ||
                strcmp(ext, ".lpc2") == 0 || strcmp(ext, ".pck2") == 0) {
                snprintf(candidate_path,
                         sizeof(candidate_path),
                         "%s/%s",
                         path,
                         name);
                if (widebrim_runtime_load_pack_candidate(runtime, candidate_path, version) == 0) {
                    closedir(dir);
                    return 0;
                }
            }
        }

        closedir(dir);
        fprintf(stderr, "No valid pack candidate found under: %s\n", path);
        return -1;
    }

    return widebrim_runtime_load_pack_candidate(runtime, path, version);
}

int main(int argc, char **argv) {
    const char *pack_path = NULL;
    int version = 0;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: %s [--pack <path>] [--version <n>] [<pack_path>]\n", argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--pack") == 0 && i + 1 < argc) {
            pack_path = argv[++i];
        } else if (strcmp(argv[i], "--version") == 0 && i + 1 < argc) {
            version = atoi(argv[++i]);
        } else if (pack_path == NULL && argv[i][0] != '-') {
            pack_path = argv[i];
        }
    }

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
        return 1;
    }

    widebrim_runtime runtime;
    widebrim_runtime_init(&runtime);

    printf("widebrim C port initialized with SDL3 and Madhatter\n");
    printf("SDL version: %d.%d.%d\n",
           SDL_MAJOR_VERSION,
           SDL_MINOR_VERSION,
           SDL_MICRO_VERSION);

    if (pack_path != NULL) {
        printf("Attempting to load pack path: %s\n", pack_path);
        if (widebrim_runtime_load_pack_arg(&runtime, pack_path, version) != 0) {
            fprintf(stderr, "Pack path rejected: %s\n", pack_path);
            fprintf(stderr, "This usually means the file is a real Datafiles pack with a PCK2/LPC2 signature that the current Madhatter parser rejects.\n");
        }
    } else {
        printf("No Layton pack supplied; running in debug bootstrap mode.\n");
    }

    widebrim_runtime_run(&runtime);

    widebrim_runtime_destroy(&runtime);
    SDL_Quit();
    return 0;
}
