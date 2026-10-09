#!/usr/bin/env python3
"""Unpack AirXonix graphics/music into ordinary PNG/WAV files.

Usage:
  python3 unpack_assets.py /path/to/AirXonix.wrp.exe --music-dir /path/to/MUSIC -o assets

The BMPPACK layout was recovered from the original engine:
  u32 record_size, u32 reversed_fourcc, u32 width, u32 height, RGB565 pixels.
The music .mus files 00..09 are raw unsigned 8-bit stereo PCM at 22050 Hz.
MUSIC/29.MUS is not music: it is the monophonic 8-bit SFX sample bank indexed by WAVEPACK.
"""
from __future__ import annotations
import argparse
import binascii
import json
from pathlib import Path
import struct
import wave
import zlib

RSRC_RVA = 0x021BA000
RSRC_FILE_OFF = 0x00046000
BMPPACK_RVA = 0x021BA570
BMPPACK_SIZE = 0x001157E0
WAVEPACK_RVA = 0x022D19F4
WAVEPACK_SIZE = 0x000001A0


def png_chunk(kind: bytes, payload: bytes) -> bytes:
    body = kind + payload
    return struct.pack('>I', len(payload)) + body + struct.pack('>I', binascii.crc32(body) & 0xFFFFFFFF)


def write_rgb_png(path: Path, width: int, height: int, rgb: bytes) -> None:
    # PNG color type 2 = RGB, 8 bits/channel, filter 0 on every scanline.
    rows = bytearray()
    stride = width * 3
    for y in range(height):
        rows.append(0)
        rows.extend(rgb[y * stride:(y + 1) * stride])
    data = b'\x89PNG\r\n\x1a\n'
    data += png_chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 2, 0, 0, 0))
    data += png_chunk(b'IDAT', zlib.compress(bytes(rows), 9))
    data += png_chunk(b'IEND', b'')
    path.write_bytes(data)


def expand_565(raw: bytes) -> bytes:
    out = bytearray(len(raw) // 2 * 3)
    oi = 0
    for (v,) in struct.iter_unpack('<H', raw):
        r5 = (v >> 11) & 31
        g6 = (v >> 5) & 63
        b5 = v & 31
        out[oi] = (r5 << 3) | (r5 >> 2)
        out[oi + 1] = (g6 << 2) | (g6 >> 4)
        out[oi + 2] = (b5 << 3) | (b5 >> 2)
        oi += 3
    return bytes(out)


def sanitize_fourcc(name: str) -> str:
    # FourCCs are short ASCII identifiers in this game. Keep useful punctuation.
    return ''.join(c if (c.isalnum() or c in '+-$_.') else '_' for c in name)


def extract_bmppack(exe: Path, out_dir: Path) -> list[dict]:
    blob = exe.read_bytes()
    off = RSRC_FILE_OFF + (BMPPACK_RVA - RSRC_RVA)
    pack = blob[off:off + BMPPACK_SIZE]
    if len(pack) != BMPPACK_SIZE:
        raise RuntimeError('AirXonix.wrp.exe is truncated or does not match the analysed build')

    out_dir.mkdir(parents=True, exist_ok=True)
    manifest: list[dict] = []
    p = 0
    index = 0
    while p + 16 <= len(pack):
        size, fourcc_u32, width, height = struct.unpack_from('<IIII', pack, p)
        if size == 0:
            break
        expected = 16 + width * height * 2
        if width == 0 or height == 0 or size < expected or p + size > len(pack):
            raise RuntimeError(f'Invalid BMPPACK record #{index} at 0x{p:X}')
        # On disk the bytes read e.g. "LLAB" for the engine FourCC "BALL".
        stored = struct.pack('<I', fourcc_u32)
        fourcc = stored[::-1].decode('latin1')
        filename = sanitize_fourcc(fourcc) + '.png'
        pixels = pack[p + 16:p + 16 + width * height * 2]
        write_rgb_png(out_dir / filename, width, height, expand_565(pixels))
        manifest.append({
            'index': index,
            'fourcc': fourcc,
            'file': filename,
            'width': width,
            'height': height,
            'source_format': 'RGB565'
        })
        p += size
        index += 1
    return manifest


def convert_music(music_dir: Path, out_dir: Path) -> list[dict]:
    out_dir.mkdir(parents=True, exist_ok=True)
    tracks = []
    candidates = sorted([p for p in music_dir.iterdir() if p.is_file() and p.suffix.lower() == '.mus' and p.stem != '29'], key=lambda p: p.name.lower())
    for src in candidates:
        pcm = src.read_bytes()
        dst = out_dir / (src.stem + '.wav')
        with wave.open(str(dst), 'wb') as w:
            w.setnchannels(2)
            w.setsampwidth(1)
            w.setframerate(22050)
            w.writeframes(pcm)
        tracks.append({
            'source': src.name,
            'file': dst.name,
            'sample_rate': 22050,
            'channels': 2,
            'bits_per_sample': 8,
            'frames': len(pcm) // 2,
            'seconds': round((len(pcm) // 2) / 22050.0, 6)
        })
    return tracks



def extract_sfx(exe: Path, music_dir: Path, out_dir: Path) -> list[dict]:
    """Split the original MUSIC/29.MUS sample bank using WAVEPACK.

    Reverse engineered behavior (0x409BE0..0x409CAF): WAVEPACK contains
    [reversed FourCC, raw byte length] pairs.  The raw lengths advance the
    cursor in 29.MUS.  The engine shortens LEVL and GAME by 2000 samples for
    playback while still advancing by their full stored lengths.

    The mixer (0x40A5F0) consumes one unsigned 8-bit source byte per output
    frame and mixes it to the 22050-Hz stereo DirectSound stream, therefore
    source effects are mono unsigned 8-bit PCM at 22050 Hz.
    """
    blob = exe.read_bytes()
    off = RSRC_FILE_OFF + (WAVEPACK_RVA - RSRC_RVA)
    table = blob[off:off + WAVEPACK_SIZE]
    if len(table) != WAVEPACK_SIZE:
        raise RuntimeError('WAVEPACK resource is truncated')

    bank_path = next((p for p in music_dir.iterdir() if p.is_file() and p.name.lower() == '29.mus'), None)
    if bank_path is None:
        raise RuntimeError('MUSIC/29.MUS SFX bank was not found')
    bank = bank_path.read_bytes()
    out_dir.mkdir(parents=True, exist_ok=True)

    result: list[dict] = []
    cursor = 0
    for index in range(0, len(table) // 8):
        stored = table[index * 8:index * 8 + 4]
        raw_len = struct.unpack_from('<I', table, index * 8 + 4)[0]
        if stored == b'\x00\x00\x00\x00' or stored == b'\xff\xff\xff\xff':
            break
        fourcc = stored[::-1].decode('latin1')
        if cursor + raw_len > len(bank):
            raise RuntimeError(f'SFX {fourcc}: WAVEPACK extends past MUSIC/29.MUS')
        play_len = raw_len - 2000 if fourcc in ('levl', 'game') else raw_len
        if play_len < 0:
            play_len = 0
        pcm = bank[cursor:cursor + play_len]
        filename = sanitize_fourcc(fourcc) + '.wav'
        with wave.open(str(out_dir / filename), 'wb') as w:
            w.setnchannels(1)
            w.setsampwidth(1)
            w.setframerate(22050)
            w.writeframes(pcm)
        result.append({
            'index': index,
            'fourcc': fourcc,
            'file': filename,
            'sample_rate': 22050,
            'channels': 1,
            'bits_per_sample': 8,
            'raw_bank_offset': cursor,
            'raw_length': raw_len,
            'playback_length': play_len,
            'seconds': round(play_len / 22050.0, 6)
        })
        cursor += raw_len
    return result

def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument('exe', type=Path, help='original AirXonix.wrp.exe')
    ap.add_argument('--music-dir', type=Path, required=True, help='original MUSIC directory')
    ap.add_argument('-o', '--out', type=Path, default=Path('assets'))
    args = ap.parse_args()

    tex = extract_bmppack(args.exe, args.out / 'textures')
    music = convert_music(args.music_dir, args.out / 'music')
    sfx = extract_sfx(args.exe, args.music_dir, args.out / 'sfx')
    manifests = args.out / 'manifests'
    manifests.mkdir(parents=True, exist_ok=True)
    (manifests / 'textures.json').write_text(json.dumps({'format': 1, 'textures': tex}, ensure_ascii=False, indent=2), encoding='utf-8')
    (manifests / 'music.json').write_text(json.dumps({'format': 1, 'tracks': music}, ensure_ascii=False, indent=2), encoding='utf-8')
    (manifests / 'sfx.json').write_text(json.dumps({'format': 1, 'effects': sfx}, ensure_ascii=False, indent=2), encoding='utf-8')
    print(f'Extracted {len(tex)} textures, {len(music)} music tracks and {len(sfx)} SFX to {args.out}')

if __name__ == '__main__':
    main()
