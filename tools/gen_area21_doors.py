#!/usr/bin/env python3
"""Generate the hook block of src/game/em_area21_doors.h and the wrappers
of src/game/em_area21_doors_internal.h from HOOKS below (the shape of
tools/gen_area14_overlay.py; tools/test_area21_doors_reference.py reads
HOOKS for the oracle's register maps). HOOKS is every callee of the AREA21
functions em_area21_doors translates, with the result kind only where an
owner uses the result.

Usage (port root): python3 tools/gen_area21_doors.py
"""
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
# address: (arguments, result): u uint32_t, i int32_t, f float, q uint64_t.
# Word arguments travel in a0.. (then t0.., the EABI's a4..), floats in
# f12.. (in order of each kind).
HOOKS = {
    # the doors 0x826BB0 / 0x826E60
    0x001BBDA0: ('u', None), 0x001BBE40: ('uui', 'i'), 0x001BC0E0: ('uu', 'i'), 0x001BC240: ('uu', None),
    0x001BC290: ('uu', 'i'), 0x001BC300: ('u', None), 0x001BA1A0: ('uu', None), 0x001AFC10: ('u', None),
    # 0x82A1D0
    0x001B0FD0: ('u', 'i'), 0x001C6380: ('u', None), 0x0011DF78: ('f', 'f'), 0x00122BB8: ('', 'i'),
    0x001A2370: ('uu', None), 0x001B17A0: ('u', 'i'),
    # the rounds 0x825C70
    0x00102948: ('uu', None), 0x00102760: ('uu', None), 0x001CD390: ('uu', None), 0x00103230: ('uuf', None),
    0x001CEEE0: ('uuuifff', None), 0x0019A570: ('uuii', 'i'), 0x001B41F0: ('uuuiii', None),
    0x001B1470: ('f', 'f'), 0x001EFD90: ('uuu', None),
    # 0x8273C0, 0x827880, 0x827A30, 0x827E70, 0x828350
    0x001FC3C0: ('uuiff', None), 0x001B1B70: ('u', None), 0x001EFD20: ('uu', None), 0x001F02C0: ('uif', None),
    0x001028B8: ('uuu', None), 0x001C6120: ('ii', 'i'), 0x001CA6E0: ('ui', None), 0x001C62C0: ('u', None),
    0x0015AC00: ('uu', None), 0x0015AE20: ('uu', None), 0x001B1190: ('i', None),
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
    a = ['o->h->ctx'] + [f'a21d_float(a{k})' if t == 'f' else f'a{k}' for k, t in enumerate(args)]
    name = f'0x{address:08X}u'
    body = [f'static inline int a21d_c_{address:08X}(A21D *o{"".join(", " + x for x in p)}'
            + {None: ')', 'f': ', uint32_t *out)', 'i': ', int32_t *out)', 'u': ', uint32_t *out)'}[result],
            '{', '    if (a21d_failed(o)) return -1;',
            f'    if (!o->h->w_{address:08X}) return a21d_latch(o, {name}, EM_AREA21_DOORS_FAULT_NULL_WORKER);']
    if result == 'f':
        body.append('    float r = 0;')
        a.append('&r')
    elif result:
        a.append('out')
    body.append(f'    if (o->h->w_{address:08X}({", ".join(a)}) < 0) '
                f'return a21d_latch(o, {name}, EM_AREA21_DOORS_FAULT_WORKER_FAILED);')
    if result == 'f':
        body.append('    *out = a21d_bits(r);')
    body += ['    return 0;', '}', '']
    return '\n'.join(body)


def replace(path, begin, end, text):
    s = path.read_text()
    new = re.sub(re.escape(begin) + r'.*?' + re.escape(end), lambda _: begin + '\n' + text + end, s, flags=re.S)
    path.write_text(new)


def main():
    rows = sorted(HOOKS.items())
    replace(ROOT / 'src/game/em_area21_doors.h', '/* BEGIN GENERATED HOOKS */', '/* END GENERATED HOOKS */',
            ''.join(hook_line(a, *v) + '\n' for a, v in rows))
    replace(ROOT / 'src/game/em_area21_doors_internal.h', '/* BEGIN GENERATED WRAPPERS */',
            '/* END GENERATED WRAPPERS */', '\n'.join(wrapper(a, *v) for a, v in rows))


if __name__ == '__main__':
    main()
