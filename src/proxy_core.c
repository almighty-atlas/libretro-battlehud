#define _GNU_SOURCE

#include "libretro.h"
#include "video_marker.h"
#include "memory_view.h"
#include "battle_decoder.h"
#include "training_progress.h"
#include "hidden_power.h"
#include "party_details.h"
#include "sha1.h"
#include "type_hud.h"

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
#ifdef __APPLE__
#define BATTLEHUD_BACKEND_BASENAME "gambatte_real_libretro.dylib"
#else
#define BATTLEHUD_BACKEND_BASENAME "gambatte_real_libretro.so"
#endif
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
static struct video_marker marker;
static struct type_hud hud;
static bool hud_enabled;
static struct memory_view memory;
static bool game_loaded;
static bool marker_enabled;
static bool debug_enabled;
static bool gambatte_memory_layout;
static const struct game_profile *profile;
static struct battle_state battle;
static struct training_progress progress;
/* Private, immutable ROM copy for bank-independent Gen 1/2 base-stat reads.
 * The virtual range is never exposed to the backend/frontend or written. */
static uint8_t *training_rom;
static size_t training_rom_size;
static void clear_training_rom(void)
{ free(training_rom);training_rom=NULL;training_rom_size=0; }

static bool decoder_read(void *context, size_t address, void *out, size_t size)
{
    (void)context;
    if (!game_loaded)
        return false;
    if(address>=0x10000000 && address-0x10000000<training_rom_size && out && size &&
       size<=training_rom_size-(address-0x10000000)) {
        memcpy(out,training_rom+address-0x10000000,size);return true;
    }
    if (memory_view_read(&memory, address, out, size))
        return true;
    /* Explicit Gambatte fallback: fixed WRAM banks 0/1 at region offsets 0/0x1000.
     * This mapping is verified against its source; not a generic CPU-base guess. */
    if (!gambatte_memory_layout || address < 0xc000 || address >= 0xe000 ||
        !size || size > 0xe000 - address)
        return false;
    size_t length = backend.get_memory_size(RETRO_MEMORY_SYSTEM_RAM);
    const unsigned char *ram = backend.get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
    if (!ram || length < (profile && profile->generation==1 ? 0x2000u : 0x8000u))
        return false;
    memcpy(out, ram + address - 0xc000, size);
    return true;
}

static void clear_battle(void)
{
    training_progress_clear(&progress);
    memset(&battle, 0, sizeof(battle));
    if (profile)
        battle.status = BATTLE_UNAVAILABLE;
}

static void update_battle(void)
{
    struct battle_state next = battle_decode(profile, decoder_read, NULL);
    training_progress_apply(&progress,&next.training);
    if (debug_enabled && !battle_state_equal(&battle, &next)) {
        if (next.status == BATTLE_ACTIVE && !next.main_menu && !next.fight_menu)
            fprintf(stderr, "battlehud: hidden (battle submenu)\n");
        else if (next.status == BATTLE_ACTIVE && next.ambiguous_target)
            fprintf(stderr,"battlehud: battle ambiguous targets; hints unknown, badges hidden\n");
        else if (next.status == BATTLE_ACTIVE) {
            fprintf(stderr, "battlehud: %s species=%u types=%s%s%s raw=%02x/%02x\n",
                    next.mode == 1 ? "wild" : "trainer", (unsigned)next.species,
                    pokemon_type_name(next.type1), next.type2 ? "/" : "",
                    next.type2 ? pokemon_type_name(next.type2) : "",
                    (unsigned)next.raw_type1, (unsigned)next.raw_type2);
            if(next.fight_menu) {
                static const char *labels[]={"unknown","super","resisted","neutral",
                                              "immune","status","unusable"};
                fprintf(stderr,"battlehud: FIGHT");
                for(unsigned i=0;i<4;i++) if(next.moves[i])
                    fprintf(stderr," %u=%s",(unsigned)next.moves[i],labels[next.effectiveness[i]]);
                fprintf(stderr,"\n");
            }
        }
        else {
            const char *reason = next.status == BATTLE_OUTSIDE ? "outside battle" :
                next.status == BATTLE_TRANSITION ? "battle transition" :
                next.status == BATTLE_UNAVAILABLE ? "memory unavailable" :
                next.status == BATTLE_INVALID ? "invalid battle data" : "unsupported game";
            fprintf(stderr, "battlehud: hidden (%s)\n", reason);
        }
    }
    if(debug_enabled && !training_stats_equal(&battle.training,&next.training)) {
        if(next.training.visible) {
            fprintf(stderr,"battlehud: stats party=%u species=%u order=HP/ATK/DEF/SPA/SPD/SPE %s=",
                    (unsigned)next.training.slot+1,(unsigned)next.training.species,
                    next.training.generation==3 ? "IV" : "DV");
            for(unsigned i=0;i<6;i++) fprintf(stderr,"%s%u",i?"/":"",(unsigned)next.training.dv[i]);
            fprintf(stderr," EV=");
            for(unsigned i=0;i<6;i++) fprintf(stderr,"%s%u",i?"/":"",(unsigned)next.training.ev[i]);
            fprintf(stderr,"\n");
            if(next.training.generation==3 && next.training.nature_known)
                fprintf(stderr,"battlehud: nature=%s ability=%s slot=%u\n",
                        gen3_nature_name(next.training.nature),
                        next.training.ability_known ? next.training.ability_name : "unavailable",
                        (unsigned)next.training.ability_slot);
            struct hidden_power hp;
            if(hidden_power_calculate(next.training.generation,next.training.dv,&hp))
                fprintf(stderr,"battlehud: Hidden Power type=%s power=%u\n",
                        pokemon_type_name(hp.type),(unsigned)hp.power);
        } else fprintf(stderr,"battlehud: stats hidden\n");
    }
    battle = next;
}

static void detect_profile(const struct retro_game_info *game)
{
    char hash[41] = {0};
    struct retro_system_info info = {0};
    backend.get_system_info(&info);
    gambatte_memory_layout = info.library_name && !strcmp(info.library_name, "Gambatte");
    if (game && game->data && game->size && game->size <= 32 * 1024 * 1024) {
        struct sha1_context sha;
        sha1_init(&sha); sha1_update(&sha, game->data, game->size); sha1_final(&sha, hash);
    } else if (game && !game->data) {
        (void)sha1_file(game->path, hash);
    }
    profile = game_profile_find(hash);
    if(profile && (!info.library_name || strcmp(info.library_name,profile->backend_name)))
        profile = NULL;
    clear_battle();
    clear_training_rom();
    if(profile && profile->generation<3 && game) {
        size_t size=game->size;FILE *file=NULL;
        if(!game->data && game->path) {
            file=fopen(game->path,"rb");
            if(file && !fseek(file,0,SEEK_END)) {long n=ftell(file);if(n>0 && n<=32*1024*1024)size=(size_t)n;rewind(file);}
        }
        if(size && size<=32*1024*1024) {
            uint8_t *copy=malloc(size);
            if(copy && (game->data || (file && fread(copy,1,size,file)==size))) {
                if(game->data)memcpy(copy,game->data,size);
                struct sha1_context sha;char checked[41];sha1_init(&sha);sha1_update(&sha,copy,size);sha1_final(&sha,checked);
                if(!strcmp(checked,profile->sha1)){training_rom=copy;training_rom_size=size;}else free(copy);
            } else free(copy);
        }
        if(file)fclose(file);
    }
    if (debug_enabled) {
        if (profile)
            fprintf(stderr, "battlehud: profile=%s sha1=%s backend=%s %s\n",
                    profile->id, hash, info.library_name,
                    info.library_version ? info.library_version : "unknown");
        else
            fprintf(stderr, "battlehud: unsupported game/backend sha1=%s; decoder disabled\n",
                    hash[0] ? hash : "unavailable");
    }
}
static enum retro_pixel_format pixel_format = RETRO_PIXEL_FORMAT_0RGB1555;

static bool proxy_environment(unsigned cmd, void *data)
{
    bool captured = cmd == RETRO_ENVIRONMENT_SET_MEMORY_MAPS &&
        memory_view_capture(&memory, data);
    bool accepted = frontend_environment ? hud_options_register(frontend_environment, cmd, data) : false;
    if (accepted && data && cmd == RETRO_ENVIRONMENT_SET_PIXEL_FORMAT)
        pixel_format = *(const enum retro_pixel_format *)data;
    return accepted || captured;
}

static void proxy_video_refresh(const void *data, unsigned width,
                                unsigned height, size_t pitch)
{
    if (frontend_video_refresh) {
        if (game_loaded && profile) {
            /* Sample the RAM belonging to this callback before compositing. */
            update_battle();
            data = type_hud_draw_options(&hud, &battle, data, width, height, pitch, pixel_format,
                hud_enabled ? hud_options_read(frontend_environment) : 0);
        }
        if (marker_enabled)
            data = video_marker_draw(&marker, data, width, height, pitch, pixel_format);
        frontend_video_refresh(data, width, height, pitch);
    }
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
    const char *enabled = getenv("LIBRETRO_BATTLEHUD_TEST_MARKER");
    marker_enabled = enabled && strcmp(enabled, "1") == 0;
    enabled = getenv("LIBRETRO_BATTLEHUD_DISABLE_HUD");
    hud_enabled = !(enabled && strcmp(enabled, "1") == 0);
    enabled = getenv("LIBRETRO_BATTLEHUD_DEBUG");
    debug_enabled = enabled && strcmp(enabled, "1") == 0;
    if (load_backend())
        backend.init();
}

void retro_deinit(void)
{
    game_loaded = false;
    clear_training_rom();
    profile = NULL;
    gambatte_memory_layout = false;
    clear_battle();
    type_hud_clear(&hud);
    memory_view_clear(&memory);
    /* Teardown must not load a core that never became available. */
    if (backend.handle) {
        backend.deinit();
        dlclose(backend.handle);
        memset(&backend, 0, sizeof(backend));
    }
    hud_options_clear();
    video_marker_clear(&marker);
    marker_enabled = false;
    debug_enabled = false;
    hud_enabled = false;
    pixel_format = RETRO_PIXEL_FORMAT_0RGB1555;
}

void retro_set_environment(retro_environment_t cb)
{
    frontend_environment = cb;
    callback_mask |= CB_ENV;
    if (backend.handle)
        backend.set_environment(cb ? proxy_environment : NULL);
    else
        (void)load_backend();
    hud_options_fallback(cb);
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
    type_hud_clear(&hud);
    clear_battle();
    if (load_backend())
        backend.reset();
}

void retro_run(void)
{
    if (load_backend()) {
        backend.run();
        if (game_loaded && profile) {
            update_battle();
            struct training_party party;
            bool known=training_party_read(profile,decoder_read,NULL,&party);
            int phase=battle.status==BATTLE_OUTSIDE?0:
                ((battle.status==BATTLE_ACTIVE || battle.status==BATTLE_TRANSITION) && battle.mode?1:-1);
            training_progress_update(&progress,known?&party:NULL,phase);
        }
    }
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
    bool restored = load_backend() && backend.unserialize(data, size);
    if (restored && game_loaded && profile) {
        type_hud_clear(&hud);
        clear_battle();
        update_battle();
    }
    return restored;
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
    game_loaded = false;
    clear_training_rom();
    profile = NULL;
    gambatte_memory_layout = false;
    clear_battle();
    type_hud_clear(&hud);
    memory_view_clear(&memory);
    game_loaded = load_backend() && backend.load_game(game);
    if (!game_loaded)
        memory_view_clear(&memory);
    else
        detect_profile(game);
    return game_loaded;
}

bool retro_load_game_special(unsigned game_type,
                             const struct retro_game_info *info,
                             size_t num_info)
{
    game_loaded = false;
    clear_training_rom();
    profile = NULL;
    gambatte_memory_layout = false;
    clear_battle();
    type_hud_clear(&hud);
    memory_view_clear(&memory);
    game_loaded = load_backend() &&
        backend.load_game_special(game_type, info, num_info);
    if (!game_loaded)
        memory_view_clear(&memory);
    return game_loaded;
}

void retro_unload_game(void)
{
    game_loaded = false;
    clear_training_rom();
    profile = NULL;
    gambatte_memory_layout = false;
    clear_battle();
    type_hud_clear(&hud);
    memory_view_clear(&memory);
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

/* Read-only wrapper extensions for the future decoder and validation hosts.
 * Calls must run on the emulation thread, between retro_run calls.
 * No CPU-address assumption is made for raw Libretro memory regions. */
bool battlehud_read_memory(size_t address, void *out, size_t size)
{
    return game_loaded && memory_view_read(&memory, address, out, size);
}

bool battlehud_read_region(unsigned id, size_t offset, void *out, size_t size)
{
    if (!game_loaded || !backend.handle || !out || !size || size > 1024 * 1024)
        return false;
    size_t length = backend.get_memory_size(id);
    const unsigned char *data = backend.get_memory_data(id);
    if (!data || offset > length || size > length - offset ||
        (uintptr_t)data > UINTPTR_MAX - offset ||
        (uintptr_t)data + offset > UINTPTR_MAX - (size - 1))
        return false;
    memcpy(out, data + offset, size);
    return true;
}

/* Validation interface; same-thread callers receive a value snapshot, never RAM. */
bool battlehud_get_battle_state(struct battle_state *out)
{
    if (!out)
        return false;
    *out = battle;
    return game_loaded && profile != NULL;
}
