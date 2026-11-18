#!/usr/bin/env python3
"""Generate the hook block of src/game/em_area20_overlay.h and the
wrappers of src/game/em_area20_overlay_internal.h from HOOKS below (the
shape of tools/gen_area14_overlay.py; tools/test_area20_overlay_reference.py
reads HOOKS for the oracle's register maps). HOOKS is every callee of the
AREA20 functions em_area20_overlay translates, with the result kind
only where an owner uses the result.

Usage (port root): python3 tools/gen_area20_overlay.py
"""
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
# address: (arguments, result): u uint32_t, i int32_t, f float, q uint64_t.
# Word arguments travel in a0.., floats in f12.. (in order of each kind).
HOOKS = {
    # the vector / matrix and float helpers the oracle runs as original
    # code (test_area20_overlay_reference's MATH)
    0x00102948: ('uu', None), 0x001029C0: ('u', None), 0x00102B08: ('uuf', None), 0x00102C58: ('uuu', None),
    0x00102918: ('uuu', None), 0x0011DE90: ('f', 'f'), 0x0011E2A8: ('f', 'f'), 0x001B1470: ('f', 'f'),
    0x00122BB8: ('', 'i'), 0x0019A570: ('uuii', 'i'), 0x0019AB20: ('uuui', 'i'), 0x0019AD00: ('uui', 'i'),
    0x001A2370: ('uu', None), 0x001AFC10: ('u', None), 0x001B0C60: ('iii', None), 0x001B0D80: ('u', None),
    0x001B0FD0: ('u', 'i'), 0x001B10B0: ('uii', None), 0x001B17A0: ('u', None), 0x001B1B70: ('u', None),
    0x001B1EA0: ('iuui', 'i'), 0x001BA1A0: ('uu', None), 0x001BA1C0: ('ui', 'i'), 0x001BA1F0: ('u', 'i'),
    0x001BA540: ('u', None), 0x001BA580: ('ui', None), 0x001BA8E0: ('ui', None), 0x001C6190: ('u', 'i'),
    0x001C6380: ('u', None), 0x001C63E0: ('ui', None), 0x001C64F0: ('uf', 'i'), 0x001C68C0: ('u', None),
    0x001CA6F0: ('ui', None), 0x001D04B0: ('uiuff', None), 0x001EFD20: ('iu', None), 0x001EFEB0: ('iu', 'u'),
    0x001F0460: ('iu', None), 0x001F9100: ('uuuf', None), 0x001FABB0: ('', None), 0x001FB0B0: ('i', None),
    0x001FC580: ('ui', None),
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
    a = ['o->h->ctx'] + [f'a20_float(a{k})' if t == 'f' else f'a{k}' for k, t in enumerate(args)]
    name = f'0x{address:08X}u'
    body = [f'static inline int a20_c_{address:08X}(A20 *o{"".join(", " + x for x in p)}'
            + {None: ')', 'f': ', uint32_t *out)', 'i': ', int32_t *out)', 'u': ', uint32_t *out)'}[result],
            '{', '    if (a20_failed(o)) return -1;',
            f'    if (!o->h->w_{address:08X}) return a20_latch(o, {name}, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);']
    if result == 'f':
        body.append('    float r = 0;')
        a.append('&r')
    elif result:
        a.append('out')
    body.append(f'    if (o->h->w_{address:08X}({", ".join(a)}) < 0) '
                f'return a20_latch(o, {name}, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);')
    if result == 'f':
        body.append('    *out = a20_bits(r);')
    body += ['    return 0;', '}', '']
    return '\n'.join(body)


def replace(path, begin, end, text):
    s = path.read_text()
    new = re.sub(re.escape(begin) + r'.*?' + re.escape(end), lambda _: begin + '\n' + text + end, s, flags=re.S)
    path.write_text(new)


def main():
    rows = sorted(HOOKS.items())
    replace(ROOT / 'src/game/em_area20_overlay.h', '/* BEGIN GENERATED HOOKS */', '/* END GENERATED HOOKS */',
            ''.join(hook_line(a, *v) + '\n' for a, v in rows))
    replace(ROOT / 'src/game/em_area20_overlay_internal.h', '/* BEGIN GENERATED WRAPPERS */',
            '/* END GENERATED WRAPPERS */', '\n'.join(wrapper(a, *v) for a, v in rows))


if __name__ == '__main__':
    main()
