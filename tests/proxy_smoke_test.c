#define _POSIX_C_SOURCE 200809L

#include "libretro.h"

#include <dlfcn.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int environment_calls;
static int video_calls;
static int audio_calls;
static int input_poll_calls;
static int input_state_calls;

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
    if (!data || width != 160 || height != 144 ||
        pitch != 160 * sizeof(uint32_t)) {
        fprintf(stderr, "unexpected video frame\n");
        exit(20);
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
    return frames;
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
    return 0;
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

#define RESOLVE(name, type)                                                    \
    type name;                                                                 \
    do {                                                                       \
        void *symbol__ = required_symbol(wrapper, #name);                      \
        memcpy(&name, &symbol__, sizeof(symbol__));                            \
    } while (0)

int main(int argc, char **argv)
{
    void *wrapper;
    struct retro_system_info info;
    struct retro_game_info game = {0};
    uint8_t rom_byte = 0;
    uint8_t serialized[4];
    uint8_t *save_ram;

    if (argc != 3) {
        fprintf(stderr, "usage: %s WRAPPER BACKEND\n", argv[0]);
        return 2;
    }

    if (setenv("LIBRETRO_BATTLEHUD_BACKEND", argv[2], 1) != 0) {
        perror("setenv");
        return 3;
    }

    wrapper = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!wrapper) {
        fprintf(stderr, "failed to open wrapper: %s\n", dlerror());
        return 4;
    }

    RESOLVE(retro_api_version, unsigned (*)(void));
    RESOLVE(retro_set_environment, void (*)(retro_environment_t));
    RESOLVE(retro_set_video_refresh, void (*)(retro_video_refresh_t));
    RESOLVE(retro_set_audio_sample_batch, void (*)(retro_audio_sample_batch_t));
    RESOLVE(retro_set_input_poll, void (*)(retro_input_poll_t));
    RESOLVE(retro_set_input_state, void (*)(retro_input_state_t));
    RESOLVE(retro_get_system_info, void (*)(struct retro_system_info *));
    RESOLVE(retro_init, void (*)(void));
    RESOLVE(retro_load_game, bool (*)(const struct retro_game_info *));
    RESOLVE(retro_run, void (*)(void));
    RESOLVE(retro_serialize_size, size_t (*)(void));
    RESOLVE(retro_serialize, bool (*)(void *, size_t));
    RESOLVE(retro_unserialize, bool (*)(const void *, size_t));
    RESOLVE(retro_get_memory_data, void *(*)(unsigned));
    RESOLVE(retro_get_memory_size, size_t (*)(unsigned));
    RESOLVE(retro_unload_game, void (*)(void));
    RESOLVE(retro_deinit, void (*)(void));

    if (retro_api_version() != RETRO_API_VERSION) {
        fprintf(stderr, "API version was not forwarded\n");
        return 30;
    }

    retro_set_environment(frontend_environment);
    retro_set_video_refresh(frontend_video);
    retro_set_audio_sample_batch(frontend_audio_batch);
    retro_set_input_poll(frontend_input_poll);
    retro_set_input_state(frontend_input_state);

    memset(&info, 0, sizeof(info));
    retro_get_system_info(&info);
    if (!info.library_name ||
        strcmp(info.library_name, "BattleHUD Fake Backend") != 0) {
        fprintf(stderr, "system info was not forwarded\n");
        return 31;
    }

    retro_init();

    game.data = &rom_byte;
    game.size = 1;
    if (!retro_load_game(&game)) {
        fprintf(stderr, "load_game was not forwarded\n");
        return 32;
    }

    retro_run();

    if (environment_calls < 1 || video_calls != 1 || audio_calls != 1 ||
        input_poll_calls != 1 || input_state_calls != 1) {
        fprintf(stderr,
                "callback forwarding failed env=%d video=%d audio=%d poll=%d state=%d\n",
                environment_calls, video_calls, audio_calls,
                input_poll_calls, input_state_calls);
        return 33;
    }

    if (retro_serialize_size() != 4 ||
        !retro_serialize(serialized, sizeof(serialized)) ||
        memcmp(serialized, "M0OK", 4) != 0 ||
        !retro_unserialize(serialized, sizeof(serialized))) {
        fprintf(stderr, "serialization was not forwarded\n");
        return 34;
    }

    if (retro_get_memory_size(RETRO_MEMORY_SAVE_RAM) != 8192) {
        fprintf(stderr, "save RAM size was not forwarded\n");
        return 35;
    }

    save_ram = retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);
    if (!save_ram) {
        fprintf(stderr, "save RAM pointer was not forwarded\n");
        return 36;
    }
    save_ram[0] = 0x42;
    if (((uint8_t *)retro_get_memory_data(RETRO_MEMORY_SAVE_RAM))[0] != 0x42) {
        fprintf(stderr, "save RAM access was not transparent\n");
        return 37;
    }

    retro_unload_game();
    retro_deinit();
    dlclose(wrapper);

    puts("M0 proxy smoke test passed");
    return 0;
}

#undef RESOLVE
