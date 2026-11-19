#!/usr/bin/env python3
"""Original-instruction oracle for 00199770's lock mark as the port binds it
(src/game/em_sight_lock.c; docs/CAMERA_LIVE.md section 7, docs/AIM_FIRE.md
section 7): 001DD170(1, 0x700038B0, 1, 0x80808080, distance), the call
00199770 makes for each class-2 target within 360 and on screen.

The user's pinned ELF supplies every instruction; none are embedded here.
The original 001DD170 runs on one image (HudEE, VU0 macro mode) with its
callees 001DD2F0, 001DD600, 001C5FB0 and 001281C0 executing their original
instructions (nested); 001CBA50 is scripted (its arguments, string and
8-byte style record are compared; its translation has its own oracle).
em_sight_lock runs on a copy through its map / call host: the reticle
translations, the number formatter em_status_draw_001C5FB0 and the vf23
rule (the fog quadword *D_00275670 + 0xA0).

Compared: every 001CBA50 entry in order (stack pointer, a0..t2, the bytes
behind its pointers), and at the end the whole RAM and scratchpad and
001DD170's locals (sp+0x30 quadword, sp+0x48 style record).

Cases: designed memory (render context, clip and view matrices, the target
position, the fog quadword, distances 0..359) and the AREA01 fork snapshot
build/fork_refs/.../a01v_02_door18 (its context, fog and matrices; the
targets are its class-2 entries' positions).
- vf23 = the fog quadword (what the port binds): every byte equal.
- vf23 = another owner's leftover (the value measured in the original in
  33 of 90 ticks, and random quadwords): every byte equal except
  0x7000360C..0x7000360F, the w lane, which is the only place vf23 reaches.
Default ~10 s; EM_TEST_FULL=1 runs every case; --mutations runs the bounded
mutant set of the binding.
"""
import ctypes as C
import os
import random
import struct
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import reference_mode as RM  # noqa: E402
import test_player_slide_reference as S  # noqa: E402
from test_area00_hud_reference import HudEE  # noqa: E402
import test_aim_fire_target_reference as T  # noqa: E402
import reference_root as refs  # noqa: E402

OUT = ROOT / 'build/sight/lock'
CONTEXT, PACKET, POSITION = 0x1D00000, 0x1D10000, 0x700038B0
MASK = 0xFFFFFFFF
# the leftover quadword measured at 001DD170 entries in the original (lane PORT-FAILSTOPS)
LEFTOVER = struct.unpack('<4I', bytes.fromhex('b5d42cc8c20f31c8bac9cc49642fdac2'))
FOG = struct.unpack('<4I', bytes.fromhex('00007f4300000045ffff7e433333a3c0'))
SNAPSHOT = 'levels-recordings-20261018-10/s87/route_a01v/a01v_02_door18'
U32, U64, I, P = C.c_uint32, C.c_uint64, C.c_int, C.POINTER


class Call(C.Structure):
    _fields_ = [('function', U32), ('sp', U32), ('a', U64 * 7), ('f12', U32), ('na', U32), ('nf', U32),
                ('v0', U64)]


MAP = C.CFUNCTYPE(C.c_void_p, C.c_void_p, U32, U32, I)
CALL = C.CFUNCTYPE(I, C.c_void_p, P(Call))


class Lock(C.Structure):
    _fields_ = [('context', C.c_void_p), ('map', MAP), ('call', CALL), ('fault_function', U32),
                ('fault_address', U32)]


ELF = LIB = None
IMAGES = {}


def build(source=None):
    OUT.mkdir(parents=True, exist_ok=True)
    source = source or str(ROOT / 'src/game/em_sight_lock.c')
    lib = OUT / (Path(source).stem + ('.dylib' if sys.platform == 'darwin' else '.so'))
    subprocess.run(['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-Wpedantic', '-ffp-contract=off',
                    '-shared', '-fPIC', '-Isrc', source, 'src/game/em_aim_fire_reticle.c',
                    'src/game/em_status_draw.c', '-o', str(lib)],
                   cwd=ROOT, check=True, capture_output=True)
    dll = C.CDLL(str(lib))
    dll.em_sight_lock_001DD170.argtypes = [P(Lock), U32, U32, U32, U32, U32, U32]
    dll.em_sight_lock_001DD170.restype = C.c_int
    return dll


def text_at(e, a):
    out = b''
    while len(out) < 32:
        c = e.read(a + len(out), 1)
        out += c
        if c == b'\0':
            break
    return out


def label_event(e, sp, a):
    return ('call', 0x1CBA50, sp & MASK, tuple(x & MASK for x in a), text_at(e, a[5]), e.read(a[6], 8))


def snapshot_image():
    folder = refs.capture(SNAPSHOT)
    if not (folder / 'eeMemory.bin').exists():
        return None
    return (folder / 'eeMemory.bin').read_bytes(), (folder / 'scratchpad.bin').read_bytes()


def snapshot_targets(ram):
    """The positions of the snapshot's class-2 entries (00183C40's model rule)."""
    cursor, = struct.unpack_from('<I', ram, 0x275B8C)
    count, = struct.unpack_from('<h', ram, 0x275B94)
    out = []
    for i in range(max(0, min(count, 32))):
        e, = struct.unpack_from('<I', ram, cursor + 4 * i)
        off = {0: 0x118, 1: 0x11C, 2: 0x11C, 3: 0x118, 4: 0x118, 5: 0x130}.get(ram[e + 3])
        if off is None:
            continue
        p, = struct.unpack_from('<I', ram, e + off)
        out.append(struct.unpack_from('<3I', ram, p + 0xC0) + (S.bits(1.0),))
    return out


def prepare(case):
    image = IMAGES.get(case.get('capture'))
    e = HudEE(ELF, *(image or (None, None)))
    if image:
        e.load_elf(ELF)
    rng = random.Random(case['seed'])
    if not image:
        e.save(0x275670, CONTEXT)
        for i in range(4):
            e.save(CONTEXT + 0x10 + 4 * i, PACKET + 0x1000 * i)
        e.write(PACKET, rng.randbytes(0x4000))
        clip = case['clip']
        view = case['view']
        for row in range(4):
            for col in range(4):
                e.save(CONTEXT + 0x2240 + 16 * row + 4 * col, S.bits(clip[row][col]))
                e.save(0x70003AC0 + 16 * row + 4 * col, S.bits(view[row][col]))
        e.write(CONTEXT + 0xA0, struct.pack('<4I', *case['fog']))
    e.write(0x70003600, rng.randbytes(0x40))
    e.write(0x8111D0, rng.randbytes(16))
    e.write(POSITION, struct.pack('<4I', *case['point']))
    e.write(S.STACK_TOP - 0x400, rng.randbytes(0x400))
    for i in range(1, 32):
        e.vf[i] = [S.bits(rng.uniform(-2, 2)) for _ in range(4)]
    ctx = e.load(0x275670)
    fog = list(struct.unpack('<4I', e.read(ctx + 0xA0, 16)))
    e.vf[23] = list(case['vf23']) if case.get('vf23') else fog
    e.vacc = [S.bits(rng.uniform(-1, 1)) for _ in range(4)]
    e.q = S.bits(.75)
    e.clip = rng.getrandbits(24)
    return e


def locals_of(e):
    frame = S.STACK_TOP - 0x50
    return e.read(frame + 0x30, 16) + e.read(frame + 0x48, 6)


def run_case(case):
    e = prepare(case)
    n = HudEE(ELF, bytearray(e.mem), bytearray(e.spad))
    n.stack[:] = e.stack
    oevents, nevents, errors = [], [], []

    def label(x):
        a = [int(v) for v in x.r[4:11]]
        oevents.append(label_event(x, x.r[29], a))
        x.r[2] = 0
    e.hooks = {0x1CBA50: label}
    args = [1, POSITION, 1, 0x80808080, case['value']]
    e.call(0x1DD170, args)
    arrays = [(C.c_uint8 * len(b)).from_buffer(b) for b in (n.mem, n.spad, n.stack)]

    def mp(_, a, size, w):
        if a < 0x2000000:
            b, i = 0, a
        elif 0x70000000 <= a < 0x70004000:
            b, i = 1, a - 0x70000000
        elif 0x7F000000 <= a < 0x7F100000:
            b, i = 2, a - 0x7F000000
        else:
            return None
        return C.addressof(arrays[b]) + i if i + size <= len(arrays[b]) else None

    def cb(_, p):
        c = p.contents
        try:
            if c.function == 0x1CBA50:
                assert (c.na, c.nf) == (7, 0)
                nevents.append(label_event(n, c.sp, list(c.a[:7])))
                c.v0 = 0
                return 0
            if c.function == 0x1281C0:
                assert (c.na, c.nf) == (0, 1)
                n.r[29] = c.sp
                n.f[12] = c.f12
                T.nested(n, 0x1281C0)
                c.v0 = n.r[2] & 0xFFFFFFFFFFFFFFFF
                return 0
            raise AssertionError(('unexpected callee', hex(c.function)))
        except Exception as ex:  # noqa: BLE001  (reported after the call)
            errors.append(ex)
            return -1
    callbacks = MAP(mp), CALL(cb)
    h = Lock(None, *callbacks, 0, 0)
    status = LIB.em_sight_lock_001DD170(C.byref(h), S.STACK_TOP, *args)
    if errors:
        raise errors[0]
    assert status == 0 and h.fault_function == 0, (case['name'], 'native fault', hex(h.fault_function),
                                                  hex(h.fault_address))
    assert nevents == oevents, (case['name'], 'labels', nevents, oevents)
    assert locals_of(n) == locals_of(e), (case['name'], 'locals')
    diff = [i for i in range(len(e.spad)) if n.spad[i] != e.spad[i]]
    if case.get('vf23'):
        assert set(diff) <= set(range(0x360C, 0x3610)), (case['name'], 'scratch outside the w lane',
                                                         [hex(0x70000000 + i) for i in diff[:8]])
    else:
        assert not diff, (case['name'], 'scratch', [hex(0x70000000 + i) for i in diff[:8]])
    assert n.mem == e.mem, (case['name'], 'RAM', next(i for i in range(len(e.mem)) if n.mem[i] != e.mem[i]))
    return len(oevents), bool(diff)


IDENTITY = ((1., 0., 0., 0.), (0., 1., 0., 0.), (0., 0., 1., 0.), (0., 0., 0., 1.))


def cases():
    out = []
    rng = random.Random(97112)
    views = [((100., 0., 0., 0.), (0., 100., 0., 0.), (0., 0., 50., 0.), (2048., 2048., 100., 1.)),
             ((60., 0., 0., 0.), (0., -90., 0., 0.), (0., 0., 1., 1.), (2048., 2048., 0., 3.))]
    fogs = [FOG, tuple(S.bits(v) for v in (255., 0., 128., -2.)), tuple(S.bits(v) for v in (255., 0., -1., -1.))]
    for i in range(160):
        pt = tuple(S.bits(rng.uniform(-1.2, 1.2)) for _ in range(3)) + (S.bits(1.0),)
        base = {'seed': i, 'clip': IDENTITY, 'view': rng.choice(views), 'point': pt,
                'fog': rng.choice(fogs), 'value': rng.choice((0, 1, 9, 10, 99, 100, 359, rng.randrange(360)))}
        out.append(dict(base, name=f'fog {i}'))
        vf = LEFTOVER if i % 2 else tuple(S.bits(rng.uniform(-3e5, 3e5)) for _ in range(4))
        out.append(dict(base, name=f'leftover {i}', vf23=vf))
    image = snapshot_image()
    if image:
        IMAGES[SNAPSHOT] = image
        for k, pt in enumerate(snapshot_targets(image[0])):
            for vf, tag in ((None, 'fog'), (LEFTOVER, 'leftover')):
                out.append({'name': f'a01v_02 {tag} {k}', 'capture': SNAPSHOT, 'seed': 500 + k, 'point': pt,
                            'value': 17 * k + 3, 'vf23': vf})
    return out


def main():
    global ELF, LIB
    start = time.monotonic()
    ELF = S.read_elf()
    LIB = build()
    every = cases()
    selected = RM.select(every, 120, 97112, axes=(lambda c: c['name'].split()[0],))
    RM.banner(RM.part(len(selected), len(every), 'lock-mark cases'))
    rows = RM.parallel_map(run_case, selected)
    drawn = sum(1 for r in rows if r[0])
    lanes = sum(1 for r in rows if r[1])
    assert drawn > len(selected) // 4, ('too few cases reach the label', drawn)
    assert any(c.get('capture') for c in selected) or not IMAGES, 'no snapshot case selected'
    print(f'PASS: {len(selected)} cases, {drawn} drew the mark and label, {lanes} leftover cases differed only in '
          f'0x7000360C; {time.monotonic() - start:.2f}s')


def mutations():
    global ELF, LIB
    ELF = S.read_elf()
    source = (ROOT / 'src/game/em_sight_lock.c').read_text()
    changes = [('fog offset', 'ctx + 0xA0u, 16', 'ctx + 0xB0u, 16'),
               ('number base', 'UINT32_C(0x008111D0)', 'UINT32_C(0x008111E0)'),
               ('number terminator', 'strlen(text) + 1u', 'strlen(text)'),
               ('builder swap', "0x001DD2F0u ? em_aim_fire_reticle_001DD2F0", "0x001DD600u ? em_aim_fire_reticle_001DD2F0")]
    folder = OUT / 'mutations'
    folder.mkdir(parents=True, exist_ok=True)
    pool = [c for c in cases() if not c.get('vf23')][:60]
    survivors = []
    for i, (name, old, new) in enumerate(changes):
        assert old in source, (name, 'missing edit')
        path = folder / f'mutant_{i:02d}.c'
        path.write_text(source.replace(old, new, 1))
        LIB = build(str(path))
        killed = None
        for case in pool:
            try:
                run_case(case)
            except AssertionError:
                killed = case['name']
                break
        if killed is None:
            survivors.append(name)
        print(f'{i + 1:02d} {name}: ' + ('SURVIVED' if killed is None else f'killed by {killed}'), flush=True)
    assert not survivors, survivors
    print(f'PASS: {len(changes)} mutants killed')


if __name__ == '__main__':
    mutations() if '--mutations' in sys.argv else main()
