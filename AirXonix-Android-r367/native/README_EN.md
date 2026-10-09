# AirXonix native port — working source snapshot r366

This is the cleaned source-only tree of the native AirXonix reimplementation for Linux ARM64 / PortMaster using SDL2 and OpenGL ES 2.0.

## r348–r349 changes

- Restored the original gameplay crawler render structure: the pink 3D body is now a separate body pass using texture slot 3, while `0x418840/0x418A90` are again treated as flat field projections/shadows rather than the visible enemy body.
- Verified the original translation, scale and X/Y rotation formulas for the large AirXonix 3D logo. The visible discrepancy was in GLES render state: alpha testing incorrectly discarded black texels from the RGB565 LOGO texture. Alpha test is now disabled for LOGO/LAXY, matching the original D3D7 path.
- Added/updated r348/r349 regression contracts; full self-contained run: **241/241 PASS**.

## Status

The registered/full-game path is already playable, but full reverse engineering of the original EXE is still in progress. In r349 the self-contained suite that does not require external generated assets passes **233/233 tests** with assertions active; four additional resource/clean-room tests are registered only when their generated artifacts are present.

The strict direct-call audit currently finds **692** unique internal CALL targets in `AirXonix.wrp.exe`; **292** are explicitly mentioned in the present source/documentation corpus and the remaining targets still need classification (many are expected to be CRT/support code). `REVERSE_CLOSURE_AUDIT PASS` means the current mandatory reverse contracts are satisfied, not that the EXE is already 100% decompiled.

The clean-room GLES2 port intentionally does not reproduce obsolete registration/DRM screens, the restricted/unregistered mode selector, or legacy Direct3D fallback paths that are irrelevant on the GLES2 target. Those branches are classified in `AIRXONIX_REVERSE_ENGINEERING_GUIDE.txt`.

## What is in this folder

- `src/` — game, renderer, audio, platform and resource source code.
- `tests/` — reverse-engineering regression tests.
- `tools/` — clean-room resource generation/extraction utilities and audit helpers.
- `scripts/` — PortMaster ARM64 build helper and reverse-closure audit.
- `cmake/` — CMake helper modules.
- `portmaster/AirXonix.sh` — standalone PortMaster launcher.
- `CMakeLists.txt` — native build configuration.
- `README_EN.md`, `README_RU.md` — final documentation.
- `AIRXONIX_REVERSE_ENGINEERING_GUIDE.txt` — consolidated address/function/global map and reverse-engineering workflow.

Historical snapshots, disassembly dumps, build directories, generated assets and original commercial binaries are intentionally excluded from this final source-only package.

## Recreate the clean-room test resources

The final archive is source-only, so generated music/textures/resource blobs are not stored in it. To reproduce the exact resource-dependent tests, generate them first (requires Python, NumPy and Pillow):

```sh
python3 tools/generate_cleanroom_resources.py assets
```

This also creates `AirXonix-cleanroom.zip`. Both generated outputs are intentionally excluded from the source archive and can be regenerated at any time.

## Host / headless validation

```sh
cmake -S . -B build-headless \
  -DAIRXONIX_BUILD_RUNTIME=OFF \
  -DAIRXONIX_BUILD_TESTS=ON \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-headless -j2
ctest --test-dir build-headless --output-on-failure
```

Expected result without external generated assets: **241/241 tests passed**.

Run the live reverse-closure audit with:

```sh
python3 scripts/audit_reverse_closure.py
```

## Native Linux build

Dependencies: CMake 3.16+, C++17, SDL2, OpenGL ES 2.0, pkg-config and optionally libpng.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
```

For a resource-free build using the built-in clean-room replacements:

```sh
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DAIRXONIX_RESOURCE_FREE=ON
cmake --build build -j2
```

## PortMaster ARM64 build

Set a PortMaster-compatible ARM64 sysroot and cross compiler, then run:

```sh
export AIRXONIX_SYSROOT=/path/to/sysroot
export CC=aarch64-linux-gnu-gcc
export CXX=aarch64-linux-gnu-g++
./scripts/build_portmaster_arm64.sh
```

The build helper forces the ARMv8-A / Cortex-A53 baseline required by H700/RK3326-class devices.

## Original resources

The source can run in clean-room/resource-free mode. For fidelity testing it can also read supported original resources from an unmodified `AirXonix.wrp.exe`; no original executable or commercial assets are included here.

The reverse-engineering guide documents the PE layout, resource RVAs, runtime addresses, algorithms and all recovered cross-references needed to reproduce the analysis.


### r350 — Settings/M1 visual fidelity
- Restored the Settings selected-row slide (`-0.0028 * row`, `0.00005/ms`) and its camera coupling `Z=offset*0.22-0.003` from `0x413B67..0x413CD4`.
- Slider knobs now receive the original fixed Z=`0x200` quarter-turn before their X spin.
- Labels/sliders/speech switch are multiplied by the shared Settings fade (`counter>>3`).
- M1/M2 background/decor animation now consumes the single game-frame `dt` rather than resampling `SDL_GetTicks()`, avoiding XONIX animation desynchronization under frame drops.
- Regression result for this snapshot: **241/241 PASS**; `REVERSE_CLOSURE_AUDIT PASS`; `AirXonix.sh` passes `bash -n`.

### Iterations r351–r366 — literal fidelity and object state
- Removed the old TIME/SLOW compensation matrix: QUESTION/TIME/SLOW now share the original orientation, while LIFE alone keeps its separate Y matrix.
- Restored literal cap normals for procedural extrusions: radial XY component `0.15` with Z=`-0.8/+1.0`, bringing heart/letter lighting back to the D3D7 behavior.
- Information page 3 now uses the exact fountain contract: shared MSVC RNG, 128-entry pool, 16-particle ring buffer, original velocity/gravity formulas, and half-lighting for the first large object.
- Fixed Homing/Eraser reset/lifetime semantics and sequential pickup initialization, preserving native cross-level fields and previous pickup coordinates instead of synthetic sentinel positions.
- Added contracts for airborne cached-world/heading behavior, field-debris lifetime, and pickup-smash slot semantics.
- Current autonomous regression: **241/241 PASS**; `REVERSE_CLOSURE_AUDIT PASS`; `portmaster/AirXonix.sh` passes `bash -n`.
