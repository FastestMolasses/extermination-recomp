#!/usr/bin/env python3
"""Generate the hook block of src/game/em_area21_creature_c.h and the wrappers
of src/game/em_area21_creature_c_internal.h from HOOKS below (the shape of
tools/gen_area17_creature.py; tools/test_area21_creature_c_reference.py reads
HOOKS for the oracle's register maps).

Usage (port root): python3 tools/gen_area21_creature_c.py
"""
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
# address: (arguments, result): u uint32_t, i int32_t, f float, q uint64_t.
# Word arguments travel in a0.., floats in f12.. (in order of each kind).
HOOKS = {
    0x001026A0: ('uuu', None), 0x001028B8: ('uuu', None), 0x00102948: ('uu', None), 0x00102958: ('uu', None),
    0x001029C0: ('u', None), 0x00102B08: ('uuf', None), 0x00102BB0: ('uuf', None), 0x001031E0: ('uu', None),
    0x00103230: ('uuf', None), 0x0011DE90: ('f', 'f'), 0x0011DF78: ('f', 'f'), 0x0011E0A8: ('uf', 'f'),
    0x0011E2A8: ('f', 'f'), 0x00122BB8: ('', 'i'), 0x001281C0: ('f', 'i'), 0x001283D0: ('uiu', 'i'),
    0x0014D1E0: ('uui', None), 0x0019A570: ('uuii', 'i'), 0x001A7B80: ('u', 'i'), 0x001AFA90: ('i', 'u'),
    0x001AFC10: ('u', None), 0x001B10B0: ('uii', 'i'), 0x001B1190: ('i', None), 0x001B1240: ('uff', 'f'),
    0x001B12B0: ('fff', 'f'), 0x001B1380: ('uuf', 'i'), 0x001B1470: ('f', 'f'), 0x001B1560: ('uuf', 'i'),
    0x001B15D0: ('uu', 'f'), 0x001B17A0: ('u', 'i'), 0x001B2B10: ('uuu', None), 0x001B4810: ('u', None),
    0x001B4CF0: ('u', None), 0x001B55E0: ('ui', None), 0x001C6160: ('u', 'i'), 0x001C63E0: ('ui', None),
    0x001C64F0: ('uf', 'i'), 0x001C67E0: ('uiff', None), 0x001C68C0: ('u', None), 0x001C94B0: ('uuuu', None),
    0x001EFD20: ('iu', None), 0x001EFD90: ('iuu', None), 0x001EFE00: ('iu', 'i'), 0x001F91C0: ('u', None),
    0x001FBD50: ('uiif', 'i'), 0x0021BE40: ('uu', 'i'),
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
    a = ['o->h->ctx'] + [f'a21c_float(a{k})' if t == 'f' else f'a{k}' for k, t in enumerate(args)]
    name = f'0x{address:08X}u'
    body = [f'static inline int a21c_c_{address:08X}(A21C *o{"".join(", " + x for x in p)}'
            + {None: ')', 'f': ', uint32_t *out)', 'i': ', int32_t *out)', 'u': ', uint32_t *out)'}[result],
            '{', '    if (a21c_failed(o)) return -1;',
            f'    if (!o->h->w_{address:08X}) return a21c_latch(o, {name}, EM_AREA21_CREATURE_C_FAULT_NULL_WORKER);']
    if result == 'f':
        body.append('    float r = 0;')
        a.append('&r')
    elif result:
        a.append('out')
    body.append(f'    if (o->h->w_{address:08X}({", ".join(a)}) < 0) '
                f'return a21c_latch(o, {name}, EM_AREA21_CREATURE_C_FAULT_WORKER_FAILED);')
    if result == 'f':
        body.append('    *out = a21c_bits(r);')
    body += ['    return 0;', '}', '']
    return '\n'.join(body)


def replace(path, begin, end, text):
    s = path.read_text()
    new = re.sub(re.escape(begin) + r'.*?' + re.escape(end), lambda _: begin + '\n' + text + end, s, flags=re.S)
    path.write_text(new)


def main():
    rows = sorted(HOOKS.items())
    replace(ROOT / 'src/game/em_area21_creature_c.h', '/* BEGIN GENERATED HOOKS */', '/* END GENERATED HOOKS */',
            ''.join(hook_line(a, *v) + '\n' for a, v in rows))
    replace(ROOT / 'src/game/em_area21_creature_c_internal.h', '/* BEGIN GENERATED WRAPPERS */',
            '/* END GENERATED WRAPPERS */', '\n'.join(wrapper(a, *v) for a, v in rows))


if __name__ == '__main__':
    main()
