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


class BattleState(C.Structure):
    _fields_ = [("status", C.c_int), ("mode", C.c_uint8), ("species", C.c_uint16),
                ("type1", C.c_int), ("type2", C.c_int),
                ("raw_type1", C.c_uint8), ("raw_type2", C.c_uint8)]


class TypeHud(C.Structure):
    _fields_ = [("clean", C.c_void_p), ("output", C.c_void_p),
                ("clean_capacity", C.c_size_t), ("output_capacity", C.c_size_t),
                ("width", C.c_uint), ("height", C.c_uint), ("pitch", C.c_size_t),
                ("format", C.c_int), ("has_frame", C.c_bool), ("has_presented", C.c_bool),
                ("presented", BattleState)]


class Info(C.Structure):
    _fields_ = [("name", C.c_char_p), ("version", C.c_char_p),
                ("extensions", C.c_char_p), ("fullpath", C.c_bool),
                ("block_extract", C.c_bool)]


ENV = C.CFUNCTYPE(C.c_bool, C.c_uint, C.c_void_p)
VIDEO = C.CFUNCTYPE(None, C.c_void_p, C.c_uint, C.c_uint, C.c_size_t)
SAMPLE = C.CFUNCTYPE(None, C.c_int16, C.c_int16)
BATCH = C.CFUNCTYPE(C.c_size_t, C.c_void_p, C.c_size_t)
POLL = C.CFUNCTYPE(None)
READ = C.CFUNCTYPE(C.c_bool, C.c_void_p, C.c_size_t, C.c_void_p, C.c_size_t)
INPUT = C.CFUNCTYPE(C.c_int16, C.c_uint, C.c_uint, C.c_uint, C.c_uint)


def worker(core_path, directory):
    core = C.CDLL(core_path)
    video_hash, audio_hash = hashlib.sha256(), hashlib.sha256()
    stats = {"frames": 0, "samples": 0, "polls": 0}
    errors = []
    captured_frame = None
    marker_test = os.environ.get("LIBRETRO_BATTLEHUD_TEST_MARKER") == "1"
    require_marker = os.environ.get("BATTLEHUD_EXPECT_MARKER") == "1"
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
        nonlocal captured_frame
        stats["frames"] += 1
        stats["geometry"] = [width, height, pitch, pixel_format]
        if not data:
            return
        bpp = 4 if pixel_format == 1 else 2
        if pitch < width * bpp:
            errors.append("invalid frame pitch")
            return
        # Own the last software frame for a standalone renderer integration check.
        captured_frame = (C.create_string_buffer(C.string_at(data, pitch * (height - 1) + width * bpp)),
                          width, height, pitch, pixel_format)
        # Padding is unspecified; compare visible pixels only.
        for row in range(height):
            pixels = C.string_at(data + row * pitch, width * bpp)
            if marker_test and row < 8:
                if require_marker:
                    white = {0: 0x7fff, 1: 0xffffff, 2: 0xffff}[pixel_format]
                    expected = white.to_bytes(bpp, sys.byteorder) * 8
                    if pixels[-8 * bpp:] != expected:
                        errors.append("test marker missing")
                pixels = pixels[:-8 * bpp]
            video_hash.update(pixels)

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
    rom[0x143] = 0x80  # GBC-compatible: test physical WRAM bank 1 as well.
    rom[0x147], rom[0x149] = 3, 2  # MBC1 + RAM + battery; 8 KiB SRAM.
    # Enable SRAM, store $42, write $6D to CPU RAM $C123, set LCD, then loop. No commercial assets.
    code = bytes.fromhex("3e 0a ea 00 00 3e 42 ea 00 a0 3e 6d ea 23 c1 3e e4 ea 47 ff 3e 91 ea 40 ff 18 fe")
    # Literal Crystal-shaped fixture: wild Pidgey, Normal/Flying, level 5, HP 20.
    # This original ROM remains unrecognized by the production profile gate.
    fixture = {0xc734: 0, 0xc711: 0, 0xd264: 0, 0xd22d: 1, 0xd206: 16,
               0xd213: 5, 0xd216: 0, 0xd217: 20, 0xd218: 0, 0xd219: 20,
               0xd224: 0, 0xd225: 2}
    writes = bytes.fromhex("3e 01 ea 70 ff")  # SVBK = 1
    for address, value in fixture.items():
        writes += bytes([0x3e, value, 0xea, address & 255, address >> 8])
    code = code[:-2] + writes + bytes.fromhex("18 fe")
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
    # Independent reference: Gambatte's SYSTEM_RAM starts at its RAM bank 0.
    # The wrapper must obtain CPU $C123 from SET_MEMORY_MAPS, even though our
    # environment callback rejects that command. Region reads never guess bases.
    ram = core.retro_get_memory_data(2)
    assert ram and core.retro_get_memory_size(2) >= 0x124
    reference = C.c_uint8.from_address(ram + 0x123).value
    assert reference == 0x6d, "test ROM RAM write missing"
    mapped_read = getattr(core, "battlehud_read_memory", None)
    region_read = getattr(core, "battlehud_read_region", None)
    if mapped_read:
        mapped_read.argtypes, mapped_read.restype = [C.c_size_t, C.c_void_p, C.c_size_t], C.c_bool
        region_read.argtypes, region_read.restype = [C.c_uint, C.c_size_t, C.c_void_p, C.c_size_t], C.c_bool
        value = C.c_uint8(0)
        assert mapped_read(0xc123, C.byref(value), 1) and value.value == reference
        assert region_read(2, 0x123, C.byref(value), 1) and value.value == reference
        assert mapped_read(0xa000, C.byref(value), 1) and value.value == 0x42
        value.value = 0x99
        assert not mapped_read(0xdeadbeef, C.byref(value), 1) and value.value == 0x99
        assert not region_read(2, core.retro_get_memory_size(2), C.byref(value), 1)
    stats["ram_probe"] = reference
    # Independent actual-emulator fixture check, including the bank-1 addresses.
    assert core.retro_get_memory_size(2) == 0x8000
    for address, expected in fixture.items():
        assert C.c_uint8.from_address(ram + address - 0xc000).value == expected
    expected_battle = [3, 1, 16, 1, 10, 0, 2]  # ACTIVE/wild/Pidgey/Normal/Flying
    if mapped_read:
        @READ
        def read_fixture(context, address, destination, length):
            return mapped_read(address, destination, length)
        find = core.game_profile_find
        find.argtypes, find.restype = [C.c_char_p], C.c_void_p
        decode = core.battle_decode
        decode.argtypes, decode.restype = [C.c_void_p, READ, C.c_void_p], BattleState
        snapshot = decode(find(b"f2f52230b536214ef7c9924f483392993e226cfb"), read_fixture, None)
        actual = [snapshot.status, snapshot.mode, snapshot.species, snapshot.type1,
                  snapshot.type2, snapshot.raw_type1, snapshot.raw_type2]
        assert actual == expected_battle, actual
        # Component integration over actual emulator pixels and the decoded model.
        # Production profile gating intentionally still rejects this synthetic ROM.
        frame_buffer, width, height, pitch, fmt = captured_frame
        original = frame_buffer.raw
        draw, clear = core.type_hud_draw, core.type_hud_clear
        draw.argtypes = [C.POINTER(TypeHud), C.POINTER(BattleState), C.c_void_p,
                         C.c_uint, C.c_uint, C.c_size_t, C.c_int]
        draw.restype = C.c_void_p
        clear.argtypes, clear.restype = [C.POINTER(TypeHud)], None
        hud = TypeHud()
        rendered = draw(C.byref(hud), C.byref(snapshot), frame_buffer, width, height, pitch, fmt)
        assert rendered and rendered != C.addressof(frame_buffer)
        bpp = 4 if fmt == 1 else 2
        white = {0: 0x7fff, 1: 0xffffff, 2: 0xffff}[fmt]
        assert int.from_bytes(C.string_at(rendered + 5 * pitch + 119 * bpp, bpp), sys.byteorder) == white
        for y in range(height):
            for x in range(width):
                badge = 115 <= x < 158 and (2 <= y < 15 or 17 <= y < 30)
                if not badge:
                    offset = y * pitch + x * bpp
                    assert C.string_at(rendered + offset, bpp) == original[offset:offset + bpp]
        assert frame_buffer.raw == original
        assert not draw(C.byref(hud), C.byref(snapshot), None, width, height, pitch, fmt)
        # Model change on a NULL duplicate must redraw; outside battle must erase.
        snapshot.type2 = 0
        assert draw(C.byref(hud), C.byref(snapshot), None, width, height, pitch, fmt)
        snapshot.status = 1
        clean = draw(C.byref(hud), C.byref(snapshot), None, width, height, pitch, fmt)
        assert clean
        for y in range(height):
            assert C.string_at(clean + y * pitch, width * bpp) == original[y * pitch:y * pitch + width * bpp]
        clear(C.byref(hud))
        assert not hud.clean and not hud.output and not hud.has_frame
        print("Type HUD real-Gambatte frame/decoder fixture passed")
        getter = core.battlehud_get_battle_state
        getter.argtypes, getter.restype = [C.POINTER(BattleState)], C.c_bool
        assert not getter(C.byref(snapshot)), "original ROM must stay unsupported"
        assert snapshot.status == 0 and snapshot.species == 0
    stats["battle_fixture"] = expected_battle

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
    if mapped_read:
        assert mapped_read(0xc123, C.byref(value), 1) and value.value == reference
        assert mapped_read(0xa000, C.byref(value), 1) and value.value == 0x42
    stats.update(video=video_hash.hexdigest(), audio=audio_hash.hexdigest(), state_size=state_size)
    core.retro_unload_game()
    if mapped_read:
        assert not mapped_read(0xc123, C.byref(value), 1)
        assert not region_read(2, 0x123, C.byref(value), 1)
    core.retro_deinit()
    # Native stdio may flush after Python output at process exit.
    # Keep the machine-readable result separate from backend diagnostics.
    (Path(directory) / "result.json").write_text(json.dumps(stats, sort_keys=True))


def main():
    if sys.argv[1] == "--worker":
        worker(sys.argv[2], sys.argv[3])
        return
    wrapper, backend = map(lambda p: str(Path(p).resolve()), sys.argv[1:3])
    marker_test = "--marker" in sys.argv[3:]
    results = []
    for core in (backend, wrapper):
        with tempfile.TemporaryDirectory() as directory:
            env = dict(os.environ, LIBRETRO_BATTLEHUD_BACKEND=backend)
            env["LIBRETRO_BATTLEHUD_TEST_MARKER"] = "1" if marker_test else "0"
            env["BATTLEHUD_EXPECT_MARKER"] = "1" if marker_test and core == wrapper else "0"
            run = subprocess.run([sys.executable, __file__, "--worker", core, directory],
                                 env=env, capture_output=True, text=True, timeout=60)
            if run.returncode:
                raise RuntimeError(f"{core}: {run.stdout}\n{run.stderr}")
            results.append(json.loads((Path(directory) / "result.json").read_text()))
    assert results[0] == results[1], results
    label = "marker and non-marker-area parity" if marker_test else "direct/proxy parity"
    print(f"Real Gambatte {label} passed:", json.dumps(results[0], sort_keys=True))


if __name__ == "__main__":
    main()
