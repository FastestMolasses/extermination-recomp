#!/usr/bin/env python3
"""Generate the hook block of src/game/em_level17_boot.h and the wrappers of
src/game/em_level17_boot_internal.h from HOOKS below (the same shape the level
modules' generated blocks have; tools/test_level17_boot_reference.py reads
HOOKS for the oracle's register maps).

Usage (port root): python3 tools/gen_level17_boot.py
"""
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
# name: (arguments, result): u uint32_t, i int32_t, f float, q uint64_t
HOOKS = {
    0x001AFC10: ('u', None), 0x001B0FD0: ('u', 'i'), 0x001B1B70: ('u', None), 0x001C6380: ('u', None),
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
    a = ['o->h->ctx'] + [f'l17b_float(a{k})' if t == 'f' else f'a{k}' for k, t in enumerate(args)]
    name = f'0x{address:08X}u'
    body = [f'static inline int l17b_c_{address:08X}(L17B *o{"".join(", " + x for x in p)}'
            + {None: ')', 'f': ', uint32_t *out)', 'i': ', int32_t *out)', 'u': ', uint32_t *out)'}[result],
            '{', '    if (l17b_failed(o)) return -1;',
            f'    if (!o->h->w_{address:08X}) return l17b_latch(o, {name}, EM_LEVEL17_BOOT_FAULT_NULL_WORKER);']
    if result == 'f':
        body.append('    float r = 0;')
        a.append('&r')
    elif result:
        a.append('out')
    body.append(f'    if (o->h->w_{address:08X}({", ".join(a)}) < 0) '
                f'return l17b_latch(o, {name}, EM_LEVEL17_BOOT_FAULT_WORKER_FAILED);')
    if result == 'f':
        body.append('    *out = l17b_bits(r);')
    body += ['    return 0;', '}', '']
    return '\n'.join(body)


def replace(path, begin, end, text):
    s = path.read_text()
    new = re.sub(re.escape(begin) + r'.*?' + re.escape(end), lambda _: begin + '\n' + text + end, s, flags=re.S)
    path.write_text(new)


def main():
    rows = sorted(HOOKS.items())
    replace(ROOT / 'src/game/em_level17_boot.h', '/* BEGIN GENERATED HOOKS */', '/* END GENERATED HOOKS */',
            ''.join(hook_line(a, *v) + '\n' for a, v in rows))
    replace(ROOT / 'src/game/em_level17_boot_internal.h', '/* BEGIN GENERATED WRAPPERS */',
            '/* END GENERATED WRAPPERS */', '\n'.join(wrapper(a, *v) for a, v in rows))


if __name__ == '__main__':
    main()
