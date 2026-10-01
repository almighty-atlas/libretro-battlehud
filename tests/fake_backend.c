#include "libretro.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static retro_environment_t environment_cb;
static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

static uint32_t frame[160 * 144];
static uint8_t save_ram[8192];

unsigned retro_api_version(void)
{
    return RETRO_API_VERSION;
}

void retro_init(void)
{
    enum retro_pixel_format format = RETRO_PIXEL_FORMAT_XRGB8888;
    if (environment_cb)
        environment_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &format);
}

void retro_deinit(void) {}

void retro_set_environment(retro_environment_t cb) { environment_cb = cb; }
void retro_set_video_refresh(retro_video_refresh_t cb) { video_cb = cb; }
void retro_set_audio_sample(retro_audio_sample_t cb) { audio_cb = cb; }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { audio_batch_cb = cb; }
void retro_set_input_poll(retro_input_poll_t cb) { input_poll_cb = cb; }
void retro_set_input_state(retro_input_state_t cb) { input_state_cb = cb; }

void retro_get_system_info(struct retro_system_info *info)
{
    memset(info, 0, sizeof(*info));
    info->library_name = "BattleHUD Fake Backend";
    info->library_version = "M0";
    info->valid_extensions = "gb|gbc";
    info->need_fullpath = false;
    info->block_extract = false;
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
    memset(info, 0, sizeof(*info));
    info->geometry.base_width = 160;
    info->geometry.base_height = 144;
    info->geometry.max_width = 160;
    info->geometry.max_height = 144;
    info->geometry.aspect_ratio = 160.0f / 144.0f;
    info->timing.fps = 60.0;
    info->timing.sample_rate = 48000.0;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
    (void)port;
    (void)device;
}

void retro_reset(void) {}

void retro_run(void)
{
    static const int16_t audio[2] = {0, 0};

    if (input_poll_cb)
        input_poll_cb();

    if (input_state_cb)
        (void)input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A);

    if (video_cb)
        video_cb(frame, 160, 144, 160 * sizeof(uint32_t));

    if (audio_batch_cb)
        audio_batch_cb(audio, 1);
    else if (audio_cb)
        audio_cb(0, 0);
}

size_t retro_serialize_size(void) { return 4; }

bool retro_serialize(void *data, size_t size)
{
    if (!data || size < 4)
        return false;
    memcpy(data, "M0OK", 4);
    return true;
}

bool retro_unserialize(const void *data, size_t size)
{
    return data && size >= 4;
}

void retro_cheat_reset(void) {}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
    (void)index;
    (void)enabled;
    (void)code;
}

bool retro_load_game(const struct retro_game_info *game)
{
    return game != NULL;
}

bool retro_load_game_special(unsigned game_type,
                             const struct retro_game_info *info,
                             size_t num_info)
{
    (void)game_type;
    return info != NULL && num_info > 0;
}

void retro_unload_game(void) {}

unsigned retro_get_region(void) { return RETRO_REGION_NTSC; }

void *retro_get_memory_data(unsigned id)
{
    return id == RETRO_MEMORY_SAVE_RAM ? save_ram : NULL;
}

size_t retro_get_memory_size(unsigned id)
{
    return id == RETRO_MEMORY_SAVE_RAM ? sizeof(save_ram) : 0;
}
