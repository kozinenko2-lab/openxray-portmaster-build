#!/usr/bin/env python3
from pathlib import Path
import argparse
import re
import shutil
import sys

INCLUDE_RE = re.compile(r'(^\s*#\s*include\s+")([^"]+)(")', re.M)
INCLUDE_SCAN_RE = re.compile(r'^\s*#\s*include\s+"([^"]+)"', re.M)

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("source_gl")
    ap.add_argument("output_gl")
    args = ap.parse_args()

    src = Path(args.source_gl).resolve()
    out = Path(args.output_gl).resolve()
    if not src.is_dir():
        raise SystemExit(f"missing source shader directory: {src}")

    if out.exists():
        shutil.rmtree(out)
    out.parent.mkdir(parents=True, exist_ok=True)
    shutil.copytree(src, out)

    src_files = [p for p in src.rglob("*") if p.is_file()]
    relmap = {p.relative_to(src).as_posix().lower(): p.relative_to(src).as_posix() for p in src_files}

    fixes = []
    for p in [x for x in out.rglob("*") if x.is_file()]:
        rel_src = p.relative_to(out)
        text = p.read_text(errors="ignore")

        def repl(m):
            inc = m.group(2)
            norm = inc.replace("\\", "/")
            candidate = (rel_src.parent / norm).as_posix()
            actual = relmap.get(candidate.lower()) or relmap.get(norm.lower())
            if actual is None:
                return m.group(0)

            desired = actual.replace("/", "\\")
            if desired != inc:
                fixes.append((rel_src.as_posix(), inc, desired))
                return m.group(1) + desired + m.group(3)
            return m.group(0)

        new_text = INCLUDE_RE.sub(repl, text)
        if new_text != text:
            p.write_text(new_text)

    missing = []
    refs = 0
    for p in [x for x in out.rglob("*") if x.is_file()]:
        text = p.read_text(errors="ignore")
        for inc in INCLUDE_SCAN_RE.findall(text):
            refs += 1
            norm = inc.replace("\\", "/")
            if not (p.parent / norm).exists() and not (out / norm).exists():
                missing.append((p.relative_to(out).as_posix(), inc))

    print(f"H700 GLES shader files: {len(src_files)}")
    print(f"Include references checked: {refs}")
    print(f"Linux case fixes: {len(fixes)}")
    for file_name, before, after in fixes:
        print(f"  {file_name}: {before} -> {after}")

    if missing:
        print("ERROR: unresolved shader includes:", file=sys.stderr)
        for file_name, inc in missing:
            print(f"  {file_name}: {inc}", file=sys.stderr)
        return 1

    print("Unresolved shader includes: 0")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
