"""AREA16 overlay oracle cases, group c (tools/test_area16_overlay_reference.py):
0x825800, 0x826A50, 0x826C20 (src/game/em_area16_overlay_c.c)."""
import random
import struct

S_ = 's'
SOURCE = 'em_area16_overlay_c.c'
FUNCS = {
    0x825800: ('em_area16_overlay_00825800', 0x998, ['u', S_], None),
    0x826A50: ('em_area16_overlay_00826A50', 0x1CC, ['u', 'u', 'f', S_], None),
    0x826C20: ('em_area16_overlay_00826C20', 0x2C, ['u', 'u', 'u'], 'i'),
}
DEAD = {}
SELF_ENTRIES = {0x825800}


def bits(x):
    return struct.unpack('<I', struct.pack('<f', x))[0]


def cases(add, K):
    b, h, w, f, fw = K.b, K.h, K.w, K.f, K.fw
    A = K.A
    N0, N1, N2 = K.SPAWN, K.SPAWN + 0x400, K.SPAWN + 0x800
    POS = K.REC

    # the three nodes 0x826A50 reads (D_008103DC, D_008103CC, D_008103C0)
    nodes = [(0x8103DC, w(N0)), (0x8103CC, w(N1)), (0x8103C0, w(N2)),
             (N0 + 0xC0, f(300.5)), (N0 + 0xC4, f(201.25)),
             (N1 + 0xC0, f(310.0)), (N1 + 0xC4, f(-7.75)),
             (N2 + 0xC0, f(296.0)), (N2 + 0xC4, f(3.5))]

    def st(state, step=None):
        return [(A + 4, b(state))] + ([(A + 5, b(step))] if step is not None else [])

    def player(x=300.0, y=150.0, z=440.0):
        return [(0x810350, fw(x, y, z))]

    # ---- 0x826A50: the speed thresholds
    for label, sb in (('0', 0), ('m0', 0x80000000), ('half', bits(0.5)), ('half-', bits(0.5) - 1),
                      ('one', bits(1.0)), ('tiny', 1), ('mhalf', bits(-0.5)), ('mhalf+', bits(-0.5) - 1),
                      ('mone', bits(-1.0)), ('big', bits(1e30)), ('mbig', bits(-1e30))):
        add(f'6a50/{label}', 0x826A50, nodes + [(POS, fw(1.0, 2.0, 471.5, 4.0))], {},
            args=[A, POS, sb, K.O.STACK_TOP])

    # ---- 0x826C20
    add('6c20/sound', 0x826C20)

    # ---- 0x825800 state 0
    for r in (0, 1, -1):
        for done in (0, 0xFF, 0xFE):
            add(f'5800/0/{r}/{done:x}', 0x825800,
                st(0) + player(1.0, 2.0, 463.25) + [(0x81078B, b(done)), (A + 0x2E4, f(5.0))],
                {0x1B0FD0: [r], 0x1B6660: [N0]})

    # ---- 0x825800 state 1: the sound, the splash, the height moves
    for ev in range(8):
        add(f'5800/1/ev{ev}', 0x825800, st(1) + nodes + player(300.0, 300.0, 440.0) +
            [(0x81080B, b(ev)), (A + 0x2EC, f(440.0))])
    # the splash guard: +0x2EC against 459, the height against 459
    for e2ec in (459.0, 'below', 470.0, 0.0):
        for z in (459.0, 'above', 300.0):
            pe = (A + 0x2EC, w(bits(459.0) - 1)) if e2ec == 'below' else (A + 0x2EC, f(e2ec))
            pz = w(bits(459.0) + 1) if z == 'above' else f(z)
            for spd in (2.0, 0.1):
                add(f'5800/1/splash/{e2ec}/{z}/{spd}', 0x825800, st(1) + nodes + player(300.0, 300.0, 0.0) +
                    [(0x810358, pz), pe, (A + 0x2E4, f(spd)), (0x81080B, b(0))])
    # the height bands (y < 205, height in (400, 430), [430, 466), >= 466)
    ys = (205.0, 'below205', 100.0)
    zs = (400.0, 'above400', 420.0, 430.0, 'below430', 450.0, 'below466', 466.0, 470.0)
    for y in ys:
        py = w(bits(205.0) - 1) if y == 'below205' else f(y)
        for z in zs:
            pz = {'above400': w(bits(400.0) + 1), 'below430': w(bits(430.0) - 1),
                  'below466': w(bits(466.0) - 1)}.get(z) or f(z)
            for mode in (0, 1, 2, 3):
                add(f'5800/1/band/{y}/{z}/{mode}', 0x825800, st(1) + nodes +
                    [(0x810350, f(305.0)), (0x810354, py), (0x810358, pz), (0x8102B5, b(mode)),
                     (A + 0x2E4, f(-0.25)), (A + 0x2EC, f(300.0)), (0x81080B, b(0))])
    # x pulled back into [297, 311]
    for x in (311.0, 'above311', 320.0, 297.0, 'below297', 290.0, 'x311b', 297.05):
        px = {'above311': w(bits(311.0) + 1), 'below297': w(bits(297.0) - 1),
              'x311b': w(bits(311.1))}.get(x) or f(x)
        add(f'5800/1/x/{x}', 0x825800, st(1) + nodes +
            [(0x810350, px), (0x810354, f(100.0)), (0x810358, f(450.0)), (0x8102B5, b(1)),
             (A + 0x2E4, f(0.3)), (A + 0x2EC, f(300.0)), (0x81080B, b(0))])
    # the event: state 0xC8 (bit 2, sound when bits & 7 == 3, height 475)
    for ev in (3, 4, 7, 0x0B, 0xFB):
        for z in (474.0, 'below475', 475.0, 480.0):
            pz = w(bits(475.0) - 1) if z == 'below475' else f(z)
            add(f'5800/1/c8/{ev:x}/{z}', 0x825800, st(1) + nodes + player(300.0, 300.0, 0.0) +
                [(0x810358, pz), (0x81080B, b(ev)), (A + 0x2EC, f(300.0)), (A + 5, b(9)),
                 (A + 0x28, h(0x1234))])
    # state 0xC9: height below 455 and a downward speed
    for z in (455.0, 'below455', 300.0):
        pz = w(bits(455.0) - 1) if z == 'below455' else f(z)
        for spd in ('m055', 'm055-', -0.06, -1.0, 'm135', 'm135-', -2.0, 0.0):
            ps = {'m055': w(bits(-0.055)), 'm055-': w(bits(-0.055) + 1), 'm135': w(bits(-1.35)),
                  'm135-': w(bits(-1.35) + 1)}.get(spd) or f(spd)
            for mode in (0, 1, 2):
                add(f'5800/1/c9/{z}/{spd}/{mode}', 0x825800, st(1) + nodes + player(300.0, 300.0, 0.0) +
                    [(0x810358, pz), (A + 0x2E4, ps), (0x8102B5, b(mode)), (0x81080B, b(0x10)),
                     (A + 0x2EC, f(300.0))])

    # ---- 0x825800 state 0xC9
    for z in (472.0, 'below472', 465.0, 459.0, 'above459', 458.5, 458.0, 'below458', 400.0):
        pz = {'below472': w(bits(472.0) - 1), 'above459': w(bits(459.0) + 1),
              'below458': w(bits(458.0) - 1)}.get(z) or f(z)
        for done in (0, 1):
            add(f'5800/c9/{z}/{done}', 0x825800, st(0xC9) + nodes +
                [(0x810350, fw(301.0, 150.0)), (0x810358, pz), (0x810360, fw(1.5, 2.5, 3.5))],
                {0x1BA1F0: [done]})

    # ---- 0x825800 state 0xC8
    for z in (480.0, 'below480', 470.0):
        pz = w(bits(480.0) - 1) if z == 'below480' else f(z)
        for done in (0, 1):
            for count in (0, 77, 78, 79, 0x7FFF, -1, -100):
                add(f'5800/c8/0/{z}/{done}/{count}', 0x825800, st(0xC8, 0) +
                    [(0x810358, pz), (0x810360, fw(1.5, 2.5, 3.5)), (A + 0x28, h(count & 0xFFFF)),
                     (A + 0x36, h(7))],
                    {0x1BA1F0: [done]})
    for half in (0, 1, 0x8000):
        add(f'5800/c8/1/{half:x}', 0x825800, st(0xC8, 1) + [(A + 0x36, h(half)), (0x81080B, b(0x13)),
                                                             (0x81078B, b(1))],
            {0x1B6660: [N0]})
    for step in (2, 0xFF):
        add(f'5800/c8/{step:x}', 0x825800, st(0xC8, step))

    # ---- 0x825800 state 2: the effects every 8th frame while +0x2DC counts down
    for cnt in (0, -1, 1, 3, 4, 5, 0x7FFFFFFF):
        for frame in (0, 8, 3, -8, -3, -16, 0x7FFFFFFF):
            add(f'5800/2/{cnt}/{frame}', 0x825800, st(2) +
                [(A + 0x2DC, w(cnt & 0xFFFFFFFF)), (A + 0x2E0, f(452.0)), (0x70003B68, w(frame & 0xFFFFFFFF))])

    for state in (3, 4, 0xC7, 0xCA, 0xFF):
        add(f'5800/{state:x}', 0x825800, st(state))

    rng = random.Random(0xA16C)
    for n in range(K.O.reference_mode.pick(40, 6)):
        state = rng.choice((1, 1, 1, 0xC9, 0xC8, 2))
        add(f'5800/random/{n}', 0x825800, st(state, rng.choice((0, 1))) + nodes +
            [(0x810350, fw(rng.uniform(280, 330), rng.uniform(150, 260), rng.uniform(380, 490))),
             (0x810360, fw(rng.uniform(-50, 50), rng.uniform(-50, 50), rng.uniform(-50, 50))),
             (0x8102B5, b(rng.choice((0, 1, 2)))), (0x81080B, b(rng.randrange(256))),
             (A + 0x2E4, f(rng.uniform(-3, 3))), (A + 0x2EC, f(rng.uniform(380, 490))),
             (A + 0x28, h(rng.randrange(75, 82))), (A + 0x36, h(rng.choice((0, 1)))),
             (A + 0x2DC, w(rng.randrange(-1, 7) & 0xFFFFFFFF)), (A + 0x2E0, f(rng.uniform(400, 460))),
             (0x70003B68, w(rng.randrange(1 << 32)))],
            {0x1BA1F0: [rng.choice((0, 1))], 0x1B6660: [N0]})
