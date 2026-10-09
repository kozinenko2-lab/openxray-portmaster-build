#!/usr/bin/env python3
"""Audit direct internal CALL targets in the canonical AirXonix PE.

The script compares every relative/direct CALL target inside the PE .text section
with hexadecimal addresses mentioned by the reconstruction source and notes.
It is deliberately conservative: an address being mentioned is not proof that
its behaviour is correct; the report is a coverage aid for reverse engineering.
"""
from __future__ import annotations
import argparse
import json
import re
import subprocess
from collections import Counter, defaultdict
from pathlib import Path

HEX_RE = re.compile(r"0x(?:00)?([0-9a-fA-F]{6,8})")
CALL_RE = re.compile(r"^\s*([0-9a-fA-F]+):.*?\bcall\s+([0-9a-fA-Fx]+)\b")
SECTION_RE = re.compile(r"^\s*\d+\s+\.text\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+")
TEXT_SUFFIXES = {'.c','.cc','.cpp','.cxx','.h','.hh','.hpp','.md','.txt','.cmake'}
SKIP_PARTS = {'.git','build','build-host','CMakeFiles'}

def run(*cmd: str) -> str:
    return subprocess.check_output(cmd, text=True, errors='replace')

def text_range(exe: Path) -> tuple[int,int]:
    for line in run('objdump','-h',str(exe)).splitlines():
        m=SECTION_RE.match(line)
        if m:
            size=int(m.group(1),16); vma=int(m.group(2),16)
            return vma,vma+size
    raise RuntimeError('could not locate .text section')

def direct_calls(exe: Path, lo: int, hi: int):
    dis=run('objdump','-d','-Mintel',str(exe))
    refs=Counter(); callers=defaultdict(list)
    for line in dis.splitlines():
        m=CALL_RE.match(line)
        if not m: continue
        caller=int(m.group(1),16)
        raw=m.group(2)
        try: target=int(raw,16)
        except ValueError: continue
        if lo <= caller < hi and lo <= target < hi:
            refs[target]+=1; callers[target].append(caller)
    return refs,callers

def corpus_addresses(root: Path):
    seen=set()
    for p in root.rglob('*'):
        if not p.is_file() or p.suffix.lower() not in TEXT_SUFFIXES: continue
        if any(part in SKIP_PARTS for part in p.parts): continue
        try: text=p.read_text(errors='ignore')
        except OSError: continue
        for m in HEX_RE.finditer(text):
            seen.add(int(m.group(1),16))
    return seen

def main() -> int:
    ap=argparse.ArgumentParser()
    ap.add_argument('exe',type=Path)
    ap.add_argument('--root',type=Path,default=Path('.'))
    ap.add_argument('--game-end',type=lambda s:int(s,0),default=None,
                    help='optional exclusive end VA for the game-owned code region')
    ap.add_argument('--json',type=Path)
    args=ap.parse_args()
    text_lo,text_hi=text_range(args.exe)
    lo=text_lo
    hi=min(text_hi,args.game_end) if args.game_end is not None else text_hi
    refs,callers=direct_calls(args.exe,lo,hi)
    corpus=corpus_addresses(args.root)
    missing=[]
    for target,count in sorted(refs.items(), key=lambda kv:(-kv[1],kv[0])):
        if target not in corpus:
            missing.append({'target':f'0x{target:08X}','references':count,
                            'callers':[f'0x{x:08X}' for x in callers[target]]})
    report={
        'text_start':f'0x{text_lo:08X}', 'text_end':f'0x{text_hi:08X}',
        'audited_start':f'0x{lo:08X}', 'audited_end':f'0x{hi:08X}',
        'direct_internal_call_targets':len(refs),
        'targets_mentioned_in_corpus':len(refs)-len(missing),
        'targets_not_mentioned_in_corpus':missing,
    }
    print(json.dumps(report,indent=2))
    if args.json:
        args.json.write_text(json.dumps(report,indent=2)+'\n')
    return 0
if __name__=='__main__': raise SystemExit(main())
