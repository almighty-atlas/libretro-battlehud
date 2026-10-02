"""Real mGBA direct/proxy parity using original ARM code, no commercial ROM or BIOS.
The Emerald-shaped RAM fixture validates mapped decoding, not interactive ROM acceptance.
"""
import ctypes as C
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
from gambatte_integration import Game, Info, ENV, VIDEO, SAMPLE, BATCH, POLL, INPUT, READ, TrainingStats, BattleState, TypeHud

class HiddenPower(C.Structure):
    _fields_ = [("type", C.c_int), ("power", C.c_uint8)]

class Descriptor(C.Structure):
    _fields_ = [("flags", C.c_uint64), ("ptr", C.c_void_p), ("offset", C.c_size_t),
                ("start", C.c_size_t), ("select", C.c_size_t), ("disconnect", C.c_size_t),
                ("length", C.c_size_t), ("space", C.c_char_p)]
class Map(C.Structure):
    _fields_ = [("descriptors", C.POINTER(Descriptor)), ("count", C.c_uint)]

def original_rom():
    rom = bytearray(32768)
    struct.pack_into("<I", rom, 0, 0xea00002e)  # ARM branch to $080000C0.
    rom[0xa0:0xac] = b"BATTLEHUDGBA!"
    rom[0xb2] = 0x96
    rom[0xbd] = (-sum(rom[0xa0:0xbd]) - 0x19) & 255
    words = [0xe3a00301, 0xe3a01003, 0xe3811b01, 0xe1c010b0,  # mode 3, BG2
             0xe3a00406, 0xe3a0101f, 0xe1c010b0,  # red first VRAM pixel
             0, 0xe3a0106d, 0xe5c01000, 0, 0xe3a01042, 0xe5c01000, 0xeafffffe,
             0x02000123, 0x03000040]
    for instruction, literal in ((7, 14), (10, 15)):
        words[instruction] = 0xe59f0000 | ((literal - instruction - 2) * 4)
    struct.pack_into("<" + "I" * len(words), rom, 0xc0, *words)
    return rom

def worker(path, directory):
    core = C.CDLL(path)
    maps, frames, samples, polls = [], [], [], []
    fmt = 0
    directory_bytes = os.fsencode(directory)
    @ENV
    def environment(cmd, data):
        nonlocal fmt, maps
        if cmd == 10:
            fmt = C.cast(data, C.POINTER(C.c_int))[0]
            return fmt in (0, 1, 2)
        if cmd in (9, 31):
            C.cast(data, C.POINTER(C.c_char_p))[0] = directory_bytes
            return True
        if cmd == 3:
            C.cast(data, C.POINTER(C.c_bool))[0] = True
            return True
        if cmd == 17:
            C.cast(data, C.POINTER(C.c_bool))[0] = False
            return True
        if cmd == 16:
            return True
        if cmd == (36 | 0x10000):
            m = C.cast(data, C.POINTER(Map)).contents
            maps = [(d.start, d.length, d.ptr, d.offset, d.disconnect, d.space)
                    for d in m.descriptors[:m.count]]
            return False  # wrapper captures maps even when host rejects the command
        return False
    @VIDEO
    def video(data, w, h, pitch):
        if data:
            assert (w, h) == (240, 160)
            bpp = 4 if fmt == 1 else 2
            frames.append((b"".join(C.string_at(data+y*pitch, w*bpp) for y in range(h)), w, h, fmt))
    @SAMPLE
    def sample(l, r):
        samples.append(bytes((C.c_int16 * 2)(l, r)))
    @BATCH
    def batch(data, n):
        samples.append(C.string_at(data, n * 4))
        return n
    @POLL
    def poll():
        polls.append(1)
    @INPUT
    def input_state(port, device, index, button):
        return 0
    callbacks = [("environment", ENV, environment), ("video_refresh", VIDEO, video),
                 ("audio_sample", SAMPLE, sample), ("audio_sample_batch", BATCH, batch),
                 ("input_poll", POLL, poll), ("input_state", INPUT, input_state)]
    for name, kind, callback in callbacks:
        f = getattr(core, "retro_set_" + name)
        f.argtypes, f.restype = [kind], None
        f(callback)
    core.retro_get_system_info.argtypes = [C.POINTER(Info)]
    info = Info()
    core.retro_get_system_info(C.byref(info))
    assert info.name == b"mGBA", info.name
    core.retro_init()
    rom = original_rom()
    rom_path = Path(directory) / "original-test.gba"
    rom_path.write_bytes(rom)
    buffer = C.create_string_buffer(bytes(rom))
    game = Game(os.fsencode(rom_path), C.cast(buffer, C.c_void_p), len(rom), None)
    core.retro_load_game.argtypes, core.retro_load_game.restype = [C.POINTER(Game)], C.c_bool
    assert core.retro_load_game(C.byref(game))
    for _ in range(120):
        core.retro_run()
    assert frames and samples and polls and maps
    def pointer(address, size):
        for start, length, ptr, offset, disconnect, space in maps:
            if start <= address and address-start+size <= length and ptr and not disconnect and not space:
                return ptr + offset + address-start
        raise AssertionError(f"Unmapped address {address:#x}")
    assert C.c_uint8.from_address(pointer(0x02000123, 1)).value == 0x6d
    assert C.c_uint8.from_address(pointer(0x03000040, 1)).value == 0x42
    mapped = getattr(core, "battlehud_read_memory", None)
    if mapped:
        mapped.argtypes, mapped.restype = [C.c_size_t, C.c_void_p, C.c_size_t], C.c_bool
        for address, expected in ((0x02000123, 0x6d), (0x03000040, 0x42)):
            v = C.c_uint8()
            assert mapped(address, C.byref(v), 1) and v.value == expected
        v = C.c_uint8(99)
        assert not mapped(0x02040000, C.byref(v), 1) and v.value == 99
        # Fill only this original test ROM's mapped RAM while execution is stopped.
        def write(address, payload):
            C.memmove(pointer(address, len(payload)), payload, len(payload))
        def u32(address, value):
            write(address, struct.pack("<I", value))
        summary = 0x02001000
        u32(0x030022c4, 0x081bfab5)
        write(0x02037fdb, b"\0")
        write(0x03005e00, bytes(16*40))
        u32(0x03005e00, 0x081c0511)
        write(0x03005e04, b"\1")
        u32(0x0203cf1c, summary)
        u32(summary, 0x020244ec)
        write(0x020244e9, b"\1")
        write(summary+0x40bc, bytes([0, 0, 0, 0, 1]))
        mon, clear = bytearray(100), bytearray(48)
        struct.pack_into("<II", mon, 0, 0, 0xdeadbeef)
        mon[19], mon[84] = 2, 12
        struct.pack_into("<H", clear, 0, 277)  # Treecko, internal species ID
        clear[24:30] = bytes([0, 1, 252, 4, 128, 125])
        iv = 31 | (1<<5) | (17<<10) | (2<<15) | (29<<20) | (3<<25)
        struct.pack_into("<I", clear, 40, iv)
        struct.pack_into("<H", mon, 28, sum(struct.unpack("<24H", clear)) & 65535)
        key = struct.pack("<I", 0xdeadbeef)
        mon[32:80] = bytes(b ^ key[i%4] for i, b in enumerate(clear))
        write(summary+12, bytes(mon))
        write(0x020244ec, bytes(mon))
        @READ
        def read(ctx, addr, out, n):
            return mapped(addr, out, n)
        find = core.game_profile_find
        find.argtypes, find.restype = [C.c_char_p], C.c_void_p
        decode = core.battle_decode
        decode.argtypes, decode.restype = [C.c_void_p, READ, C.c_void_p], BattleState
        snapshot = decode(find(b"f3ae088181bf583e55daf962a92bb46f4f1d07b7"), read, None)
        assert snapshot.training.visible and snapshot.training.species == 277
        assert list(snapshot.training.dv) == [31, 1, 17, 29, 3, 2]
        assert list(snapshot.training.ev) == [0, 1, 252, 128, 125, 4]
        calculate = core.hidden_power_calculate
        calculate.argtypes, calculate.restype = [C.c_uint, C.POINTER(C.c_uint8), C.POINTER(HiddenPower)], C.c_bool
        hp = HiddenPower()
        assert calculate(3, snapshot.training.dv, C.byref(hp)) and (hp.type, hp.power) == (6, 56)  # Ice 56
        pixels, w, h, f = frames[-1]
        bpp = 4 if f == 1 else 2
        source = C.create_string_buffer(pixels)
        hud = TypeHud()
        draw, clear_hud = core.type_hud_draw, core.type_hud_clear
        draw.argtypes = [C.POINTER(TypeHud), C.POINTER(BattleState), C.c_void_p, C.c_uint, C.c_uint, C.c_size_t, C.c_int]
        draw.restype = C.c_void_p
        out = draw(C.byref(hud), C.byref(snapshot), source, w, h, w*bpp, f)
        assert out and out != C.addressof(source)
        for y in range(h):
            for x in range(w):
                if x < 80 or y < 112:
                    offset = (y*w+x)*bpp
                    assert C.string_at(out+offset, bpp) == pixels[offset:offset+bpp]
        assert source.raw[:-1] == pixels
        clear_hud.argtypes = [C.POINTER(TypeHud)]
        clear_hud(C.byref(hud))
        getter = core.battlehud_get_battle_state
        getter.argtypes, getter.restype = [C.POINTER(BattleState)], C.c_bool
        assert not getter(C.byref(snapshot)), "Original test ROM must remain unrecognized"
    core.retro_serialize_size.restype = C.c_size_t
    state_size = core.retro_serialize_size()
    assert state_size
    state = C.create_string_buffer(state_size)
    core.retro_serialize.argtypes, core.retro_serialize.restype = [C.c_void_p, C.c_size_t], C.c_bool
    core.retro_unserialize.argtypes, core.retro_unserialize.restype = [C.c_void_p, C.c_size_t], C.c_bool
    assert core.retro_serialize(state, state_size)
    C.c_uint8.from_address(pointer(0x02000123, 1)).value = 0
    assert core.retro_unserialize(state, state_size)
    assert C.c_uint8.from_address(pointer(0x02000123, 1)).value == 0x6d
    if mapped:
        v = C.c_uint8()
        assert mapped(0x02000123, C.byref(v), 1) and v.value == 0x6d
    stats = {"frames": len(frames), "polls": len(polls), "geometry": list(frames[-1][1:]),
             "video": hashlib.sha256(b"".join(x[0] for x in frames)).hexdigest(),
             "audio": hashlib.sha256(b"".join(samples)).hexdigest(), "state_size": state_size}
    core.retro_unload_game()
    if mapped:
        assert not mapped(0x02000123, C.byref(C.c_uint8()), 1)
    core.retro_deinit()
    (Path(directory)/"result.json").write_text(json.dumps(stats))

def main():
    if sys.argv[1] == "--worker":
        worker(sys.argv[2], sys.argv[3])
        return
    wrapper, backend = [str(Path(p).resolve()) for p in sys.argv[1:3]]
    results = []
    for core in (backend, wrapper):
        with tempfile.TemporaryDirectory() as directory:
            run = subprocess.run([sys.executable, __file__, "--worker", core, directory],
                                 env=dict(os.environ, LIBRETRO_BATTLEHUD_BACKEND=backend, LIBRETRO_BATTLEHUD_TEST_MARKER="0"),
                                 capture_output=True, text=True, timeout=60)
            if run.returncode:
                raise RuntimeError(f"{core}: {run.stdout}\n{run.stderr}")
            results.append(json.loads((Path(directory)/"result.json").read_text()))
    assert results[0] == results[1], results
    print("Real mGBA direct/proxy video/audio/RAM/state parity and Emerald decoder/renderer fixture passed", results[0])

if __name__ == "__main__":
    main()
