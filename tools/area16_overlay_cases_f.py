"""AREA16 overlay oracle cases, group f (tools/test_area16_overlay_reference.py):
the five effect behaviours of AREA16's effect ids 0..4, 0x8237A0, 0x8239B0,
0x823D20, 0x823F70, 0x824110 (src/game/em_area16_overlay_f.c)."""
import os
import random

S_ = 's'
SOURCE = 'em_area16_overlay_f.c'
FUNCS = {
    0x8237A0: ('em_area16_overlay_008237A0', 0x208, ['u', S_], None),
    0x8239B0: ('em_area16_overlay_008239B0', 0x370, ['u', S_], None),
    0x823D20: ('em_area16_overlay_00823D20', 0x244, ['u', S_], None),
    0x823F70: ('em_area16_overlay_00823F70', 0x1A0, ['u', S_], None),
    0x824110: ('em_area16_overlay_00824110', 0x308, ['u', S_], None),
}
DEAD = {}
SELF_ENTRIES = set(FUNCS)
FULL = os.environ.get('EM_TEST_FULL') == '1'
RANDS = (0, 1, 12345, 0x40000000, 0x7FFFFFFF, -0x80000000, -1, -12345)
SEEDS = (0, 1, 0x10000, 0x7FFF0000, 0x80000000, 0xFFFFFFFF, 0x12345678, 0xDEADBEEF)


def cases(add, K):
    b, w, f, fw, ulp = K.b, K.w, K.f, K.fw, K.ulp
    A = K.A
    BLK = A + 0x1F0

    def st(state, step=None):
        return [(A + 4, b(state))] + ([(A + 5, b(step))] if step is not None else [])

    # a placed node: position +0xB0, rotation +0xC0, a position +0x100
    def placed():
        return [(A + 0xB0, fw(318.5, 176.25, 486.0, 1.0)), (A + 0xC0, fw(0.25, -1.5, 3.0, 0.0)),
                (A + 0x100, fw(300.0, 180.0, 430.0, 1.0))]

    rng = random.Random(0xA16F)

    # ---- 0x8237A0 (effect 0)
    for i, (r1, r2) in enumerate(zip(RANDS, RANDS[1:] + RANDS[:1])):
        add(f'37a0/0/{i}', 0x8237A0, st(0) + placed(), {0x122BB8: [r1, r2]})
    for v in (0.0, 0.5, 1.0, 1.07, 1.08, 1.1, 2.0, -0.5, 1e-30, -3.0):
        add(f'37a0/1/{v}', 0x8237A0, st(1) + placed() + [(BLK, fw(v, 0.25, 0.75))])
    for v in (ulp(1.08, -1), ulp(1.08, 1), ulp(1.1, -1), ulp(1.1, 1)):
        add(f'37a0/1/ulp/{v.hex()}', 0x8237A0, st(1) + placed() + [(BLK, v)])
    for s in (2, 3, 4, 0x63, 0xFF):
        add(f'37a0/s{s:x}', 0x8237A0, st(s) + placed() + [(BLK, fw(0.5))])

    # ---- 0x8239B0 (effect 1)
    for i, r in enumerate(RANDS):
        add(f'39b0/0/{i}', 0x8239B0, st(0, 7) + placed(), {0x122BB8: [r], 0x1CCF70: [3, 4]})
    for v in (0.0, 0.5, 0.96, 0.97, 1.0, 2.0, -1.0):
        for step in (0, 0xFF):
            add(f'39b0/1/0/{v}/{step}', 0x8239B0, st(1, 0) + placed() + [(BLK, fw(0.3, v, 0.0, 0x12345678))],
                {0x1CCF70: [-1, 0x55]})
    for v in (ulp(0.96666664, -1), ulp(0.96666664, 1)):
        add(f'39b0/1/0/ulp/{v.hex()}', 0x8239B0, st(1, 0) + placed() + [(BLK, fw(0.3)), (BLK + 4, v)])
    for a2 in (0.0, 3.48, 3.49, 3.5, 4.0):
        for a1 in (1.0, 0.02, 0.0166, 0.0167, 0.0):
            add(f'39b0/1/1/{a2}/{a1}', 0x8239B0, st(1, 1) + placed() + [(BLK, fw(1.5, a1, a2, 0xDEADBEEF))])
    for v in (ulp(3.49, -1), ulp(3.49, 1)):
        add(f'39b0/1/1/ulp/{v.hex()}', 0x8239B0, st(1, 1) + placed() + [(BLK, fw(1.5, 0.5)), (BLK + 8, v)])
    for v in (ulp(0.016666668, -1), ulp(0.016666668, 1), K.w(0x3C888889)):
        add(f'39b0/1/1/fade/{v.hex()}', 0x8239B0, st(1, 1) + placed() + [(BLK, fw(1.5)), (BLK + 4, v),
                                                                        (BLK + 8, fw(3.6))])
    for a0 in (1.99, 1.993, 2.0, 1.0, -5.0):
        for seed in SEEDS:
            add(f'39b0/1/wrap/{a0}/{seed:x}', 0x8239B0, st(1, 2) + placed() + [(BLK, fw(a0, 0.5, 1.0, seed))])
    for v in (ulp(1.993, -1), ulp(1.993, 1)):
        add(f'39b0/1/wrapulp/{v.hex()}', 0x8239B0, st(1, 2) + placed() + [(BLK, v)])
    for s in (2, 3, 4, 0xFF):
        add(f'39b0/s{s:x}', 0x8239B0, st(s, 0) + placed())

    # ---- 0x823D20 (effect 2)
    for i in range(3 if not FULL else 8):
        rs = [rng.choice(RANDS) if k % 3 else rng.randrange(-2**31, 2**31) for k in range(16)]
        add(f'3d20/0/{i}', 0x823D20, st(0) + placed(), {0x122BB8: rs})
    pattern = (0.4, 0.6, 1.49, 1.5, 1.6, 0.0, -1.0, 1.485, 1.4851, 2.0, 0.5, 1.0, 1.2, 1.3, 1.45)
    for seed in SEEDS:
        add(f'3d20/1/mix/{seed:x}', 0x823D20, st(1) + placed() + [(BLK, fw(*pattern)), (BLK + 0x3C, w(seed))])
    add('3d20/1/done', 0x823D20, st(1) + [(BLK, fw(*([1.5] * 15))), (BLK + 0x3C, w(7))])
    add('3d20/1/one', 0x823D20, st(1) + [(BLK, fw(*([1.5] * 14 + [1.49]))), (BLK + 0x3C, w(7))])
    add('3d20/1/ulp', 0x823D20, st(1) + [(BLK, ulp(1.5, -1) + fw(*([1.5] * 14))), (BLK + 0x3C, w(9))])
    for i in range(2 if not FULL else 10):
        vals = [rng.uniform(0.3, 1.6) for _ in range(15)]
        add(f'3d20/1/rand/{i}', 0x823D20, st(1) + [(BLK, fw(*vals)), (BLK + 0x3C, w(rng.getrandbits(32)))])
    for s in (2, 3, 4, 0xFF):
        add(f'3d20/s{s:x}', 0x823D20, st(s))

    # ---- 0x823F70 (effect 3)
    for i, r in enumerate(RANDS):
        add(f'3f70/0/{i}', 0x823F70, st(0) + placed(), {0x122BB8: [r]})
    for v in (0.0, 1.0, 1.17, 1.18, 1.2, 1.5, -1.0):
        for seed in SEEDS[:4] if not FULL else SEEDS:
            add(f'3f70/1/{v}/{seed:x}', 0x823F70, st(1) + placed() + [(BLK, fw(v, seed))])
    for v in (ulp(1.18, -1), ulp(1.18, 1)):
        add(f'3f70/1/ulp/{v.hex()}', 0x823F70, st(1) + placed() + [(BLK, v), (BLK + 4, w(5))])
    for s in (2, 3, 4, 0xFF):
        add(f'3f70/s{s:x}', 0x823F70, st(s))

    # ---- 0x824110 (effect 4)
    for i, r in enumerate(RANDS):
        add(f'4110/0/{i}', 0x824110, st(0, 9) + placed(), {0x122BB8: [r]})
    for step in (0, 1, 2, 3, 0x63, 4, 0xFF):
        for a1 in (0.0, 0.5, 0.8, 0.81, 0.97, 1.0, 0.03, 0.1, 0.09, -0.5):
            for a0 in (0.0, 1.5) if step in (1, 3) else (1.5,):
                add(f'4110/1/{step:x}/{a1}/{a0}', 0x824110,
                    st(1, step) + placed() + [(BLK, fw(a0, a1, 0.375))], {0x1CCF70: [0x21]})
    for v in (ulp(0.8, -1), ulp(0.8, 1), ulp(0.1, -1), ulp(0.1, 1), ulp(0.033333335, -1),
              ulp(0.033333335, 1), K.w(0x3D088889), K.w(0x80000000)):
        for step in (1, 2, 3, 0x63):
            add(f'4110/1/ulp/{step:x}/{v.hex()}', 0x824110, st(1, step) + placed() + [(BLK, fw(1.0)), (BLK + 4, v)])
    for a0 in (1.98, 1.985, 1.99, 2.0, -0.5):
        add(f'4110/1/wrap/{a0}', 0x824110, st(1, 0) + placed() + [(BLK, fw(a0, 0.5, 0.25))])
    for v in (ulp(1.985, -1), ulp(1.985, 1)):
        add(f'4110/1/wrapulp/{v.hex()}', 0x824110, st(1, 0) + placed() + [(BLK, v), (BLK + 4, fw(0.5))])
    for s in (2, 3, 4, 0xFF):
        add(f'4110/s{s:x}', 0x824110, st(s, 1) + placed())
