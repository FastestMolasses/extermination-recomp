#!/usr/bin/env python3
"""The port's EE sound state and RPC 0x64 exchanges against the original's, tick by tick.

docs/SOUND_SECOND_HALF.md. The port's side is tools/watch_takes.py --snd-dump
(EM_SOUND_DUMP: build/sndpics/watch_<take>/port/sound.jsonl, one row per
replay tick keyed by the recording's engine counter); the original's is
tools/capture_sound_original.py (decomp build/sndpics/orig/<take>/r<R0>_<R1>/,
watch tables at every main-loop top and at every exchange). Rows pair by the
engine counter; each loop top's exchanges are those the original made after
the previous loop top (ee_cycle order), as the port's row lists the fields
since its previous replay tick.

Compared at each loop top (the game code's outputs; the IOP's are the ENVX):
  lane / cue / clip  D_00282154..5B, the stream cues D_00282178[3], D_00275B2C
  ring / rh / rt     the voice ring D_00281CF0, D_00275B30 / 34
  b70 / c30 / f30    the looped-service tables, the delayed cues
  cur / ser          D_0027F740 + 0x30 / + 0x34 (the allocator's cursor, serial)
  vo                 every voice record in use, by index, in the 17 fields of
                     EmSoundSample.voice (state, note, owner, release, serial,
                     sustain, kind, age, priority, alloc, portamento, base,
                     depth, remaining, length, program, bank handle)
  envx               the reaper's input D_002817C0 (the IOP driver's reply)
  tlr                the port's allocated tracks' volume requests D_0027E0C0 +
                     0x48 / + 0x4C (0011A218's pair), against the same tracks'
  xsfx / xstr        the exchanges: per field, the original's one queue
                     D_0027F7C0 against the port's SFX commands and its stream
                     queue (the stream commands are taken out of the original's
                     queue in order; what is left must be the SFX driver's
                     commands 1, 3, 5, 6, 0xA..0xD and equal the SFX side's)

The voice numbers, the global serial and the sample addresses are history,
not this window's code: 00117428 takes the next free voice from the cursor
D_0027F740 + 0x30, and the original's title keys sounds the port's does not
run through the driver (the port's SFX side starts at the New Game's registry
load), so the two runs' cursors and serials stand apart by a constant. The
comparison therefore maps the port's voices to the original's: learned from
each exchange's per-voice commands (1, 3, 5, 6 in order), and for a voice no
command names yet (keyed before the window) the original's live voice with the
same relative key-on serial (+0x0A) and note, else the rotation the cursors
give over the 44 SFX voices (00117428 never hands out 0..3); the stream voices 0..3 map to
themselves; the cursor and the global
serial are compared by their difference from the first row's; the KON / KOFF / EON / NON masks are
compared through that map (EON / NON on the mapped voices' bits only: the
other bits are the history's), each voice record's serial (+0x0A) by its
difference from the first row's global serial, and command 5's address
(the port names the sample, the original its SPU2 address) by a one-to-one
map learned over the window. The stream voices' age (+0x1C) and bank handle
(+0x22) count from each run's boot and are left out, and so are the free
records (state 0, no owner): what a voice's last life left, or the boot (the
original's never-keyed voices hold bank handle 0 where the port's hold
0xFFFF: SOUND_SECOND_HALF.md "Open").

Nothing original is printed beyond compared values. Exit 0 when every key is
equal at every paired tick (or with --report-only).

Usage (port root):
  python3 tools/compare_sound_ticks.py build/sndpics/watch_g01_title_newgame/port/sound.jsonl \\
      ../Extermination/build/sndpics/orig/g42_area18_area14/r0_600
"""
import argparse
import json
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DECOMP = ROOT.parent / 'Extermination'
sys.path.insert(0, str(DECOMP.parent / 'pcsx2-fork/extermination/python'))

VOICE_FIELDS = (0, 2, 6, 8, 0xA, 0xC, 0x1A, 0x1C, 0x1E, 0x20, 0x44, 0x4E, 0x60, 0x62, 0x64, 0x3E, 0x22)
KEYS = ('lane', 'cue', 'clip', 'ring', 'rh', 'rt', 'b70', 'c30', 'f30', 'cur', 'ser', 'vo', 'envx', 'tlr', 'xsfx',
        'xstr')
SND_SPANS = (('counter', 0x70003B64, 4), ('pos', 0x810350, 12), ('vblank', 0x810E90, 4),
             ('reply', 0x2817C0, 0x9F0), ('voices', 0x27CCC0, 48 * 0x6A), ('tracks', 0x27E0C0, 48 * 0x78),
             ('lib', 0x27F740, 0x50), ('iopbuf', 0x275B18, 0x40))


def table_rows(path):
    from pcsx2dbg.tables import Table
    return Table(path).rows()


def orig_row(row):
    """A loop-top table row in the port's "snd" form."""
    blocks = {name: (addr, row[name]) for name, addr, _ in SND_SPANS}

    def rd(addr, n):
        for base, data in blocks.values():
            if base <= addr and addr + n <= base + len(data):
                return data[addr - base:addr - base + n]
        raise KeyError(hex(addr))

    def w(addr):
        return struct.unpack('<i', rd(addr, 4))[0]

    lane = rd(0x282154, 8)
    vo = []
    for v in range(48):
        r = rd(0x27CCC0 + 0x6A * v, 0x6A)
        f = [struct.unpack_from('<H', r, o)[0] for o in VOICE_FIELDS]
        if f[0] or f[2] != 0xFFFF or f[16] != 0xFFFF:
            vo.append([v] + f)
    return {
        'lane': [struct.unpack('b', lane[k:k + 1])[0] for k in range(5)] + [lane[7]],
        'cue': [w(0x282178 + 4 * k) for k in range(3)], 'clip': w(0x275B2C),
        'ring': [w(0x281CF0 + 4 * k) for k in range(16)],
        'rh': struct.unpack('b', rd(0x275B30, 1))[0], 'rt': struct.unpack('b', rd(0x275B34, 1))[0],
        'b70': [[k, w(0x281B70 + 4 * k)] for k in range(48) if w(0x281B70 + 4 * k) != -1],
        'c30': [[k, w(0x281C30 + 4 * k)] for k in range(48) if w(0x281C30 + 4 * k) != -1],
        'f30': [list(struct.unpack('<4i', rd(0x281F30 + 16 * k, 16))) for k in range(10)],
        'cur': struct.unpack('<I', rd(0x27F740 + 0x30, 4))[0], 'ser': struct.unpack('<I', rd(0x27F740 + 0x34, 4))[0],
        'vo': vo, 'envx': [[v, w(0x2817C0 + 4 * v)] for v in range(48) if w(0x2817C0 + 4 * v)],
        'tlr': {k: list(struct.unpack('<2i', rd(0x27E0C0 + 0x78 * k + 0x48, 8))) for k in range(48)},
    }


def orig_exchange(row):
    lib = row['lib']
    bank, count = struct.unpack_from('<ii', lib, 0x3C)
    q = row['queue'][bank * 0x1000:]
    return [list(struct.unpack_from('<4I', q, 16 * i)) for i in range(max(0, min(count, 255)))]


SFX_COMMANDS = (1, 3, 5, 6, 0xA, 0xB, 0xC, 0xD)    # the SFX driver's (em_sfx_bank.c emit)


def split_exchange(orig, port_str):
    """Take the port's stream commands out of the original's queue in order: (the rest, the rest's
    commands no SFX driver sends), or None when the port's are not a subsequence of the queue."""
    rest, j = [], 0
    for cmd in orig:
        if j < len(port_str) and cmd == port_str[j]:
            j += 1
        else:
            rest.append(cmd)
    if j != len(port_str):
        return None
    return [c for c in rest if c[0] in SFX_COMMANDS], [c for c in rest if c[0] not in SFX_COMMANDS]


def load_port(path):
    rows = {}
    for line in Path(path).open():
        try:
            d = json.loads(line)
        except ValueError:
            continue        # a line the running port is still writing
        rows[d['c']] = d
    return rows


class VoiceMap:
    """The port's voice numbers -> the original's (module docstring)."""

    def __init__(self, port_cur, orig_cur):
        # 00117428 never hands out the stream voices 0..3: the rotation turns the 44 SFX voices
        self.rot = (orig_cur - port_cur) % 44
        self.learned, self.conflicts, self.samples = {}, 0, {}

    def rotation(self, v):
        return v if v < 4 else (v - 4 + self.rot) % 44 + 4

    def __call__(self, v):
        return self.learned.get(v, self.rotation(v))

    def learn(self, pv, ov):
        """A key-on: the port's voice is the original's from now on (a voice's number is reused, and
        the two runs reuse them apart once a voice's end lands a tick apart)."""
        for k in [k for k, w in self.learned.items() if w == ov and k != pv]:
            del self.learned[k]
        self.learned[pv] = ov

    def by_serial(self, pv, ov):
        """A live SFX voice no command has named yet (keyed before the window): the original's live
        voice with the same key-on serial (relative), note and kind."""
        taken = set(self.learned.values())
        for v, f in pv.items():
            if v in self.learned or f[6] != 2:
                continue
            hits = [w for w, g in ov.items() if w not in taken and g[6] == 2 and g[4] == f[4] and g[1] == f[1]]
            self.conflicts += len(hits) > 1
            if len(hits) == 1:
                self.learned[v] = hits[0]
                taken.add(hits[0])

    def mask(self, m):
        out = 0
        for v in range(48):
            if m >> v & 1:
                out |= 1 << self(v)
        return out

    def sample(self, port_index, orig_addr):
        """1 while the port's sample index and the original's address stay one-to-one."""
        if self.samples.setdefault(port_index, orig_addr) != orig_addr:
            return 0
        return list(self.samples.values()).count(orig_addr) == 1


def same_exchange(vm, psfx, orest):
    """The port's SFX commands against the original's (the stream's taken out), through the map.
    The key-ons (command 5 names a key-on: 00115850 sends 6, 1, 5, 3 for it) teach the map in their
    order; the rest is compared as a multiset: 00116598's re-sends go in voice-number order, which is
    the history's."""
    if len(psfx) != len(orest) or sorted(c[0] for c in psfx) != sorted(c[0] for c in orest):
        return False
    pk = [c[1] for c in psfx if c[0] == 5]
    ok_ = [c[1] for c in orest if c[0] == 5]
    if len(pk) != len(ok_):
        return False
    for pv, ov in zip(pk, ok_):
        vm.learn(pv, ov)
    pairs = list(zip(sorted(psfx, key=lambda c: (c[0], vm(c[1]) if c[0] in (1, 3, 5, 6) else c[1], c[2], c[3])),
                     sorted(orest, key=lambda c: (c[0], c[1], c[2], c[3]))))
    ok = True
    for pc, oc in pairs:
        c = pc[0]
        if c in (1, 3, 6):
            ok &= [vm(pc[1]), pc[2], pc[3]] == oc[1:]
        elif c == 5:
            ok &= vm(pc[1]) == oc[1] and bool(vm.sample(pc[2], oc[2])) and pc[3] == oc[3]
        elif c in (0xA, 0xB):
            m = vm.mask(pc[2] | pc[3] << 24)
            ok &= [pc[1], m & 0xFFFFFF, m >> 24] == oc[1:]
        elif c in (0xC, 0xD):
            # EON / NON carry every voice's bit, the history's too (a voice the original's title
            # keyed with the effect send keeps its bit): compared on the voices the window mapped
            pm, om, known = 0, oc[2] | oc[3] << 24, 0
            for v, w in vm.learned.items():
                known |= 1 << w
                if (pc[2] | pc[3] << 24) >> v & 1:
                    pm |= 1 << w
            ok &= pc[1] == oc[1] and pm == om & known
        else:
            ok &= pc == oc
    return ok


def norm_voices(vo, vm=None, ser0=0):
    """Voice records by (mapped) index, the serial relative, the stream voices' age and bank left out."""
    out = {}
    for x in vo:
        v, f = x[0], list(x[1:])
        if not f[0] and f[2] == 0xFFFF:
            continue    # a free record: its fields are what its last life (or the boot) left
        f[4] = (f[4] - ser0) & 0xFFFF
        if f[6] == 3:
            f[4] = f[7] = f[16] = None
        out[vm(v) if vm else v] = f
    return out


def compare(port_path, orig_dir, report_only=False, quiet=False, max_lines=40):
    port = load_port(port_path)
    snd = table_rows(Path(orig_dir) / 'snd.tbl')
    xchg = table_rows(Path(orig_dir) / 'xchg.tbl')
    stats = {k: [0, 0, None] for k in KEYS}     # equal, differ, first difference
    paired, lines, prev_cycle, xi, vm, base = 0, [], None, 0, None, None
    for row in snd:
        counter = struct.unpack('<I', row['counter'])[0]
        mine = []
        while xi < len(xchg) and xchg[xi]['ee_cycle'] < row['ee_cycle']:
            if prev_cycle is not None and xchg[xi]['ee_cycle'] > prev_cycle:
                mine.append(orig_exchange(xchg[xi]))
            xi += 1
        first_row = prev_cycle is None
        prev_cycle = row['ee_cycle']
        p = port.get(counter)
        if p is None or p.get('snd') is None:
            continue
        paired += 1
        o = orig_row(row)
        s = p['snd']
        if vm is None:
            vm = VoiceMap(s['cur'], o['cur'])
            base = (s['ser'], o['ser'], s['cur'], o['cur'])
        if not first_row:
            px = [x for x in p['x'] if not x[3]]
            dropped = any(x[3] for x in p['x'])
            ok_n = len(px) == len(mine) and not dropped
            sfx_ok = str_ok = ok_n
            if ok_n:
                for (_, psfx, pstr, _, *_more), oq in zip(px, mine):
                    split = split_exchange(oq, pstr)
                    str_ok = str_ok and split is not None and not split[1]
                    sfx_ok = sfx_ok and split is not None and same_exchange(vm, psfx, split[0])
            note(stats, 'xsfx', sfx_ok, counter, ([x[1] for x in px], mine))
            note(stats, 'xstr', str_ok, counter, ([x[2] for x in px], mine))
        for key in ('lane', 'cue', 'clip', 'ring', 'rh', 'rt', 'b70', 'c30', 'f30'):
            note(stats, key, s[key] == o[key], counter, (s[key], o[key]))
        note(stats, 'cur', s['cur'] - base[2] == o['cur'] - base[3], counter, (s['cur'], o['cur']))
        note(stats, 'ser', s['ser'] - base[0] == o['ser'] - base[1], counter, (s['ser'], o['ser']))
        ov = norm_voices(o['vo'], None, base[1])
        vm.by_serial(norm_voices(s['vo'], None, base[0]), ov)
        pv = norm_voices(s['vo'], vm, base[0])
        note(stats, 'vo', pv == ov, counter, (diff_voices(pv, ov), ''))
        # the reaper's test (D_002817C0 < 2) on the SFX voices: the words themselves are the
        # envelope sampled at the IOP driver's tick, whose phase is hardware timing
        pe = sorted(vm(v) for v, w in s['envx'] if v >= 4 and w >= 2)
        oe = sorted(v for v, w in o['envx'] if v >= 4 and w >= 2)
        note(stats, 'envx', pe == oe, counter, (pe, oe))
        if 'tlr' in s:
            pt = {k: [a_, b_] for k, a_, b_ in s['tlr']}
            ot = {k: o['tlr'][k] for k in pt}
            note(stats, 'tlr', pt == ot, counter, (pt, ot))
    for key in KEYS:
        eq, ne, first = stats[key]
        if ne:
            lines.append(f'  {key}: {eq} equal, {ne} differ; first at counter {first[0]}: '
                         f'port {short(first[1][0])} original {short(first[1][1])}')
    if vm and vm.conflicts:
        lines.append(f'  voice map: {vm.conflicts} conflicting pairs (a voice number the commands map two ways)')
    result = dict(paired=paired, loop_tops=len(snd), exchanges=len(xchg),
                  equal={k: stats[k][0] for k in KEYS}, differ={k: stats[k][1] for k in KEYS},
                  first={k: stats[k][2][0] for k in KEYS if stats[k][2]},
                  map_conflicts=vm.conflicts if vm else 0)
    if not quiet:
        print(f'{orig_dir}: {paired} ticks paired of {len(snd)} loop tops, {len(xchg)} exchanges; '
              f'{"every key equal" if not lines else "differences:"}')
        for ln in lines[:max_lines]:
            print(ln)
    return result


def diff_voices(pv, ov):
    out = []
    for v in sorted(set(pv) | set(ov)):
        a, b = pv.get(v), ov.get(v)
        if a != b:
            out.append([v, a, b] if a is None or b is None else
                       [v, {k: [x, y] for k, (x, y) in enumerate(zip(a, b)) if x != y}])
    return out


def note(stats, key, ok, counter, values):
    st = stats[key]
    if ok:
        st[0] += 1
    else:
        st[1] += 1
        if st[2] is None:
            st[2] = (counter, values)


def short(v, n=600):
    t = json.dumps(v)
    return t if len(t) <= n else t[:n] + '...'


def main(argv):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('port', help='the port\'s sound.jsonl')
    ap.add_argument('orig', nargs='+', help='original capture folders (r<R0>_<R1>)')
    ap.add_argument('--report-only', action='store_true')
    ap.add_argument('--json', type=Path, help='write the results here')
    a = ap.parse_args(argv)
    results = {}
    bad = 0
    for d in a.orig:
        r = compare(a.port, d, a.report_only)
        results[str(d)] = r
        bad += not r['paired'] or any(r['differ'].values())
    if a.json:
        a.json.write_text(json.dumps(results, indent=1) + '\n')
    return 0 if a.report_only or not bad else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
