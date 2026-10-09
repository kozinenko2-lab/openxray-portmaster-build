#!/usr/bin/env python3
from pathlib import Path
import sys,zipfile
root=Path(__file__).resolve().parents[1]
a=root/'app/src/main/assets'
expected=['AirXonix.wrp.exe','AirXonix-cleanroom.zip'] + ['MUSIC/%02d.mus'%x for x in range(10)] + ['MUSIC/29.MUS']
missing=[x for x in expected if not (a/x).is_file()]
assert not missing, missing
with zipfile.ZipFile(a/'AirXonix-cleanroom.zip') as z:
    assert z.testzip() is None
    m=z.read('cleanroom/MANIFEST.txt')
    assert b'clean-room' in m
print('ASSET_VERIFICATION PASS original exe + 11 original tracks + cleanroom ZIP')
