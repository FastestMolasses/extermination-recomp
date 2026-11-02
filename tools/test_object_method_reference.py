#!/usr/bin/env python3
"""The object-method programs against the ORIGINAL (docs/CHAIN_PAGE.md section
6.6; docs/AREA19_PORT.md open item 2).

Checks emvup_method_batch (src/game/em_vu1_page_programs.h: the VU1 programs
of the draw methods of 0x26E310's kinds 10, 9 and 8) and the walk of their
object units as passes (src/game/em_chain_page.c) against
tools/chain_page_model.py, which executes the ORIGINAL microcode the units'
MPG codes upload from the pinned boot ELF. Reads the owner's ELF and captured
EE RAM under ../Extermination/build/; embeds no original bytes; prints counts
only.

A. Captured units. The method units a capture's RAM holds (001CB140's bytes
   in the packet arena; no capture's LATEST page CALLs one, the arena keeps
   them from the frames before) are CALLed from a page built in unused RAM
   and walked as passes by both sides. Every batch: the translation (the
   model's lanes and the host lanes) and the oracle start from the same data
   memory and registers; all 16 KiB of data memory, every register and the
   kicked packet's bytes must be equal after it. The page: primitives, their
   environments, the pass marks and the counts equal.
B. Synthetic batches: random vertices, rows, colour rows (A below 0, above 128
   and between), fog rows, data words with bit 15, positions on both sides of
   the guard band, both TOPs; both outcomes of every conditional branch.
C. The timing premise: each Q and clip-flag read sees the producer the header
   names.
D. Faults: an exponent-255 word on a live lane faults both sides.

EM_TEST_FULL=1: every captured page and all its units, 400 synthetic batches
per program; quick: one page's first units, 60 per program.
"""
from __future__ import annotations

import collections
import ctypes as C
import hashlib
import random
import struct
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import chain_page_model as M  # noqa: E402
import reference_root as refs  # noqa: E402
import test_chain_page_reference as T  # noqa: E402
from reference_mode import FULL, banner, part, pick, select  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
DECOMP = ROOT.parent / 'Extermination'
OUT = ROOT / 'build/object_method_reference'
ELF_SHA = T.ELF_SHA

# The end snapshots whose RAM holds units of a method program (searched over
# every eeMemory.bin under the decomp's build/fork_refs).
CAPTURED = (
    'levels-recordings-20261030-1/s87/route_a07/a07_00_switch',
    'levels-recordings-20261030-1/s87/route_a03b/a03b_00_door13',
    'levels-recordings-20261030-1/s87/route_a08/a08_05_terminal',
    'levels-recordings-20261031-1/s87/route_a07b/a07b_01_sub2',
    'levels-recordings-20261031-1/s87/route_a07b/a07b_02_duct',
    'levels-recordings-20261031-1/s87/route_a07b/a07b_03_sub3',
    'levels-recordings-20261031-1/s87/route_a07b/a07b_04_terminal',
    'levels-recordings-20261031-1/s87/route_a07b/a07b_05_back',
    'levels-recordings-20261031-1/s87/route_a07b/a07b_06_duct',
    'levels-recordings-20261031-1/s87/route_a07b/a07b_07_lift',
)

# Native id of each program (em_vu1_page_programs.h), and the producers of
# the non-interlocked reads (micro of the reader -> of the producer).
PROGRAMS = {M.PROGRAM_METHOD10: 10}
NAMES = {M.PROGRAM_METHOD10: 'kind10'}
PRODUCERS = {M.PROGRAM_METHOD10: {('Q', 0x03F): 0x038, ('CF', 0x036): 0x032}}
BRANCHES = {M.PROGRAM_METHOD10: (0x062,)}
OFFSETS = (0x1B0, 0x2BE)                          # BASE, BASE + OFFSET

SHIM = r'''
#include "game/em_chain_page.h"
#include "game/em_owner_draw_original.h"
#include <stdlib.h>

typedef struct { uint32_t n; uint32_t at[8]; EmVu1PQword mem[8][1024]; } Kicks;
static int kick_cb(void *ctx, const EmVu1PQword *dmem, uint32_t at)
{
    Kicks *k = ctx;
    if (k->n >= 8) return 1;
    k->at[k->n] = at;
    memcpy(k->mem[k->n], dmem, sizeof(EmVu1PQword) * 1024);
    k->n++;
    return 0;
}
int shim_mscal(int program, int host, EmVu1PRegs *r, EmVu1PQword *dmem, Kicks *k, uint32_t top)
{
    k->n = 0;
    return em_vu1p_mscal((unsigned)program, host, r, dmem, kick_cb, k, top);
}

typedef struct { uint32_t base, size; const uint8_t *bytes; } Region;
static Region *g_regions;
static unsigned g_count;
static const uint8_t *reader(void *ctx, uint32_t a, uint32_t n)
{
    (void)ctx;
    for (unsigned i = 0; i < g_count; ++i)
        if (a >= g_regions[i].base && (uint64_t)a + n <= (uint64_t)g_regions[i].base + g_regions[i].size)
            return g_regions[i].bytes + (a - g_regions[i].base);
    return NULL;
}
static EmChainPage g_page;
static const uint32_t *g_objects;
static uint32_t g_nobjects;
static int object_pass(void *ctx, uint32_t a)
{
    (void)ctx;
    for (uint32_t i = 0; i < g_nobjects; ++i)
        if (g_objects[i] == a) return 1;
    return 0;
}
static uint8_t *g_kick_out;
static uint32_t g_kick_room, g_kick_used, g_kick_count;
void em_chain_page_test_kick(uint32_t program, uint32_t at, const void *packet, uint32_t qwords)
{
    (void)program; (void)at;
    const uint32_t n = 16u * qwords;
    g_kick_count++;
    if (g_kick_used + 4u + n > g_kick_room) return;
    memcpy(g_kick_out + g_kick_used, &n, 4);
    memcpy(g_kick_out + g_kick_used + 4u, packet, n);
    g_kick_used += 4u + n;
}
/* The channel list after 001C7420, as the port builds it, over a host
 * channel at `address`. */
int shim_list(int program, uint32_t model, uint32_t w04, uint32_t ctx_0c, uint32_t ctx_9c, uint32_t arena, uint8_t *out,
              uint32_t room)
{
    EmOwnerServicesChannel ch[4];
    uint32_t ctx50[4] = {0, 0, 0, 0};
    memset(ch, 0, sizeof ch);
    ch[3].cursor = out;
    ch[3].end = out + room;
    EmOwnerDraw d;
    memset(&d, 0, sizeof d);
    d.world.ctx_0C = &ctx_0c;
    d.world.ctx_9C = &ctx_9c;
    d.world.d00275674 = &arena;
    d.world.ctx_50 = ctx50;
    d.world.ctx_50_count = 4;
    d.world.channel = ch;
    d.world.channel_count = 4;
    int rc = -1;
    if (program == 10) rc = em_owner_draw_001D42E0(&d, 3, model, w04);
    return rc < 0 ? -1 : (int)(ch[3].cursor - out);
}
int shim_page(Region *regions, unsigned count, uint32_t start, const uint32_t *skip, uint32_t nskip,
              const uint32_t *objects, uint32_t nobjects, uint32_t bank, uint32_t slot,
              EmGfxGsPrim *prims, EmChainPageQ *q, EmGfxGsEnv *envs, uint32_t capacity, EmChainPageCounts *counts,
              uint32_t *out, uint32_t *firsts, uint32_t *marks, uint8_t *kicks, uint32_t kick_room)
{
    g_regions = regions;
    g_count = count;
    g_objects = objects;
    g_nobjects = nobjects;
    g_kick_out = kicks;
    g_kick_room = kick_room;
    g_kick_used = g_kick_count = 0;
    memset(&g_page, 0, sizeof g_page);
    g_page.read = reader;
    g_page.skip_calls = skip;
    g_page.skip_count = nskip;
    g_page.prims = prims;
    g_page.prim_q = q;
    g_page.prim_env = envs;
    g_page.prim_capacity = capacity;
    g_page.object_pass = object_pass;
    g_page.draw_envs = bank;
    g_page.draw_env_slot = slot;
    int rc = em_chain_page_run(&g_page, start);
    *counts = g_page.counts;
    out[0] = g_page.fault; out[1] = g_page.fault_address; out[2] = g_page.prim_count; out[3] = g_page.again_count;
    out[4] = g_kick_count;
    for (uint32_t k = 0; k < g_page.counts.passes && k < EM_CHAIN_PAGE_WALKED_MAX; ++k) firsts[k] = g_page.pass_first[k];
    for (uint32_t k = 0; k < g_page.again_count && k < EM_CHAIN_PAGE_MARKS_MAX; ++k) marks[k] = g_page.again[k];
    return rc;
}
'''


def fail(msg):
    raise AssertionError(msg)


class Kicks(C.Structure):
    _fields_ = [('n', C.c_uint32), ('at', C.c_uint32 * 8), ('mem', C.c_uint8 * (8 * 16384))]


def build_lib():
    OUT.mkdir(parents=True, exist_ok=True)
    src, lib_path = OUT / 'shim.c', OUT / 'object_method.dylib'
    src.write_text(SHIM)
    subprocess.run(['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-ffp-contract=off', '-shared', '-fPIC',
                    '-DEM_CHAIN_PAGE_TEST_HOOK', '-I' + str(ROOT / 'src'), str(src),
                    str(ROOT / 'src/game/em_chain_page.c'), str(ROOT / 'src/game/em_gs_blocks_original.c'),
                    str(ROOT / 'src/game/em_load_veil_particles.c'), str(ROOT / 'src/game/em_owner_draw_original.c'),
                    '-o', str(lib_path)], check=True)
    return C.CDLL(str(lib_path))


def compare_batch(lib, vu, program, top, stats, where, run):
    """The oracle (run()) and the translation on both lanes from vu's state."""
    r0, dm0 = T.regs_of(vu), bytes(vu.mem)
    vu.kicks, vu.events, vu.watch, vu.top = [], [], set(), top
    vu.pending, vu.cycle, vu.q_ready, vu.p_ready = [], 0, 0, 0
    vu.ready = [[0] * 4 for _ in range(32)]
    refused = None
    try:
        run()
    except M.ModelError as e:
        refused = e
    for host in (0, 1):
        r, dm, k = T.Regs.from_buffer_copy(r0), T.DMEM.from_buffer_copy(dm0), Kicks()
        rc = lib.shim_mscal(PROGRAMS[program], host, C.byref(r), dm, C.byref(k), top)
        if refused is not None:
            if rc != 2:
                fail(f'{where}: the oracle refused ({refused}), the translation returned {rc}')
            stats['operand_faults'] += 1
            continue
        if rc:
            fail(f'{where}: the translation faulted ({rc}), the oracle ran')
        T.same_regs(vu, r, where)
        if bytes(dm) != bytes(vu.mem):
            diff = [q for q in range(1024) if bytes(dm)[16 * q:16 * q + 16] != bytes(vu.mem[16 * q:16 * q + 16])]
            fail(f'{where}: data memory differs at qwords {[hex(q) for q in diff[:8]]}')
        kicks = [e for e in vu.events if e[0] == 'kick']
        if k.n != len(kicks) or k.n != 1:
            fail(f'{where}: {k.n} XGKICKs, the original {len(kicks)}')
        raw = bytes(k.mem)
        if k.at[0] != kicks[0][1] or M.sh.gif_raw(raw[:16384], kicks[0][1]) != kicks[0][3]:
            fail(f'{where}: the XGKICK differs')
        stats['kicked_packet_bytes'] += len(kicks[0][3])
    if refused is not None:
        return False
    for (kind, reader, producer), _n in vu.seen.items():
        if PRODUCERS[program].get((kind, reader)) != producer:
            fail(f'{where}: the {kind} read at micro {reader:#05x} sees micro {producer:#05x}')
    vu.seen.clear()
    stats['batches_' + NAMES[program]] += 1
    return True


class ComparedPage(M.Page):
    def __init__(self, read, lib, stats, where, skip, objects, bank, slot):
        super().__init__(read, skip_calls=skip, pass_calls=objects, draw_envs=bank, draw_env_slot=slot)
        self.vu = T.Oracle()
        self.lib, self.stats, self.where = lib, stats, where
        self.branches = collections.Counter()

    def mscal(self, imm, at, cont=False):
        if self.program not in PROGRAMS:
            return super().mscal(imm, at, cont)
        if self.program is None or imm != 0 or (cont and not self.floor_ran):
            fail(f'{self.where}: MSCAL {imm:#x} at {at:#x}')
        self.mscal_counts[self.program] = self.mscal_counts.get(self.program, 0) + 1
        top = self.tops
        self.dbf ^= 1
        self.tops = self.base + (self.offset if self.dbf else 0)
        vu = self.vu
        compare_batch(self.lib, vu, self.program, top, self.stats, f'{self.where} batch at {at:#x}',
                      lambda: vu.run(vu.resume if cont else 0))
        self.floor_ran = True
        self.branches.update(vu.branch)
        vu.branch.clear()
        for e in vu.events:
            if e[0] == 'kick':
                self.kicks.append((self.program, e[1], e[3]))
                M.gs_feed(self.gs, e[3], f'XGKICK {e[1]:#x}')


def unit_program(read, target):
    """The program a top-level CALL's list CALLs, walked to its RET."""
    cur = target
    for _ in range(64):
        lo, hi = M.u32(read(cur, 4), 0), M.u32(read(cur + 4, 4), 0)
        qwc, tid, addr = lo & 0xFFFF, lo >> 28 & 7, hi & 0x0FFFFFFF
        if tid == 5:
            if addr in PROGRAMS:
                return addr
            cur += 16 + 16 * qwc
        elif tid == 3:
            cur += 16
        elif tid == 1:
            cur += 16 + 16 * qwc
        else:
            return None
    return None


def native_page(lib, ram, start, skip, objects, bank, slot, capacity=8192):
    arr = (T.Region * 1)(T.Region(0, len(ram), bytes(ram)))
    sk = (C.c_uint32 * max(1, len(skip)))(*skip)
    ob = (C.c_uint32 * max(1, len(objects)))(*objects)
    prims, q, envs = (T.GsPrim * capacity)(), (T.PrimQ * capacity)(), (T.GsEnv * capacity)()
    counts, out = T.Counts(), (C.c_uint32 * 5)()
    firsts, marks = (C.c_uint32 * 1024)(), (C.c_uint32 * 2048)()
    room = 1 << 22
    kicks = C.create_string_buffer(room)
    rc = lib.shim_page(arr, 1, start, sk, len(skip), ob, len(objects), bank, slot, prims, q, envs, capacity,
                       C.byref(counts), out, firsts, marks, kicks, room)
    return rc, prims, q, envs, counts, out, list(firsts[:counts.passes]), list(marks[:out[3]]), kicks.raw


def captured_units(ram):
    """The method units a capture's RAM holds: 001CB140's bytes in the packet
    arena (a capture's latest page need not hold one: the channel-3 arena
    keeps the units of the frames before), from the colour CNT to the RET."""
    units = []
    for program in PROGRAMS:
        pat = struct.pack('<II', 0x50000000, program)
        at = M.ARENA
        while True:
            at = ram.find(pat, at, 0x7635C0)
            if at < 0:
                break
            call, at = at, at + 8
            if call & 15:
                continue
            start = None
            for back in range(16, 0x600, 16):
                a = call - back
                if M.u32(ram, a) & 0xFF00FFFF == 0x10000005 and \
                        struct.unpack_from('<4I', ram, a + 16) == (0, 0x11000000, 0x01000101, 0x6C0403F5):
                    start = a
                    break
            if start is None:
                continue
            cur = call + 16
            for _ in range(16):
                lo = M.u32(ram, cur)
                tid, qwc = lo >> 28 & 7, lo & 0xFFFF
                if tid == 6:
                    units.append(start)
                    break
                cur += 16 + (16 * qwc if tid == 1 else 0)
    return sorted(set(units))


def check_lists(lib, ram, units, stats, where):
    """The port's channel list after 001C7420 against each captured unit's own
    bytes (the tags' bytes +2 and +8..+0xF are not written: stale there)."""
    arena = M.u32(ram, 0x275674)
    for u in units:
        a = u
        for _ in range(4):                               # the colour and node CNTs
            if M.u32(ram, a) >> 28 & 7 != 1:
                break
            a += 16 + 16 * (M.u32(ram, a) & 0xFFFF)
        if M.u32(ram, a + 16 + 4) not in PROGRAMS:
            fail(f'{where}: unit {u:#x} does not CALL a method program after its node CNTs')
        program = M.u32(ram, a + 16 + 4)
        # the skin record's slot and the fog-off REF: from the unit itself
        tags = []
        b = a
        for _ in range(16):
            lo = M.u32(ram, b)
            tid, qwc = lo >> 28 & 7, lo & 0xFFFF
            tags.append((tid, qwc, M.u32(ram, b + 4) & 0x0FFFFFFF, b))
            if tid == 6:
                break
            b += 16 + (16 * qwc if tid == 1 else 0)
        model, w04 = tags[-2][2] - 0x40, tags[-2][1]
        fog_off = tags[-3][2] == 0x2514B0
        skin = tags[-4 if fog_off else -3][2]
        slot = (skin & 0xFF) >> 7                         # the frame the unit was built in
        out = C.create_string_buffer(0x400)
        n = lib.shim_list(PROGRAMS[program], model, w04, 0 if fog_off else 1, slot, arena, out, 0x400)
        if n != tags[-1][3] - a:
            fail(f'{where}: unit {u:#x}: the port\'s list is {n:#x} bytes, the original\'s {tags[-1][3] - a:#x}')
        got, want = out.raw[:n], ram[a:a + n]
        for q in range(0, n, 16):
            head = q in [t[3] - a for t in tags]
            lanes = range(8) if head else range(16)
            for k in lanes:
                if head and k == 2:
                    continue
                if got[q + k] != want[q + k]:
                    fail(f'{where}: unit {u:#x}: byte {q + k:#x} of the list differs')
        stats['captured_lists'] += 1


def part_a(lib, stats):
    branches = collections.Counter()
    pages = select(CAPTURED, 1, 0xB1F, keep=lambda i, b: i == 0)
    for rel in pages:
        ram = bytearray((refs.capture(rel) / 'eeMemory.bin').read_bytes())
        units = captured_units(bytes(ram))
        if not units:
            continue                     # the hit is a unit partly overwritten
        objects = units if FULL else units[:pick(0, 4)]
        # A page of those CALLs, in unused RAM above the arena's chain table
        # (D_007635C0's slots are not walked here): each unit where 001CAAC0
        # would have CALLed it, the end link after them.
        start = 0x1F00000
        at = start
        struct.pack_into('<4I', ram, at, 0x20000000, start + 0x40, 0, 0)    # NEXT over the end link
        at = start + 0x40
        for u in objects:
            struct.pack_into('<4I', ram, at, 0x50000000, u, 0, 0)            # CALL
            at += 16
        struct.pack_into('<4I', ram, at, 0x20000000, start + 0x20, 0, 0)     # NEXT to the end link
        ram = bytes(ram)
        read = M.ram_reader(ram)
        ctx = M.u32(ram, 0x275670)
        bank, slot = M.u32(ram, 0x275674) + 0x20, M.u32(ram, ctx + 0x9C)
        where = Path(rel).name
        pg = ComparedPage(read, lib, stats, where, [], objects, bank, slot)
        pg.run(start)
        branches.update(pg.branches)
        rc, prims, q, envs, counts, out, firsts, marks, kicks = native_page(lib, ram, start, [], objects, bank, slot)
        if rc or out[0]:
            fail(f'{where}: the native page faulted ({out[0]} at {out[1]:#x})')
        T.compare_prims(pg.gs.prims, prims, q, out[2], where)
        if counts.passes != pg.passes or counts.object_passes != len(objects) or firsts != pg.pass_firsts or \
                marks != pg.again:
            fail(f'{where}: the passes differ: native {counts.passes} {firsts[:8]} {marks[:8]}, '
                 f'original {pg.passes} {pg.pass_firsts[:8]} {pg.again[:8]}')
        for i in range(out[2]):
            want = M.env_tuple(pg.gs.envs[i]) if pg.gs.envs[i] is not None else (0, (0,) * 10)
            if T.native_env(envs[i]) != want:
                fail(f'{where}: primitive {i}\'s environment differs')
        if out[4] != len(pg.kicks) or counts.kicks != len(pg.kicks) or counts.direct != len(pg.directs):
            fail(f'{where}: kicks {out[4]} / {counts.kicks}, directs {counts.direct}; '
                 f'original {len(pg.kicks)}, {len(pg.directs)}')
        check_lists(lib, ram, units, stats, where)
        stats['captured_pages'] += 1
        stats['captured_units'] += len(objects)
        stats['captured_units_on_pages'] += len(units)
        stats['captured_prims'] += out[2]
    if not stats['captured_pages']:
        fail('no captured RAM holds a whole method unit')
    return branches


def f32(x):
    return struct.unpack('<I', struct.pack('<f', x))[0]


def load_program(vu, elf, program):
    """The packet's MPG through the ELF, as VIF1 uploads it."""
    (first, cnt, imm) = next(k for k, v in M.METHOD_MPGS.items() if v == program)
    vu.code[imm * 8:imm * 8 + 8 * cnt] = T.elf_code(elf, first, 8 * cnt)


def part_b(lib, elf, stats, branches):
    n = pick(400, 60)
    for program in PROGRAMS:
        rng = random.Random(0x0B1E + program)
        for case in range(n):
            vu = T.Oracle()
            load_program(vu, elf, program)
            top = rng.choice(OFFSETS)
            mem = bytearray(16384)
            pos = lambda: rng.choice((rng.uniform(-40, 40), rng.uniform(-400, 400), rng.uniform(-4, 4), 0.0))
            node_row = [0, 8, 16][rng.randrange(3)]
            # the rows: a camera-like transform with w along z
            for r in range(32 * 8):
                T.set_row(mem, r, [f32(rng.uniform(-2, 2)) for _ in range(4)])
            for w in range(0, 3):
                base = w * 8
                sx, sy = rng.uniform(1, 6), rng.uniform(1, 6)
                T.set_row(mem, base, [f32(sx), 0, 0, 0])
                T.set_row(mem, base + 1, [0, f32(sy), 0, 0])
                T.set_row(mem, base + 2, [f32(2048 / 100), f32(2048 / 100), f32(rng.uniform(1, 1000)), f32(1.0)])
                T.set_row(mem, base + 3, [f32(rng.uniform(-4, 4)), f32(rng.uniform(-4, 4)), f32(rng.uniform(1, 4096)),
                                          f32(rng.choice((rng.uniform(1, 80), rng.uniform(-8, 8))))])
            for i in range(32):
                at = top + 4 * i
                word = rng.choice((0, 8, 16)) | (0x8000 if rng.random() < 0.2 else 0)
                T.set_row(mem, at, [rng.getrandbits(32) & 0x7F7FFFFF for _ in range(2)] + [0, 0])
                T.set_row(mem, at + 1, [f32(rng.uniform(0, 1)), f32(rng.uniform(0, 1)), f32(1.0), 0])
                T.set_row(mem, at + 2, [f32(rng.uniform(-1, 1)) for _ in range(3)] + [0])
                T.set_row(mem, at + 3, [f32(pos()), f32(pos()), f32(pos()), word])
            T.set_row(mem, 1013, [0x4B000000 | rng.getrandbits(8), 0x4B000000 | rng.getrandbits(8),
                                  0x4B000000 | rng.getrandbits(8), f32(rng.choice((1.2, -3.0, 0.0, 40.0)))])
            for r in (1014, 1015, 1016, 1017, 1018, 1019):
                T.set_row(mem, r, [T.finite(rng) for _ in range(4)])
            T.set_row(mem, 1020, [0x8020, 0x402E4000, 0x412F, 0])
            T.set_row(mem, 1021, [f32(255), f32(2048), f32(rng.uniform(0, 255)), f32(rng.uniform(-1, 1))])
            T.set_row(mem, 1022, [f32(rng.uniform(0.002, 0.02)), f32(rng.uniform(0.002, 0.02)), f32(1 / 8388608), f32(1.0)])
            T.set_row(mem, 1023, [f32(-1.0), f32(-1.0), f32(-1.0), 0])
            vu.mem[:] = mem
            for i in range(1, 32):
                vu.v[i] = [T.finite(rng) for _ in range(4)]
            vu.vi = [0] + [rng.getrandbits(16) for _ in range(15)]
            vu.accw = [T.finite(rng) for _ in range(4)]
            vu.q, vu.iw, vu.r, vu.cf = T.finite(rng), T.finite(rng), rng.getrandbits(23), rng.getrandbits(24)
            vu.p = T.finite(rng)
            if compare_batch(lib, vu, program, top, stats, f'synthetic {NAMES[program]} case {case}', lambda: vu.run(0)):
                stats['synthetic_' + NAMES[program]] += 1
            for key, k in vu.branch.items():
                branches[(program,) + key] += k
    for program, sites in BRANCHES.items():
        for site in sites:
            for taken in (True, False):
                if not branches[(program, site, taken)] and not branches[(site, taken)]:
                    fail(f'{NAMES[program]}: the branch at micro {site:#05x} was never {taken}')


def part_d(lib, elf, stats):
    for program in PROGRAMS:
        vu = T.Oracle()
        load_program(vu, elf, program)
        top = OFFSETS[0]
        mem = bytearray(16384)
        T.set_row(mem, 1020, [0x8020, 0x402E4000, 0x412F, 0])
        T.set_row(mem, top + 3, [0x7F800000, 0, 0, 0])         # position x: exponent 255
        T.set_row(mem, 0, [f32(1.0), 0, 0, 0])
        vu.mem[:] = mem
        vu.v = [[0, 0, 0, 0x3F800000]] + [[0] * 4 for _ in range(31)]
        before = stats['operand_faults']
        compare_batch(lib, vu, program, top, stats, f'operand {NAMES[program]}', lambda: vu.run(0))
        if stats['operand_faults'] != before + 2:
            fail(f'{NAMES[program]}: an exponent-255 position did not fault both lanes')


def main():
    elf = (DECOMP / 'config/SCUS_971.12').read_bytes()
    if hashlib.sha256(elf).hexdigest() != ELF_SHA:
        fail('boot ELF is not the pinned build')
    lib = build_lib()
    stats = collections.Counter()
    branches = part_a(lib, stats)
    part_b(lib, elf, stats, branches)
    part_d(lib, elf, stats)
    banner(part(stats['captured_units'], stats['captured_units_on_pages'], 'captured units walked'),
           f"{stats['captured_pages']} captured pages ({stats['captured_prims']} primitives), "
           f"{stats['captured_lists']} channel lists byte-equal to the port's",
           ', '.join(f"{stats['batches_' + NAMES[p]]} {NAMES[p]} batches compared ({stats['synthetic_' + NAMES[p]]} "
                     'synthetic)' for p in PROGRAMS),
           f"{stats['kicked_packet_bytes']:,} packet bytes (each translation on both lanes)",
           f"{stats['operand_faults']} operand faults")
    print('object method reference: PASS')
    return 0


if __name__ == '__main__':
    sys.exit(main())
