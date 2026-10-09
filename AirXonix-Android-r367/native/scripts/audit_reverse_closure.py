#!/usr/bin/env python3
from pathlib import Path
import re, sys
root=Path(__file__).resolve().parents[1]
src=root/'src'
issues=[]
# Live-code unresolved markers only; historical docs are intentionally excluded.
for p in src.rglob('*'):
    if p.suffix not in {'.cpp','.hpp'}: continue
    t=p.read_text(errors='ignore')
    for i,line in enumerate(t.splitlines(),1):
        if re.search(r'\b(?:TODO|FIXME|UNRESOLVED|NOT IMPLEMENTED)\b',line,re.I):
            # Known explanatory comments containing "old ... unresolved" still count;
            # force them to be cleaned rather than silently ignored.
            issues.append(f'{p.relative_to(root)}:{i}: {line.strip()}')
# Every LegacyModelFactory assignment in renderer.cpp must have at least one later use.
rp=root/'src/render/renderer.cpp'; rt=rp.read_text(errors='ignore')
for m in re.finditer(r'\b([A-Za-z_]\w*_)\s*=\s*LegacyModelFactory::',rt):
    name=m.group(1)
    if len(re.findall(r'\b'+re.escape(name)+r'\b',rt)) < 2:
        issues.append(f'renderer model has no consumer: {name}')
# The shipping launcher banner must not predate the current source snapshot.
launcher=(root/'portmaster/AirXonix.sh').read_text(errors='ignore')
if 'AirXonix FINAL' not in launcher:
    issues.append('PortMaster launcher FINAL banner is stale')
if issues:
    print('REVERSE_CLOSURE_AUDIT FAIL')
    print('\n'.join(issues))
    sys.exit(1)
print('REVERSE_CLOSURE_AUDIT PASS')
