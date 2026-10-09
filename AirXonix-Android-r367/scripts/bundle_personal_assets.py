#!/usr/bin/env python3
"""Insert user's own commercial resources before building a PERSONAL APK.
Usage: python3 scripts/bundle_personal_assets.py /path/to/original/game
The public repository intentionally never includes these data files.
"""
from pathlib import Path
import shutil, sys
root=Path(__file__).resolve().parents[1]
src=Path(sys.argv[1]) if len(sys.argv)>1 else None
if src is None or not src.is_dir():
    raise SystemExit('Provide a directory containing AirXonix.wrp.exe and MUSIC/')
dst=root/'app/src/main/assets'
exe=src/'AirXonix.wrp.exe'
if not exe.is_file(): raise SystemExit('Missing '+str(exe))
shutil.copy2(exe,dst/exe.name)
tracks=list((src/'MUSIC').glob('*.mus'))+list((src/'MUSIC').glob('*.MUS'))
if not tracks: raise SystemExit('Missing original MUSIC tracks')
(dst/'MUSIC').mkdir(exist_ok=True)
for fp in tracks: shutil.copy2(fp,dst/'MUSIC'/fp.name)
print('Personal resources installed:',len(tracks),'tracks and original EXE. DO NOT PUSH these files to a public repository.')
