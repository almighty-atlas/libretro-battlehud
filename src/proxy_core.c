#define _GNU_SOURCE

#include "libretro.h"

#include <dlfcn.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#ifndef BATTLEHUD_BACKEND_BASENAME
#define BATTLEHUD_BACKEND_BASENAME "gambatte_real_libretro.so"
#endif

struct backend_api {
    void *handle;

    unsigned (*api_version)(void);
    void (*init)(void);
    void (*deinit)(void);

    void (*set_environment)(retro_environment_t);
    void (*set_video_refresh)(retro_video_refresh_t);
    void (*set_audio_sample)(retro_audio_sample_t);
    void (*set_audio_sample_batch)(retro_audio_sample_batch_t);
    void (*set_input_poll)(retro_input_poll_t);
    void (*set_input_state)(retro_input_state_t);

    void (*get_system_info)(struct retro_system_info *);
    void (*get_system_av_info)(struct retro_system_av_info *);

    void (*set_controller_port_device)(unsigned, unsigned);
    void (*reset)(void);
    void (*run)(void);

    size_t (*serialize_size)(void);
    bool (*serialize)(void *, size_t);
    bool (*unserialize)(const void *, size_t);

    void (*cheat_reset)(void);
    void (*cheat_set)(unsigned, bool, const char *);

    bool (*load_game)(const struct retro_game_info *);
    bool (*load_game_special)(unsigned, const struct retro_game_info *, size_t);
    void (*unload_game)(void);

    unsigned (*get_region)(void);
    void *(*get_memory_data)(unsigned);
    size_t (*get_memory_size)(unsigned);
};

static struct backend_api backend;
static int proxy_anchor;
/* Remember only setters actually called by the frontend, including NULL. */
static unsigned callback_mask;
enum {
    CB_ENV = 1, CB_VIDEO = 2, CB_AUDIO = 4,
    CB_BATCH = 8, CB_POLL = 16, CB_STATE = 32
};

static retro_environment_t frontend_environment;
static retro_video_refresh_t frontend_video_refresh;
static retro_audio_sample_t frontend_audio_sample;
static retro_audio_sample_batch_t frontend_audio_sample_batch;
static retro_input_poll_t frontend_input_poll;
static retro_input_state_t frontend_input_state;

static bool proxy_environment(unsigned cmd, void *data)
{
    /*
     * M0 is deliberately transparent. Future milestones will observe
     * RETRO_ENVIRONMENT_SET_MEMORY_MAPS and RETRO_ENVIRONMENT_SET_PIXEL_FORMAT
     * here, then forward the call unchanged.
     */
    return frontend_environment ? frontend_environment(cmd, data) : false;
}

static void proxy_video_refresh(const void *data, unsigned width,
                                unsigned height, size_t pitch)
{
    /* M0 forwards frames byte-for-byte. M1 will composite the HUD here. */
    if (frontend_video_refresh)
        frontend_video_refresh(data, width, height, pitch);
}

static void proxy_audio_sample(int16_t left, int16_t right)
{
    if (frontend_audio_sample)
        frontend_audio_sample(left, right);
}

static size_t proxy_audio_sample_batch(const int16_t *data, size_t frames)
{
    return frontend_audio_sample_batch
        ? frontend_audio_sample_batch(data, frames)
        : 0;
}

static void proxy_input_poll(void)
{
    if (frontend_input_poll)
        frontend_input_poll();
}

static int16_t proxy_input_state(unsigned port, unsigned device,
                                 unsigned index, unsigned id)
{
    return frontend_input_state
        ? frontend_input_state(port, device, index, id)
        : 0;
}

static bool load_required_symbol(void *handle, const char *name, void *out)
{
    void *symbol;

    dlerror();
    symbol = dlsym(handle, name);
    if (!symbol) {
        const char *error = dlerror();
        fprintf(stderr, "libretro-battlehud: missing backend symbol %s: %s\n",
                name, error ? error : "unknown error");
        return false;
    }

    /*
     * POSIX guarantees dlsym results can be converted to function pointers.
     * memcpy avoids ISO C's direct object/function pointer cast diagnostics.
     */
    memcpy(out, &symbol, sizeof(symbol));
    return true;
}

static bool resolve_backend_path(char *path, size_t path_size)
{
    const char *override = getenv("LIBRETRO_BATTLEHUD_BACKEND");
    Dl_info info;
    const char *slash;
    size_t dir_len;

    if (override && override[0] != '\0') {
        if (snprintf(path, path_size, "%s", override) >= (int)path_size)
            return false;
        return true;
    }

    if (!dladdr(&proxy_anchor, &info) || !info.dli_fname)
        return false;

    slash = strrchr(info.dli_fname, '/');
    if (!slash) {
        if (snprintf(path, path_size, "./%s", BATTLEHUD_BACKEND_BASENAME)
                >= (int)path_size)
            return false;
        return true;
    }

    dir_len = (size_t)(slash - info.dli_fname);
    if (dir_len + 1 + strlen(BATTLEHUD_BACKEND_BASENAME) + 1 > path_size)
        return false;

    memcpy(path, info.dli_fname, dir_len);
    path[dir_len] = '/';
    strcpy(path + dir_len + 1, BATTLEHUD_BACKEND_BASENAME);
    return true;
}

#define LOAD_BACKEND_SYMBOL(field, name)                                      \
    do {                                                                       \
        if (!load_required_symbol(backend.handle, name, &backend.field))       \
            goto fail;                                                         \
    } while (0)

static bool load_backend(void)
{
    char path[PATH_MAX];

    if (backend.handle)
        return true;

    if (!resolve_backend_path(path, sizeof(path))) {
        fprintf(stderr, "libretro-battlehud: could not resolve backend path\n");
        return false;
    }

    backend.handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!backend.handle) {
        fprintf(stderr, "libretro-battlehud: failed to load backend %s: %s\n",
                path, dlerror());
        return false;
    }

    LOAD_BACKEND_SYMBOL(api_version, "retro_api_version");
    {
        Dl_info wrapper_info, backend_info;
        void *symbol = dlsym(backend.handle, "retro_api_version");
        if (!dladdr(&proxy_anchor, &wrapper_info) ||
            !dladdr(symbol, &backend_info) ||
            wrapper_info.dli_fbase == backend_info.dli_fbase) {
            fprintf(stderr, "libretro-battlehud: backend resolves to the proxy itself\n");
            goto fail;
        }
    }
    if (backend.api_version() != RETRO_API_VERSION) {
        fprintf(stderr, "libretro-battlehud: incompatible backend API version\n");
        goto fail;
    }
    LOAD_BACKEND_SYMBOL(init, "retro_init");
    LOAD_BACKEND_SYMBOL(deinit, "retro_deinit");
    LOAD_BACKEND_SYMBOL(set_environment, "retro_set_environment");
    LOAD_BACKEND_SYMBOL(set_video_refresh, "retro_set_video_refresh");
    LOAD_BACKEND_SYMBOL(set_audio_sample, "retro_set_audio_sample");
    LOAD_BACKEND_SYMBOL(set_audio_sample_batch, "retro_set_audio_sample_batch");
    LOAD_BACKEND_SYMBOL(set_input_poll, "retro_set_input_poll");
    LOAD_BACKEND_SYMBOL(set_input_state, "retro_set_input_state");
    LOAD_BACKEND_SYMBOL(get_system_info, "retro_get_system_info");
    LOAD_BACKEND_SYMBOL(get_system_av_info, "retro_get_system_av_info");
    LOAD_BACKEND_SYMBOL(set_controller_port_device, "retro_set_controller_port_device");
    LOAD_BACKEND_SYMBOL(reset, "retro_reset");
    LOAD_BACKEND_SYMBOL(run, "retro_run");
    LOAD_BACKEND_SYMBOL(serialize_size, "retro_serialize_size");
    LOAD_BACKEND_SYMBOL(serialize, "retro_serialize");
    LOAD_BACKEND_SYMBOL(unserialize, "retro_unserialize");
    LOAD_BACKEND_SYMBOL(cheat_reset, "retro_cheat_reset");
    LOAD_BACKEND_SYMBOL(cheat_set, "retro_cheat_set");
    LOAD_BACKEND_SYMBOL(load_game, "retro_load_game");
    LOAD_BACKEND_SYMBOL(load_game_special, "retro_load_game_special");
    LOAD_BACKEND_SYMBOL(unload_game, "retro_unload_game");
    LOAD_BACKEND_SYMBOL(get_region, "retro_get_region");
    LOAD_BACKEND_SYMBOL(get_memory_data, "retro_get_memory_data");
    LOAD_BACKEND_SYMBOL(get_memory_size, "retro_get_memory_size");

    /* A previous load may have failed before frontend setup completed. */
    if (callback_mask & CB_ENV)
        backend.set_environment(frontend_environment ? proxy_environment : NULL);
    if (callback_mask & CB_VIDEO)
        backend.set_video_refresh(frontend_video_refresh ? proxy_video_refresh : NULL);
    if (callback_mask & CB_AUDIO)
        backend.set_audio_sample(frontend_audio_sample ? proxy_audio_sample : NULL);
    if (callback_mask & CB_BATCH)
        backend.set_audio_sample_batch(frontend_audio_sample_batch ? proxy_audio_sample_batch : NULL);
    if (callback_mask & CB_POLL)
        backend.set_input_poll(frontend_input_poll ? proxy_input_poll : NULL);
    if (callback_mask & CB_STATE)
        backend.set_input_state(frontend_input_state ? proxy_input_state : NULL);
    return true;

fail:
    dlclose(backend.handle);
    memset(&backend, 0, sizeof(backend));
    return false;
}

#undef LOAD_BACKEND_SYMBOL

unsigned retro_api_version(void)
{
    return load_backend() ? backend.api_version() : RETRO_API_VERSION;
}

void retro_init(void)
{
    if (load_backend())
        backend.init();
}

void retro_deinit(void)
{
    /* Teardown must not load a core that never became available. */
    if (backend.handle) {
        backend.deinit();
        dlclose(backend.handle);
        memset(&backend, 0, sizeof(backend));
    }
}

void retro_set_environment(retro_environment_t cb)
{
    frontend_environment = cb;
    callback_mask |= CB_ENV;
    if (backend.handle)
        backend.set_environment(cb ? proxy_environment : NULL);
    else
        (void)load_backend();
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
    frontend_video_refresh = cb;
    callback_mask |= CB_VIDEO;
    if (backend.handle)
        backend.set_video_refresh(cb ? proxy_video_refresh : NULL);
    else
        (void)load_backend();
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
    frontend_audio_sample = cb;
    callback_mask |= CB_AUDIO;
    if (backend.handle)
        backend.set_audio_sample(cb ? proxy_audio_sample : NULL);
    else
        (void)load_backend();
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
    frontend_audio_sample_batch = cb;
    callback_mask |= CB_BATCH;
    if (backend.handle)
        backend.set_audio_sample_batch(cb ? proxy_audio_sample_batch : NULL);
    else
        (void)load_backend();
}

void retro_set_input_poll(retro_input_poll_t cb)
{
    frontend_input_poll = cb;
    callback_mask |= CB_POLL;
    if (backend.handle)
        backend.set_input_poll(cb ? proxy_input_poll : NULL);
    else
        (void)load_backend();
}

void retro_set_input_state(retro_input_state_t cb)
{
    frontend_input_state = cb;
    callback_mask |= CB_STATE;
    if (backend.handle)
        backend.set_input_state(cb ? proxy_input_state : NULL);
    else
        (void)load_backend();
}

void retro_get_system_info(struct retro_system_info *info)
{
    if (!info)
        return;

    if (load_backend()) {
        backend.get_system_info(info);
        return;
    }

    memset(info, 0, sizeof(*info));
    info->library_name = "libretro-battlehud";
    info->library_version = "backend unavailable";
    info->valid_extensions = "";
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
    if (!info)
        return;

    if (load_backend()) {
        backend.get_system_av_info(info);
        return;
    }

    memset(info, 0, sizeof(*info));
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
    if (load_backend())
        backend.set_controller_port_device(port, device);
}

void retro_reset(void)
{
    if (load_backend())
        backend.reset();
}

void retro_run(void)
{
    if (load_backend())
        backend.run();
}

size_t retro_serialize_size(void)
{
    return load_backend() ? backend.serialize_size() : 0;
}

bool retro_serialize(void *data, size_t size)
{
    return load_backend() && backend.serialize(data, size);
}

bool retro_unserialize(const void *data, size_t size)
{
    return load_backend() && backend.unserialize(data, size);
}

void retro_cheat_reset(void)
{
    if (load_backend())
        backend.cheat_reset();
}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
    if (load_backend())
        backend.cheat_set(index, enabled, code);
}

bool retro_load_game(const struct retro_game_info *game)
{
    return load_backend() && backend.load_game(game);
}

bool retro_load_game_special(unsigned game_type,
                             const struct retro_game_info *info,
                             size_t num_info)
{
    return load_backend() &&
        backend.load_game_special(game_type, info, num_info);
}

void retro_unload_game(void)
{
    if (load_backend())
        backend.unload_game();
}

unsigned retro_get_region(void)
{
    return load_backend() ? backend.get_region() : RETRO_REGION_NTSC;
}

void *retro_get_memory_data(unsigned id)
{
    return load_backend() ? backend.get_memory_data(id) : NULL;
}

size_t retro_get_memory_size(unsigned id)
{
    return load_backend() ? backend.get_memory_size(id) : 0;
}
