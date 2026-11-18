"""AREA16 overlay oracle cases, group d (tools/test_area16_overlay_reference.py):
0x8273F0, 0x828200, 0x827B70, 0x827CE0, 0x827FA0 (src/game/em_area16_overlay_d.c)."""
import random

S_ = 's'
SOURCE = 'em_area16_overlay_d.c'
FUNCS = {
    0x8273F0: ('em_area16_overlay_008273F0', 0x774, ['u', S_], None),
    0x828200: ('em_area16_overlay_00828200', 0x204, ['u', S_], None),
    0x827B70: ('em_area16_overlay_00827B70', 0x164, ['u', S_], None),
    0x827CE0: ('em_area16_overlay_00827CE0', 0x2B4, ['u', S_], None),
    0x827FA0: ('em_area16_overlay_00827FA0', 0x25C, ['u', S_], None),
}
# 0x828200's +0xD other than 6, 0x25 and 0x14: the original's part count is
# its caller's s1 (no assignment on that arm); the native latches
# EM_AREA16_OVERLAY_FAULT_CALLER_REGISTER there (checked by extra_checks)
DEAD = {pc: "0x828200's +0xD arm outside 6 / 0x25 / 0x14 (the caller's register)" for pc in (0x8282F8,)}
SELF_ENTRIES = {0x8273F0, 0x828200, 0x827B70, 0x827CE0, 0x827FA0}


def cases(add, K):
    b, h, w, f, fw, ulp = K.b, K.h, K.w, K.f, K.fw, K.ulp
    A, B, REC, SPAWN, FREE = K.A, K.B, K.REC, K.SPAWN, K.FREE
    CH = SPAWN                 # the spawned child / the kept child
    LIFT = B                   # D_00275B40[2]: its +0x80 value slides
    ANC = FREE + 0x5000        # the follower's anchor (+0x18 chain end)
    MID = FREE + 0x5400        # a middle link
    OBJ = FREE + 0x5800        # the anchor's +0x118 object
    ROWS = FREE + 0x6000       # a designed D_0024D820[area] row pointer array
    REC_E = FREE + 0x6400      # the 44-byte spawn records
    BIG = FREE + 0x5C00

    def st(state, step=None):
        return [(A + 4, b(state))] + ([(A + 5, b(step))] if step is not None else [])

    def slid(v=100.0):
        return [(0x275B40, w(REC)), (REC + 8, w(LIFT)), (LIFT + 0x80, f(v))]

    def pos():
        return [(A + 0xB0, fw(10.0, 20.0, 30.0, 1.0)), (A + 0xC0, fw(1.5, 2.5, 3.5, 4.5))]

    # ---------------------------------------------------------------- 0x8273F0
    # state 0: the kind (+3), 001B0FD0, D_00810809, +0xE bit 0, the spawn
    for kind in (0, 0x31, 0x30):
        for r in (0, 1):
            for ev in (0, 1, 2, 0xFF):
                for e in (0, 1, 2):
                    for spawn in (0, CH):
                        if r and (ev or e or spawn):
                            continue
                        add(f'73f0/0/{kind:x}/{r}/{ev:x}/{e}/{spawn:x}', 0x8273F0,
                            st(0) + slid(123.5) + pos() + [(A + 3, b(kind)), (0x810809, b(ev)),
                                                          (A + 0xE, h(e)), (A + 0x2D4, w(9))],
                            {0x1B0FD0: [r], 0x1AFA90: [spawn]})
    # a lower +0x2E0 case with a small slide value (the float subtract)
    for v in (0.0, -35.5, 1e30, 25.5, 35.5):
        add(f'73f0/0/slide/{v}', 0x8273F0, st(0) + slid(v) + pos() + [(A + 3, b(0)), (0x810809, b(1))],
            {0x1B0FD0: [0], 0x1AFA90: [CH]})

    # state 1, sub-state 0
    for cnt in (0, 1, 2, 5, -3 & 0xFFFFFFFF):
        for ev in (0, 0xFF):
            for sound in (0, 0x31):
                for gate in (0, 3):
                    add(f'73f0/1/0/{cnt:x}/{ev:x}/{sound:x}/{gate}', 0x8273F0,
                        st(1, 0) + slid() + pos() + [(A + 0x2E4, w(cnt)), (0x810789, b(ev)), (A + 3, b(sound)),
                                                    (A + 0x56, h(gate))])
    # sub-state 1: the counter, the end switch, direction, child
    for ctr in (0, 197, 198, 199, 200, 0x7FFE, 0x7FFF, 0xFFFF):
        for ev in (0, 0xFF):
            for up in (0, 1):
                for kid in (0, CH):
                    add(f'73f0/1/1/{ctr:x}/{ev:x}/{up}/{kid:x}', 0x8273F0,
                        st(1, 1) + slid(50.25) + pos() + [(A + 0x28, h(ctr)), (0x810789, b(ev)), (A + 0x2EC, w(up)),
                                                         (A + 0x2DC, f(0.25)), (A + 0x2E0, f(35.5)), (A + 0x2E8, f(77.5)),
                                                         (A + 0x2D8, w(kid)), (CH + 0xB4, f(9.0)), (A + 0x52, h(7)),
                                                         (A + 0xB4, f(20.0))])
    # the tail: +0x2D4 countdown, retire the child
    for sub in (0, 2, 3):
        for d4 in (0, 1, 2, 7):
            for ev in (0, 0xFF):
                for up in (0, 1):
                    for kid in (0, CH):
                        for sound in (0, 0x31):
                            if sub == 3 and (up or sound):
                                continue
                            add(f'73f0/1/t{sub}/{d4}/{ev:x}/{up}/{kid:x}/{sound:x}', 0x8273F0,
                                st(1, sub) + slid() + pos() + [(A + 0x2D4, w(d4)), (0x810809, b(ev)), (A + 0x2EC, w(up)),
                                                              (A + 0x2D8, w(kid)), (A + 3, b(sound)),
                                                              (0x810789, b(0))])
    for state in (2, 3, 4, 0xFF):
        add(f'73f0/s/{state:x}', 0x8273F0, st(state))

    # ---------------------------------------------------------------- 0x827B70
    for r in (0, 1):
        for spawn in (0, CH):
            add(f'7b70/0/{r}/{spawn:x}', 0x827B70,
                st(0) + pos() + [(A + 0x2E0, f(12.5)), (A + 0x3, b(0x31))], {0x1B0FD0: [r], 0x1AFA90: [spawn]})
    for ctr in (0, 198, 199, 200, 201, 0x7FFE, 0x7FFF, 0xFFFF):
        for rnd in (0, 0x00010000, 0x3FFF0000, 0x3FFFFFFF, 0x40000000, 0x7FFF0000, 0x80000000, 0xFFFF0000,
                    0xFFFE0000, 0x0000FFFF):
            if ctr not in (199, 200, 201, 0) and rnd not in (0, 0x40000000):
                continue
            add(f'7b70/1/{ctr:x}/{rnd:x}', 0x827B70, st(1) + [(A + 0x28, h(ctr))], {0x122BB8: [rnd]})
    for state in (2, 3, 4, 0xFF):
        add(f'7b70/s/{state:x}', 0x827B70, st(state))

    # ---------------------------------------------------------------- 0x827CE0 / 0x827FA0
    def chain(n, lift=15.5):
        # +0x54 = n links; the anchor is the n-th ancestor
        links = [A, MID, ANC, BIG]
        out = [(A + 0x54, h(n)), (A + 0x18, w(MID)), (MID + 0x18, w(ANC)), (ANC + 0x18, w(BIG))]
        end = links[n]
        out += [(end + 0x2E0, f(lift)), (end + 0x118, w(OBJ)), (OBJ + 0xC0, fw(5.0, 6.0, 7.0, 1.0)),
                (OBJ + 0xC4, f(8.5))]
        return out, end

    def table(area, base, idx_col, flags=0x22, kind=7):
        # every row index points at its own records
        return [(0x24A850 + 2 * area, h(base)), (0x24D820 + 4 * area, w(ROWS))] + \
               [(ROWS + 4 * j, w(REC_E + 0x100 * j)) for j in range(0, 8)] + \
               [(REC_E + 0x100 * j + 44 * col + off, val) for j in range(0, 8) for col in (0, 1)
                for off, val in ((2, b(0x41 + col)), (4, h(flags | kind)), (6, h(0x3412)),
                                 (8, b(0x55)), (0xA, h(0x6789)), (0xC, h(0x1111 + col)), (0xE, h(0x2222 + j)),
                                 (0x28, w(0x00AABBCC + j)))]

    for fn in (0x827CE0, 0x827FA0):
        tag = f'{fn & 0xFFFF:04x}'
        for n in (0, 1, 2):
            ch, end = chain(n)
            for r in (0, 1):
                add(f'{tag}/0/{n}/{r}', fn, st(0) + ch + pos() + [(A + 0xB4, f(3.25))], {0x1B10B0: [r]})
        add(f'{tag}/0/neg', fn, st(0) + chain(2)[0] + [(A + 0x54, h(-1))] + pos())
        ch, end = chain(1)
        for e2ec in (0, 1):
            for e2d4 in (0, 1):
                add(f'{tag}/1/gate/{e2ec}/{e2d4}', fn,
                    st(1) + ch + pos() + [(A + 0x1F8, w(end)), (A + 0x1F0, f(-30.0)), (end + 0x2EC, w(e2ec)),
                                         (end + 0x2D4, w(e2d4)), (A + 0x56, h(0))] + table(3, 0, 0) +
                    [(0x810700, b(3))], {0x1B17A0: [0, 1], 0x1B11E0: [0], 0x1AFA90: [CH]})
        for idx in (-1, 0, 2, 5):
            for base in (0, 2):
                for col in (0, 1):
                    for exists in (0, 1):
                        for spawn in (0, CH):
                            for flags in (0x00, 0x20, 0xE0, 0x100, 0xFFC0):
                                if flags not in (0, 0x20) and not (idx == 0 and base == 0 and col == 0):
                                    continue
                                if exists and (spawn or flags):
                                    continue
                                if idx < 0 and (base or col or exists or spawn or flags):
                                    continue
                                add(f'{tag}/1/spawn/{idx}/{base}/{col}/{exists}/{spawn:x}/{flags:x}', fn,
                                    st(1) + ch + pos() + [(A + 0x1F8, w(end)), (A + 0x1F0, f(-30.0)),
                                                          (A + 0x56, h(idx)), (A + 0xE, b(col)), (0x810700, b(3)),
                                                          (0x810701, b(9))] + table(3, base, col, flags | 2, 0) +
                                    [(0x24D820 + 4 * 3, w(ROWS))],
                                    {0x1B11E0: [exists], 0x1AFA90: [spawn], 0x1B17A0: [1]})
        # 0xFFC2 style flags (negative halfword) and the +0xE column 0
        add(f'{tag}/1/negflags', fn, st(1) + ch + pos() + [(A + 0x1F8, w(end)), (A + 0x1F0, f(-30.0)),
                                                         (A + 0x56, h(0)), (0x810700, b(3))] +
            table(3, 0, 0, 0xFFC2, 0), {0x1B11E0: [0], 0x1AFA90: [CH], 0x1B17A0: [0]})
        for state in (2, 3, 4, 0xFF):
            add(f'{tag}/s/{state:x}', fn, st(state))

    # ---------------------------------------------------------------- 0x828200
    for area, sub in ((0x10, 0), (0x10, 1), (0x0F, 0), (0, 0), (0x11, 0)):
        for r in (0, 1):
            add(f'8200/0/{area:x}/{sub}/{r}', 0x828200, st(0) + [(0x810700, b(area)), (0x810701, b(sub)),
                                                                 (A + 0x44, w(0x1234))], {0x1B0FD0: [r]})
    rng = random.Random(0xD16)
    for kind in (6, 0x25, 0x14):
        for gp in (0, 0x55AA):
            add(f'8200/1/{kind:x}/{gp:x}', 0x828200,
                st(1) + pos() + [(A + 0xD, b(kind)), (0x28A59C, w(gp)), (A + 0x110, w(B)),
                                 (A + 0xD0, fw(1.0, 0.0, 0.0, 0.0))],
                {0x1C6120: [rng.randrange(1 << 20) for _ in range(16)], 0x1B17A0: [1, 0, 1, 0]})
    for state in (2, 3, 4, 0xFF):
        add(f'8200/s/{state:x}', 0x828200, st(state))


def extra_checks(K, native):
    """0x828200 in state 1 with +0xD outside 6 / 0x25 / 0x14: after 001C6380
    and the two saves (the original's first three calls) the native latches
    the caller-register fault at 0x828200 and calls nothing more."""
    import ctypes as C
    O = K.O
    calls = []

    def ok(*_):
        calls.append(1)
        return 0
    for kind in (0, 7, 0x13, 0xFF):
        ram = bytearray(O.CAPTURES[K.DESIGNED][0])
        ram[K.A + 4] = 1
        ram[K.A + 0xD] = kind
        buf = (C.c_uint8 * len(ram)).from_buffer(ram)
        view = O.BYTES_FN(lambda ctx, a, n: C.addressof(buf) + a if a + n <= len(ram) else None)
        keep = [view, O.CALLBACK_FN(ok)]
        fields = {'ctx': None, 'bytes': view, 'w_callback': keep[1]}
        for name, _, args, result in O.HOOKS:
            fields[name] = O.hook_proto(args, result)(ok)
            keep.append(fields[name])
        hooks = O.Hooks(**fields)
        fault = O.Fault()
        calls.clear()
        rc = native.em_area16_overlay_00828200(C.byref(hooks), C.c_uint32(K.A), C.c_uint32(O.STACK_TOP),
                                               C.byref(fault))
        assert rc == -1 and (fault.address, fault.code) == (0x828200, 6) and len(calls) == 3, (kind, rc, calls)
        del buf
