"""AREA16 overlay oracle cases, group b (tools/test_area16_overlay_reference.py):
0x824440, 0x824540, 0x824690, 0x824E10, 0x825260, 0x825630, 0x825660,
0x825670, 0x8256E0, 0x825730, 0x825780 (src/game/em_area16_overlay_b.c)."""
import random

U_, I_, S_ = 'u', 'i', 's'
SOURCE = 'em_area16_overlay_b.c'
FUNCS = {
    0x824440: ('em_area16_overlay_00824440', 0xF8, [U_, S_], None),
    0x824540: ('em_area16_overlay_00824540', 0x148, [U_, S_], None),
    0x824690: ('em_area16_overlay_00824690', 0x778, [U_, S_], None),
    0x824E10: ('em_area16_overlay_00824E10', 0x44C, [U_, S_], None),
    0x825260: ('em_area16_overlay_00825260', 0xAC, [U_, S_], None),
    0x825630: ('em_area16_overlay_00825630', 0x24, [U_, U_, U_], 'i'),
    0x825660: ('em_area16_overlay_00825660', 0xC, [U_, S_], None),
    0x825670: ('em_area16_overlay_00825670', 0x64, [U_, U_, I_, S_], None),
    0x8256E0: ('em_area16_overlay_008256E0', 0x50, [U_, I_, S_], None),
    0x825730: ('em_area16_overlay_00825730', 0x44, [U_, S_], None),
    0x825780: ('em_area16_overlay_00825780', 0x74, [U_, I_, S_], None),
}
DEAD = {}
SELF_ENTRIES = {0x824440, 0x824540, 0x824690, 0x824E10, 0x825260}
TABLE = 0x1E70000        # a designed D_0024D7C0[area][sub] record table
PER_AREA = 0x1E70800     # its per-sub pointer row
MAT_A = 0x1E71000        # an object whose +0x90 is a matrix (self's +0x12C / +0x110)
MAT_B = 0x1E71200        # a second one (an attachment's +0x12C)
CHAIN = 0x1E72000        # the fifteen +0x1C chain members, 0x300 apart


def cases(add, K):
    b, h, w, f, fw = K.b, K.h, K.w, K.f, K.fw
    A, B, REC, WORK, SPAWN = K.A, K.B, K.REC, K.WORK, K.SPAWN
    O = K.O
    C1, C2, C3 = SPAWN, SPAWN + 0x400, SPAWN + 0x800

    def st(state, step=None):
        return [(A + 4, b(state))] + ([(A + 5, b(step))] if step is not None else [])

    def table(rec=3, ident=0x1234, ident2=0x5678):
        return [(0x24D7C0 + 4 * 0x10, w(PER_AREA)), (0x810700, b(0x10)), (0x810701, b(1)),
                (PER_AREA + 4, w(TABLE)), (A + 0x9A, b(rec)), (TABLE + 0x28 * (rec + 1) + 6, h(ident)),
                (TABLE + 0x28 * rec + 6, h(ident2))]

    def mats():
        return [(A + 0x12C, w(MAT_A)), (A + 0x110, w(MAT_B)),
                (MAT_A + 0x90, fw(1.0, 0.5, 0.0, 0.0, 0.0, 2.0, 0.25, 0.0, 0.0, 0.0, 1.5, 0.0, 10.0, 20.0, 30.0, 1.0)),
                (MAT_B + 0x90, fw(0.0, 1.0, 0.0, 0.0, -1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, -5.0, 7.0, 3.0, 1.0))]

    def attach(state1=1, state2=1, state3=1, present=(True, True, True)):
        out = []
        for slot, c, s, p in ((0x2EC, C1, state1, present[0]), (0x2E8, C2, state2, present[1]),
                              (0x2E4, C3, state3, present[2])):
            out += [(A + slot, w(c if p else 0)), (c + 4, b(s)), (c + 0x12C, w(MAT_B)), (c + 0xA0, fw(0.5, 0.5, 0.5, 0.5))]
        return out

    # ---- 0x825660, 0x825630
    add('5660/0', 0x825660, [(A + 0x2E0, w(7))])
    for v in (0, 1, 0x80000000):
        add(f'5630/{v:x}', 0x825630, [(WORK + 8, w(v)), (A + 0x2E, h(0x1234))])

    # ---- 0x825670 (idx 1 the +0x12C matrix, others the +0x110 one)
    for idx in (0, 1, 2, -1):
        add(f'5670/{idx}', 0x825670, mats(), args=[A, B, idx, O.STACK_TOP])

    # ---- 0x8256E0
    for idx in (0, 1):
        for r in (0, 0x1234, -1):
            add(f'56e0/{idx}/{r}', 0x8256E0, mats(), {0x1D7FA0: [r]}, args=[A, idx, O.STACK_TOP])

    # ---- 0x825730
    for v in (-1, 0, 5, -2):
        add(f'5730/{v}', 0x825730, [(A + 0x2E0, w(v))])

    # ---- 0x825780
    add('5780/none', 0x825780, mats() + [(A + 0x2E0, w(-1))], args=[A, 1, O.STACK_TOP])
    for idx in (0, 1):
        for p in (0, C1):
            add(f'5780/{idx}/{p:x}', 0x825780, mats() + [(A + 0x2E0, w(9))], {0x1D8060: [p]},
                args=[A, idx, O.STACK_TOP])

    # ---- 0x825260
    for r in (0, 1, -1):
        add(f'5260/0/{r}', 0x825260, st(0), {0x1B10B0: [r]})
        add(f'5260/1/{r}', 0x825260, st(1), {0x1B17A0: [r]})
    for s in (2, 3, 4, 0xFF):
        add(f'5260/{s:x}', 0x825260, st(s))

    # ---- 0x824540 (the y threshold 195, the area test, the script end, the tail)
    for y in (195.0, 100.0, 'up', 'down'):
        yb = K.ulp(195.0, 1) if y == 'up' else K.ulp(195.0, -1) if y == 'down' else f(y)
        for r in (0, 1):
            for flag in (0, 1):
                add(f'4540/0/{y}/{r}/{flag}', 0x824540,
                    st(1, 0) + [(0x810354, yb), (0x810809, b(0)), (0x70003B92, b(flag))], {0x1B1EA0: [r]})
    for r in (0, 1):
        for d in (0, 1, 0xFF):
            for flag in (0, 1):
                add(f'4540/1/{r}/{d:x}/{flag}', 0x824540,
                    st(1, 1) + [(0x810809, b(d)), (0x70003B92, b(flag))], {0x1BA1F0: [r]})
    for d in (0, 2):
        add(f'4540/2/{d}', 0x824540, st(1, 2) + [(0x810809, b(d))])

    # ---- 0x824440
    for r1 in (1, -1, 0):
        for r2 in ((0, 1) if r1 == 0 else (0,)):
            add(f'4440/0/{r1}/{r2}', 0x824440, st(0) + [(A + 0x2E0, w(5))], {0x1BA1C0: [r1], 0x1B0FD0: [r2]})
    for d in (0, 1, 2, 0xFF):
        add(f'4440/1/{d:x}', 0x824440, st(1, 0) + [(0x810809, b(d)), (0x810354, f(200.0))], {0x1B1EA0: [1]})
    for s in (2, 3, 4, 0xC9):
        add(f'4440/{s:x}', 0x824440, st(s))

    # ---- 0x824690 state 0
    for e789 in (0xFF, 0):
        for d in (0, 1, 2, 3, 0xFF):
            for r in (0, 1):
                add(f'4690/0/{e789:x}/{d:x}/{r}', 0x824690,
                    st(0) + table(4) + mats() + [(0x810789, b(e789)), (0x810809, b(d)), (A + 0x2E0, w(6))],
                    {0x1B0FD0: [r], 0x1C5570: [C1, C2, C3], 0x1D7FA0: [0x44]})
    add('4690/0/none', 0x824690, st(0) + mats() + [(0x810809, b(0xFF)), (A + 0x2E0, w(-1))],
        {0x1B0FD0: [0]})
    # state 1, step 0: the owner starts the script (attachments present / absent)
    chain = [(A + 0x1C, w(CHAIN))] + [(CHAIN + 0x300 * k + 0x1C, w(CHAIN + 0x300 * (k + 1))) for k in range(15)]
    for d in (0, 1):
        for pres in ((True, True, True), (False, False, False), (True, False, True)):
            tag = ''.join('1' if p else '0' for p in pres)
            add(f'4690/1/0/own/{d}/{tag}', 0x824690,
                st(1, 0) + attach(present=pres) + chain +
                [(0x8104C4, w(A)), (0x810809, b(d)), (0x810789, b(0)), (A + 0x2E0, w(3)), (0x70003B92, b(1))])
    # step 0, not the owner / D_00810809 not below 2: the marker
    for who, d in ((B, 0), (A, 2), (A, 0xFF)):
        for e789, flag in ((0, 0), (1, 1), (1, 0), (0xFF, 1)):
            add(f'4690/1/0/mark/{who:x}/{d:x}/{e789:x}/{flag}', 0x824690,
                st(1, 0) + [(0x8104C4, w(who)), (0x810809, b(d)), (0x810789, b(e789)), (0x70003B92, b(flag))])
    # step 1: the script's end
    for r in (0, 1):
        for d in (0, 1, 2, 0xFF):
            add(f'4690/1/1/{r}/{d:x}', 0x824690,
                st(1, 1) + table(2) + attach() + [(0x810809, b(d)), (0x810789, b(1)), (0x70003B92, b(1)),
                                                  (A + 9, b(0x21))],
                {0x1BA1F0: [r], 0x1AFA90: [C1 + 0x1000, C1 + 0x1400], 0x1B0FD0: [0]})
    for pres in ((False, False, False), (True, True, True)):
        for r in (0, 1):
            tag = ''.join('1' if p else '0' for p in pres)
            add(f'4690/1/1/spawn/{tag}/{r}', 0x824690,
                st(1, 1) + table(0) + attach(present=pres) + [(0x810809, b(2)), (0x810789, b(2))],
                {0x1BA1F0: [1], 0x1AFA90: [0, 0] if r else [C1 + 0x1000, 0], 0x1B0FD0: [r]})
    for step in (2, 0xFF):
        add(f'4690/1/{step:x}', 0x824690, st(1, step) + [(0x810789, b(1)), (0x70003B92, b(0))])
    add('4690/2', 0x824690, st(2))
    for s in (3, 4, 0xC9):
        for v in (-1, 8):
            add(f'4690/{s:x}/{v}', 0x824690, st(s) + [(A + 0x2E0, w(v))])

    # ---- 0x824E10
    def group(state, kind=1, mode=0, count=0, bit=0, mask=0, extra=()):
        return (st(state) + mats() + [(A + 0x20, w(REC)), (A + 0x24, w(B)), (REC + 4, h(kind)),
                                      (REC + 0xA, h(mode)), (REC + 0xC, f(0.75)), (A + 0x28, h(count)),
                                      (A + 0x2E, h(bit)), (B + 0x2E, h(mask))] + list(extra))
    for r in (1, -1, 0):
        add(f'4e10/0/{r}', 0x824E10, group(0, extra=attach()), {0x1BAD40: [r], 0x1C5570: [C1, C2, C3],
                                                               0x1D7FA0: [0x77]})
    add('4e10/0/null', 0x824E10, group(0, extra=attach()), {0x1BAD40: [0], 0x1C5570: [0, 0, 0]})
    for bit, mask in ((3, 8), (35, 8), (3, 0xFFF7), (16, 0xFFFF), (15, 0x8000)):
        add(f'4e10/1/bit/{bit}/{mask:x}', 0x824E10, group(1, mode=3, bit=bit, mask=mask, extra=attach()))
    for kind in (0x270D, 0x270C, 1, -1):
        for mode in (0, 1, 3, 4, 5, -1, 0x100):
            add(f'4e10/1/{kind:x}/{mode:x}', 0x824E10,
                group(1, kind=kind, mode=mode, extra=attach() + [(A + 0x2E0, w(4))]), {0x1D8060: [C1 + 0x1000]})
    add('4e10/1/handle', 0x824E10, group(1, mode=2, extra=attach() + [(A + 0x2E0, w(4))]), {0x1D8060: [0]})
    add('4e10/1/nohandle', 0x824E10, group(1, mode=2, extra=attach() + [(A + 0x2E0, w(-1))]))
    for count in (0x211F, 0x2120, 0x7FFF, 0x8000, 0):
        for s2 in (1, 0):
            add(f'4e10/1/count/{count:x}/{s2}', 0x824E10,
                group(1, mode=3, count=count, extra=attach(state1=s2, state2=s2, state3=s2) + [(A + 0x2E0, w(4))]))
    add('4e10/1/absent', 0x824E10, group(1, mode=3, count=0x211F, extra=attach(present=(False, False, False))))
    for kind in (0x270D, 1):
        for mode in (0, 1):
            add(f'4e10/2/{kind:x}/{mode}', 0x824E10, group(2, kind=kind, mode=mode))
    for pres in ((True, True, True), (False, False, False)):
        tag = ''.join('1' if p else '0' for p in pres)
        add(f'4e10/3/{tag}', 0x824E10, group(3, extra=attach(present=pres)))
    add('4e10/4', 0x824E10, group(4))

    rng = random.Random(0xA16B)
    for n in range(O.reference_mode.pick(24, 4)):
        add(f'4e10/random/{n}', 0x824E10,
            group(1, kind=rng.choice((0x270D, 1, 2)), mode=rng.choice((0, 2, 3, 4)),
                  count=rng.choice((0, 0x211F, 0x2120, rng.randrange(0x10000))),
                  bit=rng.randrange(40), mask=rng.randrange(0x10000),
                  extra=attach(rng.choice((0, 1)), rng.choice((0, 1)), rng.choice((0, 1)),
                               tuple(rng.choice((True, False)) for _ in range(3))) +
                  [(A + 0x2E0, w(rng.choice((-1, 2))))]),
            {0x1D8060: [rng.choice((0, C1 + 0x1000))]})
