"""Build an isolated NextUI H700 test Pak; no ROM or backend is redistributed."""
import argparse
import hashlib
import json
from pathlib import Path
import stat
import struct
import zipfile

ROOT = Path(__file__).resolve().parents[1]

def build(core, output, commit):
    data = Path(core).read_bytes()
    if len(data) < 20 or data[:6] != b'\x7fELF\x02\x01' or struct.unpack_from('<HH', data, 16) != (3, 183):
        raise ValueError('Expected ELF64 little-endian AArch64 shared library')
    manifest = {
        'status': 'test-package-device-acceptance-pending', 'platform': 'h700',
        'firmware_repository': 'pvaibhav/NextUI', 'firmware_tag': 'h700-rc11',
        'firmware_commit': 'cd73cd85c08449d939b7bfcb2e1ed3ed721f1dfa',
        'wrapper_commit': commit, 'wrapper_sha256': hashlib.sha256(data).hexdigest(),
        'emulator_tag': 'PBH', 'frontend_core_name': 'gambatte',
        'backend': 'firmware CORES_PATH/gambatte_libretro.so (not bundled)',
        'rom_sha1': 'f2f52230b536214ef7c9924f483392993e226cfb',
    }
    files = {
        'Emus/h700/PBH.pak/launch.sh': ((ROOT / 'packaging/nextui/h700/PBH.pak/launch.sh').read_bytes(), 0o755),
        'Emus/h700/PBH.pak/gambatte_libretro.so': (data, 0o755),
        'INSTALL.md': ((ROOT / 'docs/nextui-package.md').read_bytes(), 0o644),
        'manifest.json': ((json.dumps(manifest, indent=2) + '\n').encode(), 0o644),
    }
    output = Path(output)
    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, 'w', compression=zipfile.ZIP_DEFLATED) as archive:
        for name, (contents, mode) in files.items():
            entry = zipfile.ZipInfo(name, date_time=(2026, 1, 1, 0, 0, 0))
            entry.create_system = 3
            entry.external_attr = (stat.S_IFREG | mode) << 16
            entry.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(entry, contents)
    output.with_suffix('.sha256').write_text(hashlib.sha256(output.read_bytes()).hexdigest() + '  ' + output.name + '\n')

if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--core', required=True)
    p.add_argument('--output', required=True)
    p.add_argument('--commit', required=True)
    args = p.parse_args()
    build(args.core, args.output, args.commit)
