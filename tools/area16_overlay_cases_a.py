"""AREA16 overlay oracle cases, group a (tools/test_area16_overlay_reference.py):
0x823580, 0x8261A0, 0x826C50, 0x827030 (src/game/em_area16_overlay.c)."""
import random

S_ = 's'
SOURCE = 'em_area16_overlay.c'
FUNCS = {
    0x823580: ('em_area16_overlay_00823580', 0x214, ['u', S_], None),
    0x8261A0: ('em_area16_overlay_008261A0', 0x224, ['u', S_], None),
    0x826C50: ('em_area16_overlay_00826C50', 0x3D8, ['u', S_], None),
    0x827030: ('em_area16_overlay_00827030', 0x3C0, ['u', S_], None),
}
# 0x823580's +0x94 outside 0xA..0xE: the original goes on with its caller's
# s0 / f20 / f21 (no assignment on that arm); the native latches
# EM_AREA16_OVERLAY_FAULT_CALLER_REGISTER there (checked by extra_checks)
DEAD = {pc: "0x823580's +0x94 arm outside 0xA..0xE (the caller's registers)" for pc in (0x82361C,)}
# 0x823580 stores its size (kept in a callee-saved float register) as the
# frame vector's z: a frame local, not a register save
LOCAL_STORES = {0x8236F4}
SELF_ENTRIES = {0x8261A0, 0x826C50, 0x827030}
TABLE = 0x1E75000        # a designed D_0024D7C0[area][sub] record table
PER_AREA = 0x1E75800     # its per-sub pointer row
SWAP = 0x1E75900         # the byte *D_008102C8 points at
OBJ = 0x1E76000          # D_00275B40's designed object table (27 pointers) and objects


def cases(add, K):
    b, h, w, f, fw = K.b, K.h, K.w, K.f, K.fw
    A, B = K.A, K.B

    def st(state, step=None):
        return [(A + 4, b(state))] + ([(A + 5, b(step))] if step is not None else [])

    # the spawn-id walk: area 0x10, sub 1, record +0x9A
    def table(rec=3, ident=0x1234):
        return [(0x24D7C0 + 4 * 0x10, w(PER_AREA)), (0x810700, b(0x10)), (0x810701, b(1)),
                (PER_AREA + 4, w(TABLE)), (A + 0x9A, b(rec)), (TABLE + 0x28 * (rec + 1) + 6, h(ident))]

    # ---- 0x8261A0
    for bits in (0, 2, 0xFD, 0xFF):
        for r in (0, 1, -1):
            add(f'61a0/0/{bits:x}/{r}', 0x8261A0, st(0) + table() + [(0x81080B, b(bits))], {0x1B0FD0: [r]})
    for half in (0, 1, 0x8000):
        for parent in (0xC9, 0xC8, 0):
            for swap in (0, 1, 2):
                add(f'61a0/1/{half:x}/{parent:x}/{swap}', 0x8261A0,
                    st(1) + table(5, 0xBEEF) + [(A + 0x36, h(half)), (A + 0x18, w(B)), (B + 4, b(parent)),
                                                (0x8102C8, w(SWAP)), (SWAP, b(swap)), (0x81080B, b(0x41)),
                                                (0x28A59C, w(0x55AA))],
                    {0x1C6120: [0x1777], 0x1B17A0: [1]})
    for state in (2, 3, 4, 0xC9):
        add(f'61a0/{state:x}', 0x8261A0, st(state))
    rng = random.Random(0xA16A)
    for n in range(K.O.reference_mode.pick(24, 4)):
        add(f'61a0/random/{n}', 0x8261A0,
            st(rng.choice((0, 1, 1, 2, 3))) + table(rng.choice((0, 1, 7)), rng.randrange(0x10000)) +
            [(0x81080B, b(rng.choice((0, 2, 0xFF)))), (A + 0x36, h(rng.choice((0, 1)))), (A + 0x18, w(B)),
             (B + 4, b(rng.choice((0xC9, 1)))), (0x8102C8, w(SWAP)), (SWAP, b(rng.choice((0, 1))))],
            {0x1B0FD0: [rng.choice((0, 1))], 0x1C6120: [rng.randrange(1 << 16)]})

    # ---- 0x823580
    for state in (0, 2, 3, 4):
        add(f'3580/{state}', 0x823580, st(state) + [(A + 0x94, h(0xA))])
    mat = [(A + 0xD0, fw(1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1., 0., 10., 20., 30., 1.))]
    for sel in (0xA, 0xB, 0xC, 0xD, 0xE):
        for spawns in ((K.SPAWN, K.SPAWN + 0x400), (0, K.SPAWN), (0, 0)):
            add(f'3580/1/{sel:x}/{spawns[0]:x}/{spawns[1]:x}', 0x823580, st(1) + [(A + 0x94, h(sel))] + mat,
                {0x1EFEB0: list(spawns) + [0x1234, 0]})
    add('3580/1/rot', 0x823580, st(1) + [(A + 0x94, h(0xC)),
        (A + 0xD0, fw(.5, .25, -.75, 0., -.125, 2., 3., 0., 7., -8., .5, 0., -100., 4.5, 1e3, 1.))],
        {0x1EFEB0: [K.SPAWN, K.SPAWN + 0x400]})

    # ---- 0x826C50
    def chain(depth=5):
        nodes = [A] + [B + 0x400 * k for k in range(depth)]
        return [(n + 0x18, w(nodes[i + 1])) for i, n in enumerate(nodes[:-1])]
    for r in (0, 1):
        for k in (0, 1, 2, 3):
            add(f'6c50/0/{r}/{k}', 0x826C50, st(0) + mat + [(A + 3, b(k)), (A + 0x30, w(0x5555))], {0x1B0FD0: [r]})
    for bits in (0, 4, 0xFB, 0xFF):
        for k in (0, 1, 2, 3):
            for r in (0, 1):
                add(f'6c50/1/{bits:x}/{k}/{r}', 0x826C50, st(1) + [(A + 0xB, b(bits)), (A + 3, b(k)),
                    (A + 0x2E4, fw(1.5, 2.5, 3.5))], {0x1BA1F0: [r]})
    for n in (0, 1, 2, 120, 0x8000, 0xFFFF):
        for k in (0, 1, 2, 3):
            for r in (0, 1):
                add(f'6c50/4/{n:x}/{k}/{r}', 0x826C50, st(4) + chain() + [(A + 0x28, h(n)), (A + 3, b(k)),
                    (A + 0x2E4, fw(4., 5., 6.))], {0x1BA1F0: [r]})
    for state in (2, 3, 5, 0xFF):
        add(f'6c50/{state:x}', 0x826C50, st(state))

    # ---- 0x827030
    objs = [(0x275B40, w(OBJ))] + [(OBJ + 4 * i, w(OBJ + 0x100 + 0x100 * i)) for i in range(27)]
    for r in (0, 1):
        add(f'7030/0/{r}', 0x827030, st(0), {0x1B0FD0: [r]})
    for flip in (0, 1):
        for d in (1, -1, 0, 0x10001, 0x7FFF0000):
            for n in (0, 1, 15, 29, 30, 31, -1, 0x7FFF):
                add(f'7030/1/{flip}/{d & 0xFFFFFFFF:x}/{n & 0xFFFF:x}', 0x827030,
                    st(1) + objs + [(A + 0x2A, h(flip)), (A + 0x1F0, w(d)), (A + 0x28, h(n)),
                                    (A + 0x1F4, f(.25)), (A + 3, b(0))])
    # the two boxes: each bound just inside, on it and just outside
    for k, box in ((1, ((180., 201.), (149.9, 200.4), (124., 239.7))), (0, ((205., 226.), (99.9, 152.), (122., 236.)))):
        mid = [(lo + hi) / 2 for lo, hi in box]
        for axis in range(3):
            for edge in (0, 1):
                bound = box[axis][edge]
                for name, d in (('in', 1 if edge == 0 else -1), ('on', 0), ('out', -1 if edge == 0 else 1)):
                    pos = [K.f(v) for v in mid]
                    pos[axis] = K.ulp(bound, d)
                    add(f'7030/box/{k}/{axis}/{edge}/{name}', 0x827030,
                        st(1) + objs + [(A + 3, b(k)), (A + 0x1F0, w(1)), (A + 0x1F4, f(1 / 6)), (A + 0x28, h(7)),
                                        (0x810350, b''.join(pos) + f(1.))])
    for state in (2, 3, 0xFF):
        add(f'7030/{state:x}', 0x827030, st(state))


def extra_checks(K, native):
    """0x823580 with +0x94 outside 0xA..0xE: the native latches the
    caller-register fault at 0x823580 (the original would read its caller's
    registers there) and calls no hook."""
    import ctypes as C
    O = K.O
    calls = []

    def refuse(*_):
        calls.append(1)
        return -1
    for sel in (0, 9, 0xF, 0x7FFF, -1):
        ram = bytearray(O.CAPTURES[K.DESIGNED][0])
        ram[K.A + 4] = 1
        ram[K.A + 0x94:K.A + 0x96] = (sel & 0xFFFF).to_bytes(2, 'little')
        buf = (C.c_uint8 * len(ram)).from_buffer(ram)
        view = O.BYTES_FN(lambda ctx, a, n: C.addressof(buf) + a if a + n <= len(ram) else None)
        keep = [view, O.CALLBACK_FN(refuse)]
        fields = {'ctx': None, 'bytes': view, 'w_callback': keep[1]}
        for name, _, args, result in O.HOOKS:
            fields[name] = O.hook_proto(args, result)(refuse)
            keep.append(fields[name])
        hooks = O.Hooks(**fields)
        fault = O.Fault()
        rc = native.em_area16_overlay_00823580(C.byref(hooks), C.c_uint32(K.A), C.c_uint32(O.STACK_TOP),
                                               C.byref(fault))
        assert rc == -1 and (fault.address, fault.code) == (0x823580, 6) and not calls, (sel, rc, fault.code)
        del buf
