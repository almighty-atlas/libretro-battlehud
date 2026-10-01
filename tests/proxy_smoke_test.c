#define _GNU_SOURCE

#include "libretro.h"

#include <dlfcn.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned (*retro_api_version_fn)(void);
typedef void (*retro_set_environment_fn)(retro_environment_t);
typedef void (*retro_set_video_refresh_fn)(retro_video_refresh_t);
typedef void (*retro_set_audio_sample_fn)(retro_audio_sample_t);
typedef void (*retro_set_audio_sample_batch_fn)(retro_audio_sample_batch_t);
typedef void (*retro_set_input_poll_fn)(retro_input_poll_t);
typedef void (*retro_set_input_state_fn)(retro_input_state_t);
typedef void (*retro_get_system_info_fn)(struct retro_system_info *);
typedef void (*retro_init_fn)(void);
typedef bool (*retro_load_game_fn)(const struct retro_game_info *);
typedef void (*retro_run_fn)(void);
typedef size_t (*retro_serialize_size_fn)(void);
typedef bool (*retro_serialize_fn)(void *, size_t);
typedef bool (*retro_unserialize_fn)(const void *, size_t);
typedef void *(*retro_get_memory_data_fn)(unsigned);
typedef size_t (*retro_get_memory_size_fn)(unsigned);
typedef void (*retro_unload_game_fn)(void);
typedef void (*retro_deinit_fn)(void);

static int environment_calls;
static int video_calls;
static int audio_calls;
static int input_poll_calls;
static int input_state_calls;
static int sample_calls;

static bool frontend_environment(unsigned cmd, void *data)
{
    if (cmd == RETRO_ENVIRONMENT_SET_PIXEL_FORMAT) {
        enum retro_pixel_format *format = data;
        if (!format || *format != RETRO_PIXEL_FORMAT_XRGB8888)
            return false;
    }

    environment_calls++;
    return true;
}

static void frontend_video(const void *data, unsigned width,
                           unsigned height, size_t pitch)
{
    if (width != 160 || height != 144 || pitch != 164 * sizeof(uint32_t) ||
        (video_calls % 2 == 0 ? !data : data != NULL)) {
        fprintf(stderr, "unexpected video frame\n");
        exit(20);
    }
    if (data) {
        const unsigned char *bytes = data;
        for (size_t i = 0; i < pitch * height; ++i)
            if (bytes[i] != 0x5a) {
                fprintf(stderr, "frame bytes or padding changed\n");
                exit(22);
            }
    }
    video_calls++;
}

static size_t frontend_audio_batch(const int16_t *data, size_t frames)
{
    if (!data || frames != 1) {
        fprintf(stderr, "unexpected audio batch\n");
        exit(21);
    }
    audio_calls++;
    return 0; /* Backpressure must reach the backend unchanged. */
}

static void frontend_audio_sample(int16_t left, int16_t right)
{
    if (left != -123 || right != 456)
        exit(23);
    sample_calls++;
}

static void frontend_input_poll(void)
{
    input_poll_calls++;
}

static int16_t frontend_input_state(unsigned port, unsigned device,
                                    unsigned index, unsigned id)
{
    (void)port;
    (void)device;
    (void)index;
    (void)id;
    input_state_calls++;
    return 7;
}

static void *required_symbol(void *handle, const char *name)
{
    void *symbol = dlsym(handle, name);
    if (!symbol) {
        fprintf(stderr, "missing wrapper symbol %s: %s\n", name, dlerror());
        exit(10);
    }
    return symbol;
}

#define RESOLVE(variable, type, symbol_name)                                   \
    type variable = NULL;                                                      \
    do {                                                                       \
        void *symbol__ = required_symbol(wrapper, symbol_name);                \
        memcpy(&variable, &symbol__, sizeof(symbol__));                        \
    } while (0)

int main(int argc, char **argv)
{
    void *wrapper;
    struct retro_system_info info;
    struct retro_game_info game = {0};
    uint8_t rom_byte = 0;
    uint8_t serialized[4];
    uint8_t *save_ram;

    const char *mode = argc == 4 ? argv[3] : "normal";

    if (argc != 3 && argc != 4) {
        fprintf(stderr, "usage: %s WRAPPER BACKEND\n", argv[0]);
        return 2;
    }

    if (setenv("LIBRETRO_BATTLEHUD_BACKEND", argv[2], 1) != 0) {
        perror("setenv");
        return 3;
    }

    if (!strcmp(mode, "adjacent"))
        unsetenv("LIBRETRO_BATTLEHUD_BACKEND");
    else if (!strcmp(mode, "recovery"))
        setenv("LIBRETRO_BATTLEHUD_BACKEND", "/nonexistent/battlehud-backend.so", 1);

    wrapper = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!wrapper) {
        fprintf(stderr, "failed to open wrapper: %s\n", dlerror());
        return 4;
    }

    RESOLVE(api_version, retro_api_version_fn, "retro_api_version");
    RESOLVE(set_environment, retro_set_environment_fn, "retro_set_environment");
    RESOLVE(set_video_refresh, retro_set_video_refresh_fn, "retro_set_video_refresh");
    RESOLVE(set_audio_sample, retro_set_audio_sample_fn, "retro_set_audio_sample");
    RESOLVE(set_audio_sample_batch, retro_set_audio_sample_batch_fn, "retro_set_audio_sample_batch");
    RESOLVE(set_input_poll, retro_set_input_poll_fn, "retro_set_input_poll");
    RESOLVE(set_input_state, retro_set_input_state_fn, "retro_set_input_state");
    RESOLVE(get_system_info, retro_get_system_info_fn, "retro_get_system_info");
    RESOLVE(core_init, retro_init_fn, "retro_init");
    RESOLVE(load_game, retro_load_game_fn, "retro_load_game");
    RESOLVE(run, retro_run_fn, "retro_run");
    RESOLVE(serialize_size, retro_serialize_size_fn, "retro_serialize_size");
    RESOLVE(serialize, retro_serialize_fn, "retro_serialize");
    RESOLVE(unserialize, retro_unserialize_fn, "retro_unserialize");
    RESOLVE(get_memory_data, retro_get_memory_data_fn, "retro_get_memory_data");
    RESOLVE(get_memory_size, retro_get_memory_size_fn, "retro_get_memory_size");
    RESOLVE(unload_game, retro_unload_game_fn, "retro_unload_game");
    RESOLVE(core_deinit, retro_deinit_fn, "retro_deinit");

    if (!strcmp(mode, "failure")) {
        get_system_info(&info);
        if (strcmp(info.library_version, "backend unavailable") ||
            load_game(&game) || serialize_size() ||
            get_memory_data(RETRO_MEMORY_SAVE_RAM) ||
            get_memory_size(RETRO_MEMORY_SAVE_RAM))
            return 40;
        run();
        core_deinit();
        dlclose(wrapper);
        puts("M0 invalid backend rejected");
        return 0;
    }

    if (api_version() != RETRO_API_VERSION) {
        fprintf(stderr, "API version was not forwarded\n");
        return 30;
    }

    set_environment(frontend_environment);
    set_video_refresh(frontend_video);
    set_audio_sample_batch(frontend_audio_batch);
    set_input_poll(frontend_input_poll);
    set_input_state(frontend_input_state);

    if (!strcmp(mode, "recovery"))
        setenv("LIBRETRO_BATTLEHUD_BACKEND", argv[2], 1);

    memset(&info, 0, sizeof(info));
    get_system_info(&info);
    if (!info.library_name ||
        strcmp(info.library_name, "BattleHUD Fake Backend") != 0) {
        fprintf(stderr, "system info was not forwarded\n");
        return 31;
    }

    core_init();

    game.data = &rom_byte;
    game.size = 1;
    if (!load_game(&game)) {
        fprintf(stderr, "load_game was not forwarded\n");
        return 32;
    }

    run();

    run(); /* NULL duplicate frame with identical geometry/pitch. */
    if (environment_calls < 1 || video_calls != 2 || audio_calls != 2 ||
        input_poll_calls != 2 || input_state_calls != 2) {
        fprintf(stderr,
                "callback forwarding failed env=%d video=%d audio=%d poll=%d state=%d\n",
                environment_calls, video_calls, audio_calls,
                input_poll_calls, input_state_calls);
        return 33;
    }

    if (serialize_size() != 4 ||
        !serialize(serialized, sizeof(serialized)) ||
        memcmp(serialized, "M0OK", 4) != 0 ||
        !unserialize(serialized, sizeof(serialized))) {
        fprintf(stderr, "serialization was not forwarded\n");
        return 34;
    }

    if (get_memory_size(RETRO_MEMORY_SAVE_RAM) != 8192) {
        fprintf(stderr, "save RAM size was not forwarded\n");
        return 35;
    }

    save_ram = get_memory_data(RETRO_MEMORY_SAVE_RAM);
    if (!save_ram) {
        fprintf(stderr, "save RAM pointer was not forwarded\n");
        return 36;
    }
    if (save_ram[1] != 7 || save_ram[2] != 0)
        return 41; /* Input return value reached the backend. */
    set_audio_sample(frontend_audio_sample);
    set_audio_sample_batch(NULL);
    run();
    if (sample_calls != 1 || audio_calls != 2)
        return 42; /* Unsetting batch must enable single-sample fallback. */
    set_video_refresh(NULL);
    set_input_poll(NULL);
    set_input_state(NULL);
    set_audio_sample(NULL);
    run();
    if (video_calls != 3 || input_poll_calls != 3 || input_state_calls != 3 ||
        sample_calls != 1 || audio_calls != 2)
        return 43;
    save_ram[0] = 0x42;
    if (((uint8_t *)get_memory_data(RETRO_MEMORY_SAVE_RAM))[0] != 0x42) {
        fprintf(stderr, "save RAM access was not transparent\n");
        return 37;
    }

    unload_game();
    core_deinit();
    /* The frontend can retain the wrapper, but the backend must be released. */
    if (strcmp(mode, "adjacent")) {
        void *retained = dlopen(argv[2], RTLD_NOW | RTLD_NOLOAD);
        if (retained) {
            dlclose(retained);
            return 44;
        }
    }
    int previous_env = environment_calls;
    core_init();
    if (environment_calls != previous_env + 1)
        return 45; /* Environment registration survives unload/reinit. */
    if (!load_game(&game))
        return 46;
    run(); /* All explicitly disabled callbacks must remain disabled. */
    if (video_calls != 3 || sample_calls != 1 || input_poll_calls != 3)
        return 47;
    unload_game();
    core_deinit();
    dlclose(wrapper);

    puts("M0 proxy smoke test passed");
    return 0;
}

#undef RESOLVE
