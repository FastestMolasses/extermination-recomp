#!/usr/bin/env python3
"""Generate the hook block of src/game/em_area21_groups.h and the wrappers
of src/game/em_area21_groups_internal.h from HOOKS below (the shape of
tools/gen_area14_overlay.py; tools/test_area21_groups_reference.py reads
HOOKS for the oracle's register maps). HOOKS is every callee of the AREA21
functions em_area21_groups translates, with the result kind only where an
owner uses the result.

Usage (port root): python3 tools/gen_area21_groups.py
"""
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
# address: (arguments, result): u uint32_t, i int32_t, f float, q uint64_t.
# Word arguments travel in a0.. (then t0.., the EABI's a4..), floats in
# f12.. (in order of each kind).
HOOKS = {
    # the group 0x82B500 / 0x82BA70 members 0x823580, 0x8237B0, 0x823860, 0x8238E0
    0x00102948: ('uu', None), 0x0011DE90: ('f', 'f'), 0x00122BB8: ('', 'i'), 0x001C6190: ('u', 'i'),
    0x001CD520: ('iiuqufff', None), 0x001EFD20: ('uu', 'u'),
    # 0x824CF0 (a boss-fight effect node, g77)
    0x001029C0: ('u', None), 0x00102C58: ('uuu', None), 0x00102918: ('uuu', None), 0x001AFC10: ('u', None),
    0x001CCF70: ('u', 'i'), 0x001CFB50: ('uiufffff', None), 0x001CFBE0: ('iiuui', None),
    # 0x8243B0 (placement [52]; 001281C0 is the float -> unsigned helper)
    0x001281C0: ('f', 'i'), 0x0021B9A0: ('iff', None),
    # 0x824FE0 (the node of [43] / [44]'s deferred spawn: 001EFD20's result is the effect node)
    0x001026A0: ('uuu', None), 0x00102BB0: ('uuf', None),
    # 0x825B00 (the effect node 0014EC00's 001EFD90 starts, g77 row 5893)
    0x001CFAE0: ('uiuffff', None),
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
    a = ['o->h->ctx'] + [f'a21p_float(a{k})' if t == 'f' else f'a{k}' for k, t in enumerate(args)]
    name = f'0x{address:08X}u'
    body = [f'static inline int a21p_c_{address:08X}(A21P *o{"".join(", " + x for x in p)}'
            + {None: ')', 'f': ', uint32_t *out)', 'i': ', int32_t *out)', 'u': ', uint32_t *out)'}[result],
            '{', '    if (a21p_failed(o)) return -1;',
            f'    if (!o->h->w_{address:08X}) return a21p_latch(o, {name}, EM_AREA21_GROUPS_FAULT_NULL_WORKER);']
    if result == 'f':
        body.append('    float r = 0;')
        a.append('&r')
    elif result:
        a.append('out')
    body.append(f'    if (o->h->w_{address:08X}({", ".join(a)}) < 0) '
                f'return a21p_latch(o, {name}, EM_AREA21_GROUPS_FAULT_WORKER_FAILED);')
    if result == 'f':
        body.append('    *out = a21p_bits(r);')
    body += ['    return 0;', '}', '']
    return '\n'.join(body)


def replace(path, begin, end, text):
    s = path.read_text()
    new = re.sub(re.escape(begin) + r'.*?' + re.escape(end), lambda _: begin + '\n' + text + end, s, flags=re.S)
    path.write_text(new)


def main():
    rows = sorted(HOOKS.items())
    replace(ROOT / 'src/game/em_area21_groups.h', '/* BEGIN GENERATED HOOKS */', '/* END GENERATED HOOKS */',
            ''.join(hook_line(a, *v) + '\n' for a, v in rows))
    replace(ROOT / 'src/game/em_area21_groups_internal.h', '/* BEGIN GENERATED WRAPPERS */',
            '/* END GENERATED WRAPPERS */', '\n'.join(wrapper(a, *v) for a, v in rows))


if __name__ == '__main__':
    main()
