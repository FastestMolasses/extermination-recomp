#!/usr/bin/env python3
"""Original-instruction oracle for the weapon sights (src/game/em_sight.c,
docs/CAMERA_LIVE.md section 7): the sixteen functions the aim camera's
sight switch 001999C0 reaches (001DB830, 001DB9D0, 001DBCB0, 001DBD50,
001DBE20, 001DBF00, 001DC020, 001DC610, 001DC890, 001DC960, 001DCD40,
001DCF40, 001DCFF0, 00199770 and the packet builders 002082B0, 002081A0).

The user's pinned ELF supplies every instruction; none are embedded here.
Each case designs memory at original addresses over the ELF's image: the
render context's slot cursors (*D_00275670 + 0x10 + 4 slot) into a packet
buffer of random bytes, the scratchpad 0x70003600..0x7000363F and
0x700038A0.., the projection rows 0x70003AC0, the caller's stack (random
bytes), the ELF's .data templates (or random ones), and for 00199770 the
player, its gun, the class-2 list D_00275B90 / D_00275B98 and its entries.
The original runs on one image (test_player_fall_reference.FallEE), the
native module on a copy through its map / call / store host.

Callees: the SDK leaves (float_to_int 001281C0, sin 0011E2A8, cos
0011DE90, sqrt 0011E748, fabs 0011DF78, 00102948, 00102850, 001028B8,
001028D0, 001026A0) execute their ORIGINAL instructions on both sides
(nested, identically). 001D1F80, 001D64A0, 00207F80, 001CBA50 and 001DD170
are scripted (the three builders advance the slot cursor by their packet
size), 00183B80 returns and 00183C40 writes what the case chooses; scripted
callees are not claimed verified here (their translations have their own
oracles). Compared: every callee entry in order (function, stack pointer,
argument registers, f12, and the bytes behind each pointer argument at the
call: 001D64A0's two 3-word groups and colour quadword, 001CBA50's string and
8-byte style record, the SDK leaves' sources, 001DD170's vector), and at the
end the whole RAM and scratchpad and the stack below the caller (every byte
but the original's register-save slots). Chain cases run each arm of
001999C0 (the drawers in order at one stack pointer, $ra's upper doubleword
0 as on the EE), so a drawer's read of bytes an earlier one left on the
stack is compared. Every instruction word of the sixteen executes except
the ones no input reaches (named below), and every conditional branch goes
both ways. Fail-stop cases: no map, no call, an unmapped range, each callee
failing in turn, a latched fault, a NULL call that is never reached.

Default run ~10 s (mode quick); EM_TEST_FULL=1 runs the wider sweep.
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
from test_player_fall_reference import FallEE  # noqa: E402

OUT = ROOT / 'build/sight'
MASK = 0xFFFFFFFF
FUNCS = {0x1DB830: 0x194, 0x1DB9D0: 0x2D4, 0x1DBCB0: 0x94, 0x1DBD50: 0xD0, 0x1DBE20: 0xD4, 0x1DBF00: 0x118,
         0x1DC020: 0x5E8, 0x1DC610: 0x27C, 0x1DC890: 0xCC, 0x1DC960: 0x3E0, 0x1DCD40: 0x1F8, 0x1DCF40: 0xA8,
         0x1DCFF0: 0x178, 0x199770: 0x250, 0x2082B0: 0x49C, 0x2081A0: 0x110}
# Words no input executes: 001DCFF0's unsigned-to-float fallbacks (each
# converts a byte masked to 0..255, never negative, so the sign branch before
# it is never taken) and the word after 002082B0's unconditional branch.
DEAD = set(range(0x1DD048, 0x1DD064, 4)) | set(range(0x1DD084, 0x1DD0A0, 4)) | \
    set(range(0x1DD0BC, 0x1DD0D8, 4)) | set(range(0x1DD0F4, 0x1DD110, 4)) | {0x2086E8}
NEVER_TAKEN = {0x1DD034, 0x1DD070, 0x1DD0A8, 0x1DD0E0}
# Every other jal target: (integer argument registers, f12 used).
SIG = {0x1281C0: (0, 1), 0x11E2A8: (0, 1), 0x11DE90: (0, 1), 0x11E748: (0, 1), 0x11DF78: (0, 1),
       0x102948: (2, 0), 0x102850: (2, 1), 0x1028B8: (3, 0), 0x1028D0: (3, 0), 0x1026A0: (3, 0),
       0x1D1F80: (3, 0), 0x1D64A0: (5, 0), 0x207F80: (6, 0), 0x1CBA50: (7, 0), 0x183B80: (1, 0),
       0x183C40: (2, 0), 0x1DD170: (5, 0)}
REAL = {0x1281C0, 0x11E2A8, 0x11DE90, 0x11E748, 0x11DF78, 0x102948, 0x102850, 0x1028B8, 0x1028D0, 0x1026A0}
SWEEPS = (360.0, 360.0, 100.0, 7.5, 0.0, -40.0, 1e12, -1e12)
ADVANCE = {0x1D64A0: 0x60, 0x207F80: 0x50, 0x1CBA50: 0x80}
CTX = 0x811CC0
PKT = 0x01E00000                 # four slot buffers of 0x8000
PLAYER, GUN, LIST, ACTORS, RING, BUF = 0x8102B0, 0x01D00000, 0x01D10000, 0x01D20000, 0x01D30000, 0x01D31000
SP = S.STACK_TOP
SAVED_GPR = set(range(16, 24)) | {30, 31}
CODE = set()
for _f, _n in FUNCS.items():
    CODE.update(range(_f, _f + _n, 4))

U32, U64, I, P = C.c_uint32, C.c_uint64, C.c_int, C.POINTER


class Call(C.Structure):
    _fields_ = [('function', U32), ('sp', U32), ('a', U64 * 7), ('f', U32 * 8), ('na', U32), ('nf', U32),
                ('v0', U64), ('f0', U32)]


MAP = C.CFUNCTYPE(C.c_void_p, C.c_void_p, U32, U32, I)
CALL = C.CFUNCTYPE(I, C.c_void_p, P(Call))
STORE = C.CFUNCTYPE(None, C.c_void_p, U32, U32)


class Host(C.Structure):
    _fields_ = [('context', C.c_void_p), ('map', MAP), ('call', CALL), ('store', STORE), ('sp', U32),
                ('fault', I), ('fault_function', U32), ('fault_address', U32)]


# entry -> (native name, argument kinds: s slot / u word / f float bits / i int)
ENTRIES = {0x1DB830: ('001DB830', 'su'), 0x1DB9D0: ('001DB9D0', 'sf'), 0x1DBCB0: ('001DBCB0', 'sf'),
           0x1DBD50: ('001DBD50', 's'), 0x1DBE20: ('001DBE20', 's'), 0x1DBF00: ('001DBF00', 's'),
           0x1DC020: ('001DC020', 'sf'), 0x1DC610: ('001DC610', 's'), 0x1DC890: ('001DC890', 'sf'),
           0x1DC960: ('001DC960', 's'), 0x1DCD40: ('001DCD40', 'su'), 0x1DCF40: ('001DCF40', 'su'),
           0x1DCFF0: ('001DCFF0', 'su'), 0x199770: ('00199770', 'u'), 0x2082B0: ('002082B0', 'su'),
           0x2081A0: ('002081A0', 'suiiu')}
ELF = LIB = None


def build():
    OUT.mkdir(parents=True, exist_ok=True)
    source = os.environ.get('EM_SIGHT_SOURCE', str(ROOT / 'src/game/em_sight.c'))
    lib = OUT / (Path(source).stem + ('.dylib' if sys.platform == 'darwin' else '.so'))
    subprocess.run(['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-Wpedantic', '-ffp-contract=off',
                    '-shared', '-fPIC', '-Isrc', source, '-o', str(lib)], cwd=ROOT, check=True,
                   capture_output=True)
    dll = C.CDLL(str(lib))
    for entry, (name, kinds) in ENTRIES.items():
        f = getattr(dll, 'em_sight_' + name)
        f.argtypes = [P(Host)] + [C.c_int32 if k in 'si' else U32 for k in kinds]
        f.restype = C.c_int
    dll.em_sight_call.argtypes = [P(Host), P(Call)]
    dll.em_sight_call.restype = C.c_int
    return dll


class Oracle(FallEE):
    """FallEE that records the instruction words of the sixteen it fetches,
    the stack bytes register saves store, and branch outcomes."""

    def __init__(self, ram=None, spad=None):
        super().__init__(ELF, ram, spad)
        self.seen = set()
        self.saves = set()
        self.branches = set()
        self.word = 0

    def load(self, address, size=4):
        if address in CODE:
            self.seen.add(address)
        return super().load(address, size)

    def execute(self, word, pc):
        self.word = word
        super().execute(word, pc)

    def save(self, address, value, size=4):
        a = address & MASK
        if 0x7F000000 <= a < 0x7F100000:
            w = self.word
            op, rs, rt = w >> 26, w >> 21 & 31, w >> 16 & 31
            if rs == 29 and ((op in (0x1F, 0x3F) and rt in SAVED_GPR) or (op == 0x39 and rt >= 20)):
                self.saves.update(range(a, a + size))
        super().save(address, value, size)

    def branch(self, w, pc):
        b = super().branch(w, pc)
        if b is not None and pc in CODE:
            self.branches.add((pc, b[0]))
        return b


def nested(e, fn):
    saved = (list(e.r), list(e.rh), list(e.f), e.hi, e.lo, e.acc, e.cond, [list(v) for v in e.vf],
             list(e.vacc), e.q, e.word)
    hooks, e.hooks = e.hooks, {}
    e.r[29] = (e.r[29] - 0x400) & ~15
    e.r[31] = S.RETURN
    e.run(fn)
    v, f = e.r[2], e.f[0]
    e.r, e.rh, e.f, e.hi, e.lo, e.acc, e.cond, e.vf, e.vacc, e.q, e.word = saved
    e.hooks = hooks
    e.r[2] = v
    e.f[0] = f


def string_at(e, a):
    out = b''
    while len(out) < 48:
        c = e.read(a + len(out), 1)
        out += c
        if c == b'\0':
            break
    return out


def pointers(e, fn, a):
    """The bytes behind a callee's pointer arguments at its entry."""
    if fn == 0x1D64A0:
        return (e.read(a[1], 12), e.read(a[2], 12), e.read(a[3], 16))
    if fn == 0x1CBA50:
        return (string_at(e, a[5]), e.read(a[6], 8))
    if fn == 0x1DD170:
        return (e.read(a[1], 16),)
    if fn in (0x102948, 0x102850):
        return (e.read(a[1] & ~15, 16),)
    if fn in (0x1028B8, 0x1028D0):
        return (e.read(a[1] & ~15, 16), e.read(a[2] & ~15, 16))
    if fn == 0x1026A0:
        return (e.read(a[1] & ~15, 64), e.read(a[2] & ~15, 16))
    return ()


def policy(e, fn, case):
    """What a callee does: REAL ones run their original instructions, the
    others follow the case's script (identically on both sides)."""
    a = [int(e.r[4 + i]) & MASK for i in range(7)]
    if fn in REAL:
        nested(e, fn)
        return
    e.r[2] = 0
    e.f[0] = 0
    if fn in ADVANCE:
        cell = e.load(0x275670) + ((a[0] << 2) & MASK) + 0x10
        e.save(cell, (e.load(cell) + ADVANCE[fn]) & MASK)
    elif fn == 0x183B80:
        e.r[2] = S.sx32(case['accept'].get(a[0], 0)) & 0xFFFFFFFFFFFFFFFF
    elif fn == 0x183C40:
        e.write(a[1], struct.pack('<4I', *case['where'].get(a[0], (0, 0, 0, 0))))


def F(x):
    return S.bits(x)


def design(case):
    """The case's memory image (an Oracle over the ELF plus designed bytes)."""
    rng = random.Random(case['seed'])
    e = Oracle()
    e.write(0x275670, struct.pack('<I', CTX))
    for k in range(4):
        e.write(CTX + 0x10 + 4 * k, struct.pack('<I', PKT + 0x8000 * k + 16 * rng.randrange(0, 8)))
    e.write(PKT, rng.randbytes(0x20000))
    e.write(0x70003600, struct.pack('<16I', *(F(rng.uniform(-300, 300)) for _ in range(16))))
    e.write(0x700038A0, rng.randbytes(0x60))
    e.write(SP - 0x1000, rng.randbytes(0x1000))
    if case.get('data') == 'random':
        e.write(0x253240, struct.pack('<4I', *(rng.getrandbits(32) for _ in range(4))))
        e.write(0x253250, struct.pack('<4f', rng.uniform(-300, 300), rng.uniform(-300, 300), rng.uniform(-5, 5),
                                      rng.uniform(-900, 900)))
        e.write(0x253260, rng.randbytes(0x20))
        e.write(0x253280, rng.randbytes(0x150))
    # the projection rows and the player for 00199770
    s = rng.choice((1.0, 1.0, 0.5, 4.0))
    px, py, pz = float(rng.randint(-500, 500)), float(rng.randint(-100, 100)), float(rng.randint(-500, 500))
    wz = rng.choice((0.0, 0.0, 0.0, -0.05, 0.05))
    rows = [(s, 0, 0, 0), (0, s, 0, 0), (0, 0, 1, wz), (2048 - s * px, 2048 - s * py, 0, 1 - wz * pz)]
    e.write(0x70003AC0, b''.join(struct.pack('<4f', *row) for row in rows))
    e.write(PLAYER + 0x20, struct.pack('<I', GUN))
    e.write(PLAYER + 0xA0, struct.pack('<3f', px, py, pz))
    e.write(GUN + 0xA0, struct.pack('<4I', *(rng.getrandbits(32) for _ in range(4))))
    count = case.get('count', rng.choice((0, 1, 3, 6)))
    e.write(0x275B98, struct.pack('<h', count))
    e.write(0x275B90, struct.pack('<I', LIST))
    accept, where = {}, {}
    for i in range(max(count, 0)):
        actor = ACTORS + 0x400 * i
        e.write(LIST + 4 * i, struct.pack('<I', actor))
        e.write(actor, bytes([rng.choice((0, 1, 1, 1, 0x40))]))
        e.write(actor + 0x34, struct.pack('<h', rng.choice((0, 1, 1, -3))))
        accept[actor] = rng.choice((0, 1, 1, 1, 2, -1))
        d = rng.choice((10.0, 100.0, 170.0, 359.0, 361.0, 2000.0))
        ox, oy = rng.uniform(-1, 1), rng.uniform(-1, 1)
        where[actor] = (F(px + ox * d / s), F(py + oy * d / s), F(pz + rng.uniform(-60, 60)), rng.getrandbits(32))
        if rng.random() < 0.25:          # exactly 360 away (the < 360 bound)
            where[actor] = (F(px + rng.choice((360.0, -360.0))), F(py), F(pz), F(1.0))
    case['accept'], case['where'] = accept, where
    # 002082B0's ring record and 002081A0's buffer for direct calls
    a0 = rng.choice((0.0, 0.0, 45.0, -30.0, 350.0))
    sweep = case.get('sweep', rng.choice(SWEEPS))
    e.write(RING, struct.pack('<8f', rng.choice((32768.0, 2048.0)), rng.choice((32768.0, 1000.0)), a0, a0 + sweep,
                              *(rng.uniform(0, 300) for _ in range(4))))
    e.write(RING + 0x20, struct.pack('<16f', *(rng.choice((rng.uniform(0, 255), 128.0, -3.0, 300.0))
                                               for _ in range(16))))
    e.write(BUF, rng.randbytes(0x100))
    return e


def args_of(rng, fn):
    slot = rng.choice((1, 1, 1, 1, 0, 2, 3))
    f = F(rng.choice((1.0, 1.0, 0.0, 0.5, 0.99, 0.01, -0.0, 2.0, -1.0, 1e6, rng.uniform(-3, 3), rng.uniform(0, 1))))
    if fn == 0x1DB830:
        return [slot, rng.choice((0xFF, 0xFF, 0, rng.getrandbits(32)))]
    if fn in (0x1DB9D0, 0x1DBCB0, 0x1DC020, 0x1DC890):
        return [slot, f]
    if fn in (0x1DCD40, 0x1DCF40, 0x1DCFF0):
        return [slot, rng.choice((0x80B9E1A5, 0x801996FF, rng.getrandbits(32)))]
    if fn == 0x199770:
        return [PLAYER]
    if fn == 0x2082B0:
        return [slot, RING]
    if fn == 0x2081A0:
        return [slot, BUF, rng.choice((2, 2, 0, -1, 0x7FFF)), rng.choice((4, 4, 0, 1, 2, 7, -1, -6)),
                rng.getrandbits(32)]
    return [slot]


def to_native(kinds, args):
    return [S.sx32(v) if k in 'si' else v & MASK for k, v in zip(kinds, args)]


def first_diff(a, b):
    for i, (x, y) in enumerate(zip(a, b)):
        if x != y:
            return i, x, y
    return min(len(a), len(b)), a[len(b):len(b) + 1], b[len(a):len(a) + 1]


def run_case(case):
    e = design(case)
    n = Oracle(e.mem, e.spad)
    n.stack[:] = e.stack
    oevents, nevents, errors, calls = [], [], [], [0]

    def hook(fn):
        def inner(x):
            na, nf = SIG[fn]
            a = [int(v) & MASK for v in x.r[4:11]]
            oevents.append(('call', fn, x.r[29] & MASK, tuple(a[:na]), tuple(x.f[12] & MASK for _ in range(nf)),
                            pointers(x, fn, a)))
            policy(x, fn, case)
        return inner
    e.hooks = {fn: hook(fn) for fn in SIG}
    steps = case['steps']
    for fn, args in steps:
        e.r[29] = SP
        e.r[31] = S.RETURN
        for i, v in enumerate(args):
            if ENTRIES[fn][1][i] == 'f':
                e.f[12] = v & MASK
            else:
                e.r[4 + i] = S.sx32(v & MASK) & 0xFFFFFFFFFFFFFFFF
        if fn == 0x1DB830:
            e.f[12] = F(1.0)
        e.run(fn)
    arrays = [(C.c_uint8 * len(buf)).from_buffer(buf) for buf in (n.mem, n.spad, n.stack)]

    def mapping(_, a, size, write):
        try:
            if case.get('unmapped') is not None and a <= case['unmapped'] < a + size:
                return None
            if a < 0x2000000:
                b, i = 0, a
            elif 0x70000000 <= a < 0x70004000:
                b, i = 1, a - 0x70000000
            elif 0x7F000000 <= a < 0x7F100000:
                b, i = 2, a - 0x7F000000
            else:
                return None
            if i + size > len(arrays[b]):
                return None
            return C.addressof(arrays[b]) + i
        except Exception as ex:          # noqa: BLE001
            errors.append(ex)
            return None

    def observe(_, a, size):
        pass

    def worker(_, p):
        c = p.contents
        try:
            fn = c.function
            na, nf = SIG[fn]
            assert (c.na, c.nf) == (na, nf), (hex(fn), 'signature', c.na, c.nf)
            a = [int(v) & MASK for v in c.a[:7]]
            assert all(int(v) == 0 for v in c.a[na:7]), (hex(fn), 'unused argument registers set')
            nevents.append(('call', fn, c.sp, tuple(a[:na]), tuple(int(c.f[0]) for _ in range(nf)),
                            pointers(n, fn, a)))
            k = len(nevents)
            assert nevents == oevents[:k], (case['name'], 'callee entries', first_diff(nevents, oevents))
            calls[0] += 1
            if calls[0] == case.get('failat'):
                return -1
            n.r[29] = c.sp
            for i in range(7):
                n.r[4 + i] = S.sx32(a[i]) & 0xFFFFFFFFFFFFFFFF
            if nf:
                n.f[12] = c.f[0]
            policy(n, fn, case)
            c.v0 = n.r[2] & 0xFFFFFFFFFFFFFFFF
            c.f0 = n.f[0] & MASK
            return 0
        except Exception as ex:          # noqa: BLE001
            errors.append(ex)
            return -1
    callbacks = (MAP(mapping), CALL(worker), STORE(observe))
    h = Host(None, *callbacks, SP, 0, 0, 0)
    status = 0
    for fn, args in steps:
        name, kinds = ENTRIES[fn]
        h.sp = SP
        status = getattr(LIB, 'em_sight_' + name)(C.byref(h), *to_native(kinds, args))
        if status:
            break
    if errors:
        raise errors[0]
    if 'failat' in case or case.get('unmapped') is not None:
        code = 3 if 'failat' in case else 2
        assert status == -1 and h.fault == code and h.fault_function == steps[0][0], \
            (case['name'], 'failure latch', status, h.fault, hex(h.fault_function))
        assert nevents == oevents[:len(nevents)], (case['name'], 'failure prefix')
        before = list(nevents)
        name, kinds = ENTRIES[steps[0][0]]
        assert getattr(LIB, 'em_sight_' + name)(C.byref(h), *to_native(kinds, steps[0][1])) == -1
        assert nevents == before, (case['name'], 'latched call had effects')
        return set(), set(), len(nevents), calls[0]
    assert status == 0 and h.fault == 0, (case['name'], status, h.fault, hex(h.fault_function),
                                          hex(h.fault_address))
    assert nevents == oevents, (case['name'], 'callee entries', first_diff(nevents, oevents), len(nevents),
                                len(oevents))
    if n.mem != e.mem or n.spad != e.spad:
        where = [hex(i) for i in range(len(e.mem)) if n.mem[i] != e.mem[i]][:8]
        where += [hex(0x70000000 + i) for i in range(len(e.spad)) if n.spad[i] != e.spad[i]][:8]
        raise AssertionError((case['name'], 'memory differs', where))
    saves = e.saves | n.saves
    lo = SP - 0x1000 - 0x7F000000
    for i in range(lo, SP - 0x7F000000):
        if e.stack[i] != n.stack[i] and 0x7F000000 + i not in saves:
            raise AssertionError((case['name'], 'stack local differs', hex(0x7F000000 + i)))
    return e.seen, e.branches, len(oevents), sum(1 for x in oevents if x[0] == 'call')


# ======================================================================
# Cases
# ======================================================================

ARMS = {  # 001999C0's arms (a1 = 1): the drawers in order at one sp
    'delta': ((0x1DB9D0, 'one'), (0x1DBF00, None), (0x1DC020, 'zoom')),
    'interceptor': ((0x1DB9D0, 'one'), (0x1DC610, None), (0x1DC890, 'zoom')),
    'missile': ((0x1DB830, 0xFF), (0x1DBE20, None), (0x199770, None)),
    'scope': ((0x1DB9D0, 'one'), (0x1DC960, None), (0x1DBCB0, 'zoom')),
    'night': ((0x1DB830, 0xFF), (0x1DBD50, None)),
}


def cases():
    rng = random.Random(0x5167)
    out = []
    for k in range(RM.pick(400, 400)):
        fn = rng.choice(list(FUNCS))
        out.append({'name': f'{fn:06X}/{k}', 'seed': rng.getrandbits(32), 'steps': [(fn, args_of(rng, fn))],
                    'data': rng.choice(('elf', 'elf', 'random'))})
    for arm, steps in ARMS.items():
        for k in range(RM.pick(24, 24)):
            zoom = F(rng.choice((0.0, 0.25, 0.5, 1.0, 0.99, rng.random())))
            seq = []
            for fn, arg in steps:
                if fn == 0x199770:
                    seq.append((fn, [PLAYER]))
                elif arg == 'one':
                    seq.append((fn, [1, F(1.0)]))
                elif arg == 'zoom':
                    seq.append((fn, [1, zoom]))
                elif arg is None:
                    seq.append((fn, [1]))
                else:
                    seq.append((fn, [1, arg]))
            out.append({'name': f'arm {arm}/{k}', 'seed': rng.getrandbits(32), 'steps': seq, 'data': 'elf',
                        'count': rng.choice((0, 2, 6)) if arm == 'missile' else 0})
    # 002082B0 at every sweep (a negative span, one past INT_MAX columns);
    # 002081A0 at every count
    for sweep in sorted(set(SWEEPS)):
        out.append({'name': f'grid ring {sweep}', 'seed': rng.getrandbits(32), 'data': 'elf', 'sweep': sweep,
                    'steps': [(0x2082B0, [1, RING])]})
    for count in (4, 0, 1, 2, 7, -1, -6):
        out.append({'name': f'grid lines {count}', 'seed': rng.getrandbits(32), 'data': 'elf',
                    'steps': [(0x2081A0, [1, BUF, 2, count, 0x80CDC355])]})
    # the list walk at its extremes and a lock mark per entry
    for k in range(RM.pick(60, 30)):
        out.append({'name': f'locks/{k}', 'seed': rng.getrandbits(32), 'steps': [(0x199770, [PLAYER])],
                    'data': 'elf', 'count': rng.choice((1, 4, 8))})
    return out


def worker(case):
    seen, branches, ev, nc = run_case(case)
    return sorted(seen), sorted(branches), ev, nc


def fail_cases(rng):
    out = []
    for fn in (0x1DC960, 0x1DB9D0, 0x199770, 0x1DC610):
        base = {'name': f'fail {fn:06X}', 'seed': rng.getrandbits(32), 'data': 'elf', 'count': 6,
                'steps': [(fn, [PLAYER] if fn == 0x199770 else [1])]}
        if fn == 0x1DB9D0:
            base['steps'] = [(fn, [1, F(1.0)])]
        row = run_case(dict(base))
        n = row[3]
        ks = set(range(1, n + 1)) if RM.FULL else set(range(1, min(n, 4) + 1)) | set(range(1, n + 1, max(1, n // 6))) | {n}
        for k in sorted(ks):
            out.append(dict(base, failat=k, name=f'{base["name"]} at {k}'))
    out.append({'name': 'unmapped cursor', 'seed': 1, 'data': 'elf', 'steps': [(0x1DB9D0, [1, F(1.0)])],
                'unmapped': CTX + 0x14})
    out.append({'name': 'unmapped stack', 'seed': 2, 'data': 'elf', 'steps': [(0x1DC890, [1, F(0.5)])],
                'unmapped': SP - 0x90 + 0x40})
    return out


def refusals():
    """No map, no call, a latched fault; a NULL call never reached."""
    count = 0
    mem = (C.c_uint8 * 0x2000000)()
    stack = (C.c_uint8 * 0x100000)()

    def mp(_, a, size, write):
        if a + size <= 0x2000000:
            return C.addressof(mem) + a
        if 0x7F000000 <= a and a + size <= 0x7F100000:
            return C.addressof(stack) + a - 0x7F000000
        return None
    struct.pack_into('<I', mem, 0x275670, CTX)
    struct.pack_into('<I', mem, CTX + 0x14, PKT)
    mapfn = MAP(mp)
    for fn, (name, kinds) in ENTRIES.items():
        args = to_native(kinds, [1, F(1.0) if 'f' in kinds else 0x80000000, 0, 4, 0][:len(kinds)])
        if fn == 0x199770:
            args = [PLAYER]
        if fn == 0x2081A0:
            args = [1, BUF, 2, 4, 0]
        if fn == 0x2082B0:
            args = [1, RING]
        for absent in ('map', 'call', 'latched'):
            h = Host(None, MAP() if absent == 'map' else mapfn, CALL(), STORE(), SP,
                     7 if absent == 'latched' else 0, 0, 0)
            rc = getattr(LIB, 'em_sight_' + name)(C.byref(h), *args)
            if absent == 'call' and fn == 0x2081A0:
                # a leaf: no callee to reach, so a NULL call is never a fault
                assert rc == 0 and h.fault == 0, (hex(fn), absent, rc, h.fault)
                continue
            assert rc == -1 and h.fault == (7 if absent == 'latched' else 1), (hex(fn), absent, rc, h.fault)
            assert absent == 'latched' or h.fault_function == fn, (hex(fn), hex(h.fault_function))
            count += 1
    # the dispatcher: an unknown function is not run; a known one is
    c = Call(0x1DD170, SP)
    assert LIB.em_sight_call(None, C.byref(c)) == 1
    h = Host(None, mapfn, CALL(), STORE(), 0, 0, 0, 0)
    c = Call(0x2081A0, SP, (U64 * 7)(1, 0x01000000, 2, 4, 0x80CDC355))
    assert LIB.em_sight_call(C.byref(h), C.byref(c)) == 0 and h.sp == SP and h.fault == 0
    return count + 2


def main():
    global ELF, LIB
    start = time.monotonic()
    ELF = S.read_elf()
    LIB = build()
    ee = FallEE(ELF)
    targets = set()
    for fn, size in FUNCS.items():
        for pc in range(fn, fn + size, 4):
            word = ee.load(pc)
            if word >> 26 == 3:
                targets.add((word & 0x3FFFFFF) << 2)
    assert targets - set(FUNCS) == set(SIG), (sorted(hex(t) for t in targets - set(FUNCS) - set(SIG)),
                                              sorted(hex(t) for t in set(SIG) - targets))
    all_cases = cases()
    selected = RM.select(all_cases, 170, 0x5167, axes=(lambda c: c['steps'][0][0], lambda c: c['data']),
                         keep=lambda i, c: c['name'].startswith('grid') or
                         (c['name'].startswith('arm') and int(c['name'].split('/')[1]) < 6))
    RM.banner(RM.part(len(selected), len(all_cases), 'sight cases'))
    rows = RM.parallel_map(worker, selected, cost=lambda c: len(c['steps']) * (3 if 'arm' in c['name'] else 1))
    seen = set().union(*(set(r[0]) for r in rows))
    branches = set().union(*(set(tuple(b) for b in r[1]) for r in rows))
    missing = sorted(CODE - DEAD - seen)
    assert not missing, ('instruction words never executed', [hex(a) for a in missing[:24]], len(missing))
    assert not (DEAD & seen), ('a word named unreachable executed', sorted(hex(a) for a in DEAD & seen))
    sites = set()
    for pc in sorted(CODE - DEAD):
        word = ee.load(pc)
        op, rs, rt = word >> 26, word >> 21 & 31, word >> 16 & 31
        if op in (4, 20) and rs == 0 and rt == 0:
            continue
        if ee.branch(word, pc) is not None:
            sites.add(pc)
    gaps = sorted((hex(pc), b) for pc in sites for b in (False, True)
                  if (pc, b) not in branches and not (pc in NEVER_TAKEN and b))
    assert not gaps, ('branch outcomes never exercised', gaps)
    fails = fail_cases(random.Random(0xFA11))
    fail_rows = RM.parallel_map(run_case, fails)
    refused = refusals()
    words = len(CODE)
    print('sight vs original instructions: PASS %d cases (%d arm chains), %d callee entries compared; '
          '%d of %d words executed (%d unreachable, named), %d branch outcomes; %d callee-failure / unmapped '
          'cuts; %d host refusals (%.1fs)' %
          (len(selected), sum(1 for c in selected if c['name'].startswith('arm')), sum(r[3] for r in rows),
           words - len(DEAD), words, len(DEAD), len(branches), len(fail_rows), refused,
           time.monotonic() - start))


if __name__ == '__main__':
    main()
