#!/usr/bin/env python3
"""Package existing native outputs; fail if the expected binary is absent/wrong."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import zipfile

NAME = 'BaakProduction Master'

def check_binary(path, platform):
    data = path.read_bytes()
    if platform == 'windows':
        if len(data) < 64 or data[:2] != b'MZ':
            raise ValueError(f'Not a Windows PE binary: {path}')
        pe = struct.unpack_from('<I', data, 60)[0]
        if data[pe:pe+4] != b'PE\0\0' or struct.unpack_from('<H', data, pe+4)[0] != 0x8664:
            raise ValueError(f'Not a Windows x64 PE binary: {path}')
    elif data[:4] not in (b'\xca\xfe\xba\xbe', b'\xbe\xba\xfe\xca', b'\xca\xfe\xba\xbf', b'\xbf\xba\xfe\xca'):
        raise ValueError(f'Not a universal Mach-O binary: {path}')

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--platform', choices=['windows', 'macos'], required=True)
    ap.add_argument('--artifacts', type=Path, required=True)
    ap.add_argument('--output', type=Path, required=True)
    a = ap.parse_args()
    stem = 'BaakProductionMaster-Windows-x64' if a.platform == 'windows' else 'BaakProductionMaster-macOS-Universal'
    out = a.output.resolve()
    stage = out / stem
    if stage.exists():
        shutil.rmtree(stage)
    stage.mkdir(parents=True)
    src = a.artifacts.resolve()
    if a.platform == 'windows':
        entries = [('VST3', NAME+'.vst3'), ('Standalone', NAME+'.exe')]
        check_binary(src/'VST3'/(NAME+'.vst3')/'Contents'/'x86_64-win'/(NAME+'.vst3'), 'windows')
        check_binary(src/'Standalone'/(NAME+'.exe'), 'windows')
        instructions = '''Windows x64 alpha build\n\nCopy the entire BaakProduction Master.vst3 bundle into:\nC:\\Program Files\\Common Files\\VST3\\\nThen rescan VST3 plugins in your DAW. Administrator permission may be needed for this folder.\nThe standalone EXE processes an audio device; it is not a file player.\nInstall the Microsoft Visual C++ x64 Redistributable if the system reports missing MSVC runtime DLLs.\n'''
    else:
        entries = [('VST3', NAME+'.vst3'), ('AU', NAME+'.component'), ('Standalone', NAME+'.app')]
        for folder, name in entries:
            check_binary(src/folder/name/'Contents'/'MacOS'/NAME, 'macos')
        instructions = '''macOS Intel + Apple Silicon alpha build\n\nCopy the VST3 bundle to ~/Library/Audio/Plug-Ins/VST3/\nCopy the AU component to ~/Library/Audio/Plug-Ins/Components/\nRestart your DAW and rescan. Logic uses the AU component.\nThe app can be copied to Applications.\nBundles are ad-hoc signed, not Developer ID signed or Apple notarized. macOS may block a downloaded build; use a properly signed/notarized release for normal distribution.\n'''
    for folder, name in entries:
        item = src/folder/name
        if item.is_dir():
            shutil.copytree(item, stage/name, symlinks=True)
        else:
            shutil.copy2(item, stage/name)
    instructions += '\nThis is alpha 0.1. It does not contain trained AI or validated true-peak limiting. See the source and validation record for exact capabilities.\n'
    (stage/'INSTALL.txt').write_text(instructions)
    project = Path(__file__).resolve().parents[1]
    for name in ['LICENSE.txt', 'THIRD_PARTY.md']:
        shutil.copy2(project/name, stage/name)
    # Include corresponding project source; CI supplies the pinned JUCE source independently.
    source = stage/'Source'
    source.mkdir()
    for name in ['Source', 'tests', 'tools']:
        shutil.copytree(project/name, source/name)
    shutil.copy2(project/'CMakeLists.txt', source/'CMakeLists.txt')
    shutil.copy2(project/'README.md', source/'README.md')
    (stage/'BUILD.json').write_text(json.dumps({'platform':a.platform,'version':'0.1.0','juce_revision':'d60b7a78030661574a615b586d9325ba7a7fa7dd','notarized':False}, indent=2))
    archive = out/(stem+'.zip')
    with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
        for f in sorted(stage.rglob('*')):
            if f.is_file(): z.write(f, str(f.relative_to(out)))
    print(archive)
    print('SHA256', hashlib.sha256(archive.read_bytes()).hexdigest())

if __name__ == '__main__':
    main()
