"""Compare real Gambatte directly and through the proxy using an original test ROM."""

import ctypes as C
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile


class Game(C.Structure):
    _fields_ = [("path", C.c_char_p), ("data", C.c_void_p),
                ("size", C.c_size_t), ("meta", C.c_char_p)]


class Info(C.Structure):
    _fields_ = [("name", C.c_char_p), ("version", C.c_char_p),
                ("extensions", C.c_char_p), ("fullpath", C.c_bool),
                ("block_extract", C.c_bool)]


ENV = C.CFUNCTYPE(C.c_bool, C.c_uint, C.c_void_p)
VIDEO = C.CFUNCTYPE(None, C.c_void_p, C.c_uint, C.c_uint, C.c_size_t)
SAMPLE = C.CFUNCTYPE(None, C.c_int16, C.c_int16)
BATCH = C.CFUNCTYPE(C.c_size_t, C.c_void_p, C.c_size_t)
POLL = C.CFUNCTYPE(None)
INPUT = C.CFUNCTYPE(C.c_int16, C.c_uint, C.c_uint, C.c_uint, C.c_uint)


def worker(core_path, directory):
    core = C.CDLL(core_path)
    video_hash, audio_hash = hashlib.sha256(), hashlib.sha256()
    stats = {"frames": 0, "samples": 0, "polls": 0}
    errors = []
    pixel_format = 0  # Libretro defaults to 0RGB1555.
    directory_bytes = os.fsencode(directory)

    @ENV
    def environment(cmd, data):
        nonlocal pixel_format
        if cmd == 10:  # SET_PIXEL_FORMAT
            pixel_format = C.cast(data, C.POINTER(C.c_int))[0]
            return pixel_format in (0, 1, 2)
        if cmd in (9, 31):  # GET_SYSTEM_DIRECTORY / GET_SAVE_DIRECTORY
            C.cast(data, C.POINTER(C.c_char_p))[0] = directory_bytes
            return True
        if cmd == 3:  # GET_CAN_DUPE
            C.cast(data, C.POINTER(C.c_bool))[0] = True
            return True
        if cmd == 17:  # GET_VARIABLE_UPDATE
            C.cast(data, C.POINTER(C.c_bool))[0] = False
            return True
        if cmd == 16:  # SET_VARIABLES; GET_VARIABLE returns false for defaults.
            return True
        return False

    @VIDEO
    def video(data, width, height, pitch):
        stats["frames"] += 1
        stats["geometry"] = [width, height, pitch, pixel_format]
        if not data:
            return
        bpp = 4 if pixel_format == 1 else 2
        if pitch < width * bpp:
            errors.append("invalid frame pitch")
            return
        # Padding is unspecified; compare visible pixels only.
        for row in range(height):
            video_hash.update(C.string_at(data + row * pitch, width * bpp))

    @SAMPLE
    def sample(left, right):
        audio_hash.update(bytes((C.c_int16 * 2)(left, right)))
        stats["samples"] += 1

    @BATCH
    def batch(data, frames):
        audio_hash.update(C.string_at(data, frames * 4))
        stats["samples"] += frames
        return frames

    @POLL
    def poll():
        stats["polls"] += 1

    @INPUT
    def input_state(port, device, index, button):
        return 0

    callbacks = [("environment", ENV, environment), ("video_refresh", VIDEO, video),
                 ("audio_sample", SAMPLE, sample), ("audio_sample_batch", BATCH, batch),
                 ("input_poll", POLL, poll), ("input_state", INPUT, input_state)]
    for name, kind, callback in callbacks:
        setter = getattr(core, "retro_set_" + name)
        setter.argtypes, setter.restype = [kind], None
        setter(callback)

    core.retro_get_system_info.argtypes = [C.POINTER(Info)]
    info = Info()
    core.retro_get_system_info(C.byref(info))
    assert info.name and b"Gambatte" in info.name, info.name
    core.retro_init()
    rom = bytearray(32768)
    rom[0x100:0x103] = bytes.fromhex("c3 50 01")  # JP $0150
    rom[0x134:0x13f] = b"BATTLEHUD00"
    rom[0x147], rom[0x149] = 3, 2  # MBC1 + RAM + battery; 8 KiB SRAM.
    # Enable SRAM, store $42, set palette/LCD, then loop. No commercial assets.
    code = bytes.fromhex("3e 0a ea 00 00 3e 42 ea 00 a0 3e e4 ea 47 ff 3e 91 ea 40 ff 18 fe")
    rom[0x150:0x150 + len(code)] = code
    checksum = 0
    for byte in rom[0x134:0x14d]:
        checksum = (checksum - byte - 1) & 255
    rom[0x14d] = checksum
    rom_path = Path(directory) / "battlehud-test.gb"
    rom_path.write_bytes(rom)
    buffer = C.create_string_buffer(bytes(rom))
    game = Game(os.fsencode(rom_path), C.cast(buffer, C.c_void_p), len(rom), None)
    core.retro_load_game.argtypes, core.retro_load_game.restype = [C.POINTER(Game)], C.c_bool
    assert core.retro_load_game(C.byref(game)), "test ROM failed to load"
    for _ in range(120):
        core.retro_run()
    assert not errors, errors
    assert stats["frames"] and stats["samples"] and stats["polls"], stats
    core.retro_get_memory_size.argtypes, core.retro_get_memory_size.restype = [C.c_uint], C.c_size_t
    core.retro_get_memory_data.argtypes, core.retro_get_memory_data.restype = [C.c_uint], C.c_void_p
    size = core.retro_get_memory_size(0)
    pointer = core.retro_get_memory_data(0)
    assert size == 8192 and pointer, "SRAM missing"
    assert C.c_uint8.from_address(pointer).value == 0x42, "ROM did not execute"
    stats["sram"] = hashlib.sha256(C.string_at(pointer, size)).hexdigest()
    core.retro_serialize_size.restype = C.c_size_t
    state_size = core.retro_serialize_size()
    assert state_size > 0
    state = C.create_string_buffer(state_size)
    for name in ("serialize", "unserialize"):
        fn = getattr(core, "retro_" + name)
        fn.argtypes, fn.restype = [C.c_void_p, C.c_size_t], C.c_bool
    assert core.retro_serialize(state, state_size)
    C.c_uint8.from_address(pointer).value = 0
    assert core.retro_unserialize(state, state_size)
    pointer = core.retro_get_memory_data(0)
    assert C.c_uint8.from_address(pointer).value == 0x42, "state did not restore SRAM"
    stats.update(video=video_hash.hexdigest(), audio=audio_hash.hexdigest(), state_size=state_size)
    core.retro_unload_game()
    core.retro_deinit()
    # Native stdio may flush after Python output at process exit.
    # Keep the machine-readable result separate from backend diagnostics.
    (Path(directory) / "result.json").write_text(json.dumps(stats, sort_keys=True))


def main():
    if sys.argv[1] == "--worker":
        worker(sys.argv[2], sys.argv[3])
        return
    wrapper, backend = map(lambda p: str(Path(p).resolve()), sys.argv[1:3])
    results = []
    for core in (backend, wrapper):
        with tempfile.TemporaryDirectory() as directory:
            env = dict(os.environ, LIBRETRO_BATTLEHUD_BACKEND=backend)
            run = subprocess.run([sys.executable, __file__, "--worker", core, directory],
                                 env=env, capture_output=True, text=True, timeout=60)
            if run.returncode:
                raise RuntimeError(f"{core}: {run.stdout}\n{run.stderr}")
            results.append(json.loads((Path(directory) / "result.json").read_text()))
    assert results[0] == results[1], results
    print("Real Gambatte direct/proxy parity passed:", json.dumps(results[0], sort_keys=True))


if __name__ == "__main__":
    main()
