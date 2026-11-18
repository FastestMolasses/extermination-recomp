#!/usr/bin/env python3
"""Export the local AREA11 opening program and its actor spawn list.

Two images of the user's AREA11 overlay, each a placement list followed by
the script whose op14 reads it:

  assets/scene_snow/opening.emsc      0x828F30..0x8292C0, entry 0x828FC0
      (the New Game opening, the controller 00823E80's)
  assets/scene_snow/cable_scene.emsc  0x828BE0..0x828F30, entry 0x828C70
      (the scene after AREA17's cable ride: the flag-0x30 manager 00823CE0
      starts it when D_00810788 is set; its op14 reads the list 0x828BE0)

Output contains user-supplied disc data and must remain under ignored
assets/. The 64-byte script records retain their original addresses and
flags so the native interpreter can follow the original command stream.
"""
from __future__ import annotations
import argparse
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[1]
BASE = 0x00828F30
ENTRY = 0x00828FC0
END = 0x008292C0
CABLE_BASE = 0x00828BE0
CABLE_ENTRY = 0x00828C70
CABLE_END = 0x00828F30


def export_image(overlay: bytes, base: int, entry: int, end: int) -> bytes:
    """One image: the list at `base`, the script `entry` .. `end` (its stop
    record last). The list's terminator (halfword 0xFFFF) lies before the
    script."""
    if len(overlay) != 0x7800 or overlay[:4] != b"MWo3":
        raise ValueError("Expected SCUS-97112 AREA11 overlay")
    arena = struct.unpack_from("<I", overlay, 8)[0]
    if arena != 0x00823500:
        raise ValueError("Unexpected AREA11 arena")
    payload = overlay[base-arena:end-arena]
    first = struct.unpack_from("<I", payload, entry-base)[0]
    last = struct.unpack_from("<I", payload, end-base-64)[0]
    if first != 7 or last != 0x80000007:
        raise ValueError(f"script {entry:#x}: bounds do not match the original program")
    for at in range(0, entry-base, 0x2C):   # EM_SDF_ENTRY_SIZE
        if struct.unpack_from("<H", payload, at)[0] == 0xFFFF:
            break
    else:
        raise ValueError(f"list {base:#x}: no terminator before the script")
    return struct.pack("<4sIIII", b"EMSC", 1, base, entry, len(payload)) + payload


def export_opening(overlay: bytes) -> bytes:
    if len(overlay) != 0x7800 or overlay[:4] != b"MWo3":
        raise ValueError("Expected SCUS-97112 AREA11 overlay")
    arena = struct.unpack_from("<I", overlay, 8)[0]
    if arena != 0x00823500:
        raise ValueError("Unexpected AREA11 arena")
    payload = overlay[BASE-arena:END-arena]
    if len(payload) != END-BASE:
        raise ValueError("Truncated opening program")
    first = struct.unpack_from("<I",payload,ENTRY-BASE)[0]
    last = struct.unpack_from("<I",payload,END-BASE-64)[0]
    if first != 7 or last != 0x80000007:
        raise ValueError("Opening script bounds do not match the original program")
    return struct.pack("<4sIIII",b"EMSC",1,BASE,ENTRY,len(payload)) + payload


def main() -> None:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--overlay",type=Path,
                        default=ROOT.parent/"Extermination/extract/OVERLAY/AREA11.BIN")
    parser.add_argument("--out",type=Path,
                        default=ROOT/"assets/scene_snow/opening.emsc")
    parser.add_argument("--cable-out",type=Path,
                        default=ROOT/"assets/scene_snow/cable_scene.emsc")
    args=parser.parse_args()
    overlay=args.overlay.read_bytes()
    blob=export_opening(overlay)
    if blob != export_image(overlay, BASE, ENTRY, END):
        raise ValueError("the two opening exports differ")
    cable=export_image(overlay, CABLE_BASE, CABLE_ENTRY, CABLE_END)
    for path, data in ((args.out, blob), (args.cable_out, cable)):
        if path.is_symlink():
            path.unlink()   # a worktree link: write this tree's own copy
        path.parent.mkdir(parents=True,exist_ok=True)
        path.write_bytes(data)
    print(f"AREA11: exported the opening ({args.out}) and the cable ride's scene ({args.cable_out})")


if __name__=="__main__": main()
