#!/usr/bin/env python3
"""Generate the hook block of src/game/em_area21_overlay.h and the wrappers
of src/game/em_area21_overlay_internal.h from HOOKS below (the shape of
tools/gen_area21_doors.py; tools/test_area21_overlay_reference.py reads
HOOKS for the oracle's register maps). HOOKS is every callee of the AREA21
functions em_area21_overlay translates (the Gatling [62] 0x8297C0 and the
cannon [61] 0x828700 with their overlay callees), with the result kind only
where a translation uses the result.

Usage (port root): python3 tools/gen_area21_overlay.py
"""
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
# address: (arguments, result): u uint32_t, i int32_t, f float, q uint64_t.
# Word arguments travel in a0.. (then t0.., the EABI's a4..), floats in
# f12.. (in order of each kind).
HOOKS = {
    # the Gatling 0x8297C0 and its callees
    0x001B0FD0: ('u', 'i'), 0x001C6380: ('u', None), 0x001C5570: ('uuii', 'u'), 0x001A2370: ('uu', None),
    0x001B1470: ('f', 'f'), 0x001B1E20: ('ii', None), 0x001FB9F0: ('iiii', None), 0x001EFD90: ('iuu', None),
    0x001F4F40: ('i', 'u'), 0x00102948: ('uu', None), 0x00102958: ('uu', None), 0x001029C0: ('u', None),
    0x00102BB0: ('uuf', None), 0x00102B08: ('uuf', None), 0x001026D0: ('uuu', None), 0x001026A0: ('uuu', None),
    0x001F4010: ('iu', None), 0x0011DE90: ('f', 'f'), 0x001B17A0: ('u', None), 0x001F5940: ('iui', None),
    0x001AFC10: ('u', None),
    # 0x829310
    0x001B5DC0: ('i', 'i'), 0x0011DF78: ('f', 'f'),
    # 0x8294C0, 0x829590 / 0x828410
    0x00182F90: ('uu', None), 0x001031E0: ('uu', None), 0x001028D0: ('uuu', None), 0x00102760: ('uu', None),
    # 0x8260A0
    0x001D1F80: ('iii', None), 0x00207F80: ('iiiiiu', None), 0x001281C0: ('f', 'i'),
    # 0x829F10 / 0x829F40
    0x00122BB8: ('', 'i'), 0x00102918: ('uuu', None), 0x001EFEB0: ('uu', 'u'), 0x001CCF70: ('u', 'i'),
    0x001CFB50: ('uiufffff', None), 0x001CFBE0: ('iiuui', None),
    # the cannon's 0x828E80 and its projectile 0x828F30
    0x001AFA90: ('i', 'u'), 0x00103230: ('uuf', None), 0x001EFE00: ('uu', 'u'), 0x0019A570: ('uuii', 'i'),
    0x001B41F0: ('uuuiii', None), 0x001FBD50: ('uiif', None), 0x001C94B0: ('uuuu', None),
    0x001EFD20: ('uu', None),
}
TYPES = {'u': 'uint32_t', 'i': 'int32_t', 'f': 'float', 'q': 'uint64_t'}


def params(args, result):
    out = [f'{TYPES[t]} a{k}' for k, t in enumerate(args)]
    if result:
        out.append(f'{TYPES[result]} *result')
    return out


def hook_line(address, args, result):
    return f'    int (*w_{address:08X})(void *ctx{"".join(", " + p for p in params(args, result))});'


def wrapper(address, args, result):
    # float arguments and results travel as their bits in the module
    p = [f'{"uint32_t" if t == "f" else TYPES[t]} a{k}' for k, t in enumerate(args)]
    a = ['o->h->ctx'] + [f'a21g_float(a{k})' if t == 'f' else f'a{k}' for k, t in enumerate(args)]
    name = f'0x{address:08X}u'
    body = [f'static inline int a21g_c_{address:08X}(A21G *o{"".join(", " + x for x in p)}'
            + {None: ')', 'f': ', uint32_t *out)', 'i': ', int32_t *out)', 'u': ', uint32_t *out)'}[result],
            '{', '    if (a21g_failed(o)) return -1;',
            f'    if (!o->h->w_{address:08X}) return a21g_latch(o, {name}, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);']
    if result == 'f':
        body.append('    float r = 0;')
        a.append('&r')
    elif result:
        a.append('out')
    body.append(f'    if (o->h->w_{address:08X}({", ".join(a)}) < 0) '
                f'return a21g_latch(o, {name}, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);')
    if result == 'f':
        body.append('    *out = a21g_bits(r);')
    body += ['    return 0;', '}', '']
    return '\n'.join(body)


def replace(path, begin, end, text):
    s = path.read_text()
    new = re.sub(re.escape(begin) + r'.*?' + re.escape(end), lambda _: begin + '\n' + text + end, s, flags=re.S)
    path.write_text(new)


def main():
    rows = sorted(HOOKS.items())
    replace(ROOT / 'src/game/em_area21_overlay.h', '/* BEGIN GENERATED HOOKS */', '/* END GENERATED HOOKS */',
            ''.join(hook_line(a, *v) + '\n' for a, v in rows))
    replace(ROOT / 'src/game/em_area21_overlay_internal.h', '/* BEGIN GENERATED WRAPPERS */',
            '/* END GENERATED WRAPPERS */', '\n'.join(wrapper(a, *v) for a, v in rows))


if __name__ == '__main__':
    main()
