#!/usr/bin/env python3
"""AREA21's second level background (lane PORT-A21-ENDING): from g79's end on
(phase 3 over, the ending script 0x82CBD0 started; card g80) the channel-3
list 001E1E60 builds sends another TEX0 than the play captures g75 .. g78
(export_second_half_common SUBS ('area21', 0) leaves g79 out for that reason).
This runs the same background export (export_area13_level.export_background:
the decomp's export_level --background path, its texels checked against the
capture's GS freeze) over g79's end capture alone and installs the asset as
assets/area21/sub0/background_<TEX0 hex>.embg, the name em_render_frame's
background gate loads when the list's TEX0 is not the loaded asset's.

Usage (native arm64 macOS; the user's disc and ELF as for the area export):
  python3 tools/export_area21_ending_background.py
Output: assets/area21/sub0/background_<TEX0>.embg (disc-derived, ignored).
"""
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import export_second_half_common as SH  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
CAPTURE = 'g79_area21_phase3'


def main():
    C = SH.install('area21', 0)
    import export_area13_level as L13
    import export_area13_common as A13
    v = SH.SUBS[('area21', 0)]
    g20 = Path(v['paths'][0]).parent
    A13.capture_paths = lambda: [g20 / CAPTURE]
    caps = A13.captures('area21')
    el = L13.L.load_export_level()
    out = ROOT / 'build/a21ending/background'
    out.mkdir(parents=True, exist_ok=True)
    info = L13.export_background(el, caps, out)
    if not info:
        sys.exit(f'{CAPTURE}: the background is not armed')
    blob = (out / el.BG_ASSET).read_bytes()
    tex0 = struct.unpack_from('<Q', blob, 16)[0]
    dest = ROOT / 'assets/area21/sub0' / f'background_{tex0:016X}.embg'
    dest.write_bytes(blob)
    (out / el.BG_ASSET).unlink()
    print(f'{CAPTURE}: background TEX0 {tex0:016X}, {len(blob)} bytes -> {dest.relative_to(ROOT)}')
    for line in info.get('log', []):
        print(' ', line)


if __name__ == '__main__':
    main()
