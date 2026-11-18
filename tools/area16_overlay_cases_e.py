"""AREA16 overlay oracle cases, group e (tools/test_area16_overlay_reference.py):
0x8263D0, 0x825310 (src/game/em_area16_overlay_e.c)."""
import random

S_ = 's'
SOURCE = 'em_area16_overlay_e.c'
FUNCS = {
    0x8263D0: ('em_area16_overlay_008263D0', 0x674, ['u', S_], None),
    0x825310: ('em_area16_overlay_00825310', 0x314, ['u', S_], None),
}
DEAD = {}
SELF_ENTRIES = {0x8263D0, 0x825310}


def cases(add, K):
    b, h, w, f, fw, ulp = K.b, K.h, K.w, K.f, K.fw, K.ulp
    A, B, REC, SPAWN = K.A, K.B, K.REC, K.SPAWN
    EV = 0x81080B

    def st(state, step=None):
        return [(A + 4, b(state))] + ([(A + 5, b(step))] if step is not None else [])

    # D_00275B40 -> REC, whose [2] is B (B + 0x80 is the slid value)
    def slid(v=0.0):
        return [(0x275B40, w(REC)), (REC + 8, w(B)), (B + 0x80, f(v))]

    def node(fade=0.5, wave=0.3, eff=SPAWN):
        return [(A + 0x2EC, f(fade)), (A + 0x2E8, f(wave)), (A + 0x2E4, f(0.7)), (A + 0x2E0, f(0.1)),
                (A + 0x2D0, w(eff)), (A + 0xB0, fw(10.0, 20.0, 30.0, 1.0))]

    # ---- 0x8263D0, state 0
    for r in (0, 1):
        for bits in (0, 1, 2, 3, 4, 5, 0xFF):
            add(f'63d0/0/{r}/{bits:x}', 0x8263D0, st(0) + slid(0.5) + node() + [(EV, b(bits))],
                {0x1B0FD0: [r], 0x122BB8: [0x40000000, -0x80000000, 12345], 0x1EFD20: [SPAWN]})
    add('63d0/0/nospawn', 0x8263D0, st(0) + slid() + [(EV, b(1))], {0x1B0FD0: [0], 0x122BB8: [1, 2, 3]})
    # state 1, the retire bit and the +5 sub-states
    for bits in (4, 5, 6, 7):
        for eff in (0, SPAWN):
            for step in (0, 1, 2):
                add(f'63d0/1/ret/{bits:x}/{eff:x}/{step}', 0x8263D0,
                    st(1, step) + slid() + node(eff=eff) + [(EV, b(bits)), (A + 0xB, b(4)), (A + 0x28, h(5)),
                                                            (A + 0x2DC, w(10)), (A, b(1))],
                    {0x1BA1F0: [0], 0x1B17A0: [1]})
    # sub-state 0: +0xB bit 2 and the script choice
    for bits in (0, 1, 2, 3):
        for kb in (0, 4, 0xFB, 0xFF):
            add(f'63d0/1/0/{bits:x}/{kb:x}', 0x8263D0,
                st(1, 0) + slid() + node() + [(EV, b(bits)), (A + 0xB, b(kb)), (A + 0x28, h(77)), (A, b(1))])
    # sub-state 1: the +0x28 counter, 001BA1F0 and the +0x2DC slide
    for t in (0, 118, 119, 120, 121, 0x7FFF, 0x8000, 0xFFFF):
        for done in (0, 1):
            add(f'63d0/1/1/t{t:x}/{done}', 0x8263D0,
                st(1, 1) + slid() + node() + [(EV, b(0)), (A + 0x28, h(t)), (A + 0x2DC, w(0))],
                {0x1BA1F0: [done]})
    for n in (0, 119, 120, 121, 130, 137, 138, 139, 140, 200, -5, 0x7FFFFFFF):
        for down in (0, 1):
            for bits in (0, 1):
                add(f'63d0/1/1/n{n & 0xFFFFFFFF:x}/{down}/{bits}', 0x8263D0,
                    st(1, 1) + slid(-0.75) + node() + [(EV, b(bits | 0x80)), (A + 0x28, h(120)),
                                                       (A + 0x2DC, w(n & 0xFFFFFFFF)), (A + 0x2D4, w(down))],
                    {0x1BA1F0: [0]})
    # the fade (+0x2EC), the draw, the wave (+0x2E8) and the 001F5940 arm
    fades = (0.0, 0.02, 0.5, 0.98, 0.99, 1.0, 0.01, -0.5)
    for bits in (0, 1, 3):
        for fade in fades:
            for eff in (0, SPAWN):
                add(f'63d0/1/fade/{bits}/{fade}/{eff:x}', 0x8263D0,
                    st(1, 2) + slid() + node(fade, 1.5, eff) + [(EV, b(bits)), (A, b(1))],
                    {0x1CCF70: [0x77], 0x1B17A0: [0]})
    for fade in (ulp(0.98, -1), ulp(0.98, 1), ulp(0.02, -1), ulp(0.02, 1)):
        for bits in (0, 1):
            add(f'63d0/1/fadeulp/{bits}/{fade.hex()}', 0x8263D0,
                st(1, 2) + slid() + node(0.5) + [(A + 0x2EC, fade), (EV, b(bits)), (A, b(0))])
    for wave in (1.98, 1.99, 2.0, 0.0, -3.0):
        for me in (0, 1, 2):
            add(f'63d0/1/wave/{wave}/{me}', 0x8263D0,
                st(1, 2) + slid() + node(0.5, wave) + [(EV, b(1)), (A, b(me))], {0x1CCF70: [-1]})
    for wave in (ulp(1.98, -1), ulp(1.98, 1)):
        add(f'63d0/1/waveulp/{wave.hex()}', 0x8263D0, st(1, 2) + slid() + node(0.5) + [(A + 0x2E8, wave), (EV, b(1))])
    # state 2, 3 and the others
    for eff in (0, SPAWN):
        add(f'63d0/2/{eff:x}', 0x8263D0, st(2) + node(eff=eff), {0x1B17A0: [1]})
    for state in (3, 4, 0xFF):
        add(f'63d0/{state:x}', 0x8263D0, st(state) + node())
    rng = random.Random(0xA16E)
    for n in range(K.O.reference_mode.pick(40, 6)):
        add(f'63d0/random/{n}', 0x8263D0,
            st(rng.choice((0, 1, 1, 1, 2, 3)), rng.choice((0, 1, 1, 2))) + slid(rng.uniform(-2, 1)) +
            node(rng.uniform(-0.1, 1.1), rng.uniform(0, 2.1), rng.choice((0, SPAWN))) +
            [(EV, b(rng.randrange(256))), (A + 0xB, b(rng.randrange(256))), (A + 0x28, h(rng.choice((50, 119, 120)))),
             (A + 0x2DC, w(rng.choice((0, 120, 125, 138, 139)))), (A + 0x2D4, w(rng.choice((0, 1)))),
             (A, b(rng.choice((0, 1))))],
            {0x1B0FD0: [rng.choice((0, 1))], 0x1BA1F0: [rng.choice((0, 0, 1))], 0x1CCF70: [rng.randrange(1 << 16)],
             0x122BB8: [rng.randrange(-(1 << 31), 1 << 31) for _ in range(3)], 0x1EFD20: [rng.choice((0, SPAWN))]})

    # ---- 0x825310
    def blk(phases=(0.1, 0.5, 1.99, 2.5), seed=0x12345678, x=5.0, z=-7.0):
        return [(A + 0x1F0, fw(*phases)), (A + 0x200, f(x)), (A + 0x204, f(z)), (A + 0x208, w(seed & 0xFFFFFFFF))]

    for r in (0, 1):
        add(f'5310/0/{r}', 0x825310, st(0) + blk() + [(A + 0xB0, fw(1.0, 2.0, 3.0, 1.0))],
            {0x1B0FD0: [r], 0x122BB8: [0x7FFFFFFF, 1, -1, -0x80000000, 0x10000]})
    for flag in (0, 1, 2, 3):
        for b92 in (0, 1, 0x80):
            for act in (0, 1):
                add(f'5310/1/{flag}/{b92:x}/{act}', 0x825310,
                    st(1) + blk() + [(0x810809, b(flag)), (0x70003B92, b(b92)), (A + 0x2EC, f(1.0)),
                                     (A + 0x2E8, f(40.0))],
                    {0x1B17A0: [act], 0x11E2A8: [K.O.fb(0.25)]})
    for ph in (3.13, 3.1315927, 3.14, -3.2, 0.0):
        add(f'5310/1/phase/{ph}', 0x825310, st(1) + blk() + [(A + 0x2EC, f(ph))], {0x11E2A8: [K.O.fb(-1.0)]})
    for ph in (ulp(3.1315927, -1), ulp(3.1315927, 1), ulp(3.1315927, 2)):
        add(f'5310/1/phaseulp/{ph.hex()}', 0x825310, st(1) + blk() + [(A + 0x2EC, ph)])
    for seed in (0, 0xFFFFFFFF, 0x80000000, 0x7FFF0000):
        add(f'5310/1/seed/{seed:x}', 0x825310, st(1) + blk(seed=seed) + [(0x810809, b(2)), (0x70003B92, b(1))])
    for phases in ((1.999, 1.9995, 1.998, 2.0), (2.0, 0.0, -1.0, 3.0), (1.99, 1.995, 1.9975, 1.999)):
        add(f'5310/1/wrap/{phases}', 0x825310, st(1) + blk(phases))
    for state in (2, 3, 4, 0xFF):
        add(f'5310/{state:x}', 0x825310, st(state) + blk())
    for n in range(K.O.reference_mode.pick(30, 4)):
        add(f'5310/random/{n}', 0x825310,
            st(rng.choice((0, 1, 1, 1, 3))) +
            blk(tuple(rng.uniform(-1, 2.1) for _ in range(4)), rng.randrange(1 << 32), rng.uniform(-100, 100),
                rng.uniform(-100, 100)) +
            [(0x810809, b(rng.choice((0, 2)))), (0x70003B92, b(rng.choice((0, 1)))), (A + 0x2EC, f(rng.uniform(-3.2, 3.2)))],
            {0x1B0FD0: [rng.choice((0, 1))], 0x1B17A0: [rng.choice((0, 1))],
             0x11E2A8: [K.O.fb(rng.uniform(-1, 1))],
             0x122BB8: [rng.randrange(-(1 << 31), 1 << 31) for _ in range(5)]})
