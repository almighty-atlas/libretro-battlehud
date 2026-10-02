"""Check package layout and launcher forwarding with a mocked MinArch, not a device."""
import importlib.util
import json
import os
from pathlib import Path
import stat
import struct
import subprocess
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('package_nextui', ROOT / 'scripts/package_nextui.py')
package = importlib.util.module_from_spec(spec)
spec.loader.exec_module(package)

with tempfile.TemporaryDirectory(prefix='battlehud pak ') as temporary:
    # macOS temporary directories can be symlink aliases; launch uses cd -P.
    t = Path(temporary).resolve()
    data = bytearray(20)
    data[:6] = b'\x7fELF\x02\x01'
    struct.pack_into('<HH', data, 16, 3, 183)
    core, archive = t / 'wrapper.so', t / 'test.zip'
    core.write_bytes(data)
    package.build(core, archive, 'test-commit')
    with zipfile.ZipFile(archive) as z:
        names = set(z.namelist())
        assert names == {'Emus/h700/PBH.pak/launch.sh', 'Emus/h700/PBH.pak/gambatte_libretro.so', 'INSTALL.md', 'manifest.json'}
        assert z.read('Emus/h700/PBH.pak/gambatte_libretro.so') == data
        assert stat.S_IMODE(z.getinfo('Emus/h700/PBH.pak/launch.sh').external_attr >> 16) == 0o755
        manifest = json.loads(z.read('manifest.json'))
        assert manifest['emulator_tag'] == 'PBH' and manifest['wrapper_commit'] == 'test-commit'
        z.extractall(t / 'sd')
    assert archive.with_suffix('.sha256').is_file()
    core.write_bytes(b'wrong architecture')
    try:
        package.build(core, t / 'bad.zip', 'test')
    except ValueError:
        pass
    else:
        raise AssertionError('bad ELF accepted')
    assert not (t / 'bad.zip').exists()
    sd = t / 'sd'
    env = dict(os.environ)
    for variable, directory in [('CORES_PATH', '.system/h700/cores'), ('BIOS_PATH', 'Bios'),
                                ('SAVES_PATH', 'Saves'), ('CHEATS_PATH', 'Cheats'),
                                ('LOGS_PATH', 'logs'), ('USERDATA_PATH', '.userdata/h700')]:
        p = sd / directory
        p.mkdir(parents=True, exist_ok=True)
        env[variable] = str(p)
    backend = Path(env['CORES_PATH']) / 'gambatte_libretro.so'
    backend.write_bytes(b'original firmware core')
    normal_save = Path(env['SAVES_PATH']) / 'GBC/keep.sav'
    normal_save.parent.mkdir()
    normal_save.write_bytes(b'original save')
    rom = sd / "Roms/Pokemon BattleHUD (PBH)/Crystal's test.gbc"
    rom.parent.mkdir(parents=True)
    rom.write_bytes(b'original ROM')
    host = t / 'host.py'
    host.write_text('import json,os,sys\nfrom pathlib import Path\n'
                    'Path(os.environ["TEST_CALL"]).write_text(json.dumps(dict(args=sys.argv[1:],'
                    'backend=os.environ.get("LIBRETRO_BATTLEHUD_BACKEND"),'
                    'debug=os.environ.get("LIBRETRO_BATTLEHUD_DEBUG"),cwd=os.getcwd())))\n')
    stubdir = t / 'bin'
    stubdir.mkdir()
    stub = stubdir / 'minarch.elf'
    stub.write_text('#!/bin/sh\nexec python3 "$TEST_HOST" "$@"\n')
    stub.chmod(0o755)
    env.update(PATH=str(stubdir) + os.pathsep + os.environ['PATH'], PLATFORM='h700',
               TEST_HOST=str(host), TEST_CALL=str(t / 'call.json'))
    env.pop('LIBRETRO_BATTLEHUD_DEBUG', None)
    launch = sd / 'Emus/h700/PBH.pak/launch.sh'
    result = subprocess.run(['sh', str(launch), str(rom)], env=env, capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    call = json.loads(Path(env['TEST_CALL']).read_text())
    assert call['args'] == [str(launch.parent / 'gambatte_libretro.so'), str(rom)]
    assert call['backend'] == str(backend) and call['debug'] == '1'
    assert call['cwd'] == env['USERDATA_PATH']
    assert (Path(env['SAVES_PATH']) / 'PBH').is_dir()
    assert (Path(env['LOGS_PATH']) / 'PBH.txt').is_file()
    assert backend.read_bytes() == b'original firmware core'
    assert normal_save.read_bytes() == b'original save' and rom.read_bytes() == b'original ROM'
    for changes, args in [({'PLATFORM': 'tg5040'}, [str(rom)]),
                          ({'CORES_PATH': str(t / 'missing')}, [str(rom)]),
                          ({'USERDATA_PATH': ''}, [str(rom)]), ({}, [])]:
        bad_env = dict(env, **changes)
        Path(env['TEST_CALL']).unlink(missing_ok=True)
        r = subprocess.run(['sh', str(launch), *args], env=bad_env, capture_output=True, text=True)
        assert r.returncode and not Path(env['TEST_CALL']).exists()
print('NextUI package layout, architecture guard, quoting, isolation and launcher mock passed')
