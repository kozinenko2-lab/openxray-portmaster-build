#!/usr/bin/env python3
"""Extract the three known custom resources from AirXonix.wrp.exe.
Offsets are resolved from the PE .rsrc RVA discovered during static analysis.
This tool requires the user's own original executable; no game assets are distributed.
"""
from pathlib import Path
import argparse, struct
RSRC_RVA=0x021BA000
RSRC_FILE_OFF=0x00046000
RES={
    'BMPPACK': (0x021BA570,0x001157E0),
    'SOUNDINF':(0x022CFD50,0x00001CA4),
    'WAVEPACK': (0x022D19F4,0x000001A0),
}
def main():
    ap=argparse.ArgumentParser(); ap.add_argument('exe'); ap.add_argument('-o','--out',default='data_extracted'); a=ap.parse_args()
    blob=Path(a.exe).read_bytes(); out=Path(a.out); out.mkdir(parents=True,exist_ok=True)
    for name,(rva,size) in RES.items():
        off=RSRC_FILE_OFF+(rva-RSRC_RVA); b=blob[off:off+size]
        if len(b)!=size: raise SystemExit(f'{name}: truncated executable/resource')
        (out/(name+'.bin')).write_bytes(b); print(f'{name}: {len(b)} bytes')
    b=(out/'WAVEPACK.bin').read_bytes()
    with (out/'WAVEPACK.txt').open('w',encoding='utf-8') as f:
        for i in range(0,len(b),8):
            raw=b[i:i+4]; name=raw[::-1].decode('latin1'); value=struct.unpack_from('<I',b,i+4)[0]
            f.write(f'{i//8:02d} {name!r} 0x{value:08X} {value}\n')
if __name__=='__main__': main()
