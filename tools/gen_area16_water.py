#!/usr/bin/env python3
"""Generate the hook block of src/game/em_area16_water.h and the wrappers of
src/game/em_area16_water_internal.h from HOOKS below (the shape of
tools/gen_area17_creature.py; tools/test_area16_water_reference.py reads
HOOKS for the oracle's register maps).

Usage (port root): python3 tools/gen_area16_water.py
"""
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
# address: (arguments, result): u uint32_t, i int32_t, f float
HOOKS = {
    0x001026A0: ('uuu', None), 0x00102948: ('uu', None), 0x00122BB8: ('', 'i'), 0x001281C0: ('f', 'i'),
    0x001CAAC0: ('uu', None), 0x001CD370: ('i', 'u'), 0x001D1F20: ('i', None), 0x001D1FF0: ('ii', None),
    0x001D6B10: ('iiii', None), 0x001D6BA0: ('iiiiii', None), 0x001D6C90: ('iiiiiiii', None),
}
TYPES = {'u': 'uint32_t', 'i': 'int32_t', 'f': 'float'}


def params(args, result):
    out = [f'{TYPES[t]} a{k}' for k, t in enumerate(args)]
    if result:
        out.append(f'{TYPES[result]} *result')
    return out


def hook_line(address, args, result):
    return f'    int (*w_{address:08X})(void *ctx{"".join(", " + p for p in params(args, result))});'


def wrapper(address, args, result):
    # float arguments travel as their bits in the module
    p = [f'{"uint32_t" if t == "f" else TYPES[t]} a{k}' for k, t in enumerate(args)]
    a = ['o->h->ctx'] + [f'a16w_float(a{k})' if t == 'f' else f'a{k}' for k, t in enumerate(args)]
    name = f'0x{address:08X}u'
    body = [f'static inline int a16w_c_{address:08X}(A16W *o{"".join(", " + x for x in p)}'
            + {None: ')', 'i': ', int32_t *out)', 'u': ', uint32_t *out)'}[result],
            '{', '    if (a16w_failed(o)) return -1;',
            f'    if (!o->h->w_{address:08X}) return a16w_latch(o, {name}, EM_AREA16_WATER_FAULT_NULL_WORKER);']
    if result:
        a.append('out')
    body.append(f'    if (o->h->w_{address:08X}({", ".join(a)}) < 0) '
                f'return a16w_latch(o, {name}, EM_AREA16_WATER_FAULT_WORKER_FAILED);')
    body += ['    return 0;', '}', '']
    return '\n'.join(body)


def replace(path, begin, end, text):
    s = path.read_text()
    new = re.sub(re.escape(begin) + r'.*?' + re.escape(end), lambda _: begin + '\n' + text + end, s, flags=re.S)
    path.write_text(new)


def main():
    rows = sorted(HOOKS.items())
    replace(ROOT / 'src/game/em_area16_water.h', '/* BEGIN GENERATED HOOKS */', '/* END GENERATED HOOKS */',
            ''.join(hook_line(a, *v) + '\n' for a, v in rows))
    replace(ROOT / 'src/game/em_area16_water_internal.h', '/* BEGIN GENERATED WRAPPERS */',
            '/* END GENERATED WRAPPERS */', '\n'.join(wrapper(a, *v) for a, v in rows))


if __name__ == '__main__':
    main()
