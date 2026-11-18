#!/usr/bin/env python3
"""Execute the original 001645D0 (state +5 = 0x0A, the climb up from a hang)
and compare em_player_hang.c's em_player_hang_up_state.

The original instructions run from the captured AREA11 EE RAM
(../Extermination/build/startup-reference/playable_ee.bin, state 04), whose
code bytes for 001645D0 are first checked against the user's pinned ELF; none
are embedded here. The interpreter is tools/test_player_hang_reference.py's
HangEE (COP1 through tools/ee_float_model.py, the measured EE rules), with
the branch record moved to 001645D0's range.

No kept capture holds the player in state 0x0A (all 475 full-RAM images under
the decomp's build/startup-reference, fork_refs, guided_play and human_play
were scanned for +5 at 0x8102B5), so the machine states are designed from the
captured player record at 0x8102B0. Changed bytes, per case:
  +5, +6       the state (0x0A) and the sub-state (0..4, and values the
               switch does not handle)
  +7, +D, +1F0, +25F   random bytes, so the stores of sub-states 0 and 4 show
  +28          the step counter halfword (0, 1, 12, 0x8000, 0xFFFF, random)
  +200         the clip-flag word (bit 0x1000 set or clear, random words)
  +B0..+B8, +290..+298, +2E0..+2E8   the pose, target and step floats
               (signed zeros, denormals, units, extremes, uniform values)
A fifth of the cases instead start from 0x320 random bytes (the bone
pointers +40..+47 kept). Everything else is the captured record.

Its two callees, 001749A0 (clip request) and 001FBD50 (positional sound),
are hooked, scripted per case and recorded, never simulated: the scripted
return value and actor writes (including +6, +28, +200 and the pose words,
which the original re-reads after some calls) are applied identically on
both sides, and the call sequence with every argument (floats as bits) must
match. All 0x320 actor bytes must match after the call and at every callee
entry. A scripted worker fault (-1 on the native side) must stop the native
at the original's bytes at that call. Every conditional branch outcome of
001645D0 and every handled sub-state must be exercised (asserted).

EM_TEST_FULL=1 runs the exhaustive sweep; the default run is a fixed-seed
sample with the same comparisons and coverage assertions.
"""
import ctypes as C
import random
import struct
import sys
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import test_player_hang_reference as H  # noqa: E402
from test_player_slide_reference import EE, read_elf, bits, s32, sx32  # noqa: E402
from test_player_floor_reference import LiveActor  # noqa: E402
import reference_mode  # noqa: E402

PLAYER = H.PLAYER
UP, UP_END = 0x1645D0, 0x1647D0
CALLEES = {0x1749A0: 'request', 0x1FBD50: 'sound'}
FLOATS = (0xB0, 0xB4, 0xB8, 0x290, 0x294, 0x298, 0x2E0, 0x2E4, 0x2E8)
SPECIAL = (0, 0x80000000, 1, 0x80000001, 0x007FFFFF, 0x3F800000, 0xBF800000, 0x7F7FFFFF, 0xFF7FFFFF,
           0x7F800000, 0xFF800000, 0x7FFFFFFF, 0x41400000, 0x00800000, 0x80800000)


class UpEE(H.HangEE):
    """HangEE recording the conditional branches inside 001645D0."""

    def branch(self, word, pc):
        outcome = EE.branch(self, word, pc)
        if outcome is not None and UP <= pc < UP_END:
            self.branches.add((pc, outcome[0]))
        return outcome


def fword(rng):
    if rng.random() < 0.2:
        return rng.choice(SPECIAL)
    return bits(rng.uniform(-600.0, 600.0))


def effect(seed, name, index):
    """(result, [(offset, size, value)]) of the index-th `name` call of a case."""
    rng = random.Random(zlib.crc32(('%d/%s/%d' % (seed, name, index)).encode()))
    writes = []
    def maybe(p, offset, size, value):
        if rng.random() < p: writes.append((offset, size, value))
    maybe(0.25, 6, 1, rng.randrange(8))
    maybe(0.2, 0x28, 2, rng.choice([0, 1, 5, 0xFFFF]))
    maybe(0.2, 0x200, 4, rng.choice([0, 0x1000, 0x8000, 0x9000]))
    for offset in FLOATS:
        maybe(0.08, offset, 4, fword(rng))
    maybe(0.1, 0x25F, 1, rng.randrange(256))
    return rng.choice([0, 1, 3, -1 & 0xFFFFFFFF]), writes


# ------------------------------------------------------------- oracle ----

RAM = ELF = NATIVE = CAPTURED = None
_EE = None


def oracle_ee():
    global _EE
    if _EE is None:
        _EE = UpEE(ELF, RAM)
    return _EE


def install_hooks(ee, seed):
    counters = {}
    ee.snapshots = []

    def hook(name):
        def run(o):
            index = counters.get(name, 0); counters[name] = index + 1
            o.snapshots.append(bytes(o.read(PLAYER, 0x320)))
            assert o.r[4] & 0xFFFFFFFF == PLAYER, (name, hex(o.r[4]))
            o.log.append((name, s32(o.r[5]), s32(o.r[6]), o.f[12]))
            result, writes = effect(seed, name, index)
            for offset, size, value in writes:
                o.save(PLAYER + offset, value, size)
            o.r[2] = sx32(result)
        return run
    ee.hooks = {address: hook(name) for address, name in CALLEES.items()}


class Native:
    def __init__(self, seed, fault_at=None):
        self.seed, self.log, self.snapshots, self.counters = seed, [], [], {}
        self.fault_at, self.live, self.keep = fault_at, None, []
        w = self.workers = H.HangWorkers()
        for name in ('request', 'sound'):
            fn = H.CALL_FN(self._make(name)); self.keep.append(fn)
            setattr(w, name, fn)

    def _make(self, name):
        def run(_, actor, x, y, f):
            self.log.append((name, x, y, bits(f)))
            self.snapshots.append(bytes(self.live.bytes))
            index = self.counters.get(name, 0); self.counters[name] = index + 1
            if self.fault_at == (name, index):
                return -1
            _result, writes = effect(self.seed, name, index)
            raw = actor.contents.bytes
            for offset, size, value in writes:
                for i in range(size): raw[offset + i] = (value >> (8 * i)) & 0xFF
            return 0
        return run


def random_case(seed):
    rng = random.Random(seed)
    if rng.random() < 0.2:
        raw = bytearray(rng.randrange(256) for _ in range(0x320))
        raw[0x40:0x48] = CAPTURED[0x40:0x48]
    else:
        raw = bytearray(CAPTURED)
    raw[5] = 0x0A
    raw[6] = rng.choice((0, 1, 2, 3, 3, 3, 4) * 3 + (5, 0x0A, 0x80, 0xFF, rng.randrange(256)))
    for offset in (7, 0xD, 0x1F0, 0x25F):
        raw[offset] = rng.randrange(256)
    struct.pack_into('<H', raw, 0x28, rng.choice([0, 0, 1, 1, 2, 12, 0x7FFF, 0x8000, 0xFFFF,
                                                   rng.randrange(0x10000)]))
    struct.pack_into('<I', raw, 0x200, rng.choice([0, 0x1000, 0x1000, 0x8000, 0x9000, 0xFFFFEFFF,
                                                    rng.randrange(1 << 32)]))
    for offset in FLOATS:
        struct.pack_into('<I', raw, offset, fword(rng))
    if rng.random() < 0.3:          # a realistic climb: target a little above / ahead of the pose
        for k in range(3):
            pose = rng.uniform(-400.0, 400.0)
            struct.pack_into('<f', raw, 0xB0 + 4 * k, pose)
            struct.pack_into('<f', raw, 0x290 + 4 * k, pose + rng.uniform(-30.0, 30.0))
    fault_at = None
    if rng.random() < 0.1:
        fault_at = (rng.choice(('request', 'sound')), 0)
    return raw, fault_at


def run_case(seed):
    raw, fault_at = random_case(seed)
    ee = oracle_ee()
    ee.reset(); ee.branches = set()
    ee.write(PLAYER, bytes(raw))
    install_hooks(ee, seed)
    ee.call(UP, (PLAYER,))
    want = bytes(ee.read(PLAYER, 0x320))
    side = Native(seed, fault_at)
    live = LiveActor(); C.memmove(live.bytes, bytes(raw), 0x320)
    side.live = live
    result = NATIVE.em_player_hang_up_state(C.addressof(side.workers), C.byref(live))
    got = bytes(live.bytes)
    names = [e[0] for e in ee.log]
    if fault_at is not None and fault_at[0] in names:
        k = names.index(fault_at[0])
        assert result == -1, (seed, 'fault not propagated', fault_at)
        assert side.log == ee.log[:k + 1], (seed, 'fault log', side.log, ee.log[:k + 1])
        assert side.snapshots == ee.snapshots[:k + 1], (seed, 'fault: actor bytes at an earlier call')
        assert got == ee.snapshots[k], (seed, 'fault bytes',
                                        [hex(i) for i in range(0x320) if got[i] != ee.snapshots[k][i]][:16])
        return raw[6], True, frozenset(ee.branches)
    assert result == 0, (seed, 'native result', result)
    assert side.log == ee.log, (seed, 'callee sequence', raw[6], side.log, ee.log)
    for k, (mine, theirs) in enumerate(zip(side.snapshots, ee.snapshots)):
        if mine != theirs:
            raise AssertionError((seed, 'sub-state %#x' % raw[6], 'actor bytes at call', k, ee.log[k][0],
                                  [hex(i) for i in range(0x320) if mine[i] != theirs[i]][:16]))
    if got != want:
        raise AssertionError((seed, 'sub-state %#x' % raw[6], 'actor bytes differ at',
                              [hex(i) for i in range(0x320) if got[i] != want[i]][:24]))
    return raw[6], False, frozenset(ee.branches)


def run_chunk(seeds):
    return [run_case(seed) for seed in seeds]


def refusal_checks():
    """A missing request or sound worker refuses before any write."""
    raw = bytes(range(256)) * 3 + bytes(0x20)
    raw = raw[:6] + bytes([3]) + raw[7:0x28] + bytes(2) + raw[0x2A:]     # +6 = 3, +28 = 0
    for field in ('request', 'sound'):
        live = LiveActor(); C.memmove(live.bytes, raw, 0x320)
        side = Native(1)
        setattr(side.workers, field, H.CALL_FN())
        assert NATIVE.em_player_hang_up_state(C.addressof(side.workers), C.byref(live)) == -1, field
        assert bytes(live.bytes) == raw, ('refusal wrote', field)
    live = LiveActor(); C.memmove(live.bytes, raw, 0x320)
    assert NATIVE.em_player_hang_up_state(None, C.byref(live)) == -1
    assert bytes(live.bytes) == raw
    return 3


def main():
    global RAM, ELF, NATIVE, CAPTURED
    ELF = read_elf()
    if not H.RAM_PATH.exists():
        raise SystemExit('missing %s (the captured AREA11 RAM)' % H.RAM_PATH)
    RAM = H.RAM_PATH.read_bytes()
    at = UP - 0x100000 + 0x300
    assert RAM[UP:UP_END] == ELF[at:at + (UP_END - UP)], 'captured RAM differs from the ELF at 001645D0'
    CAPTURED = RAM[PLAYER:PLAYER + 0x320]
    NATIVE = H.build_native()
    NATIVE.em_player_hang_up_state.argtypes = [C.c_void_p, H.A]

    total = reference_mode.pick(200000, 20000)
    base = UP * 7
    chunks = [list(range(base + i, base + min(i + 500, total))) for i in range(0, total, 500)]
    results = [r for chunk in reference_mode.parallel_map(run_chunk, chunks) for r in chunk]
    branches, per_state, faults = set(), {}, 0
    for st, fault, case_branches in results:
        branches |= case_branches
        per_state[st] = per_state.get(st, 0) + 1
        faults += fault
    wanted = [(pc, taken) for pc in H.conditional_branches(RAM, UP, UP_END) for taken in (False, True)]
    missing = [(hex(pc), taken) for pc, taken in wanted if (pc, taken) not in branches]
    assert not missing, ('branch outcomes never exercised', missing)
    for state in range(5):
        assert per_state.get(state, 0) > 0, ('sub-state never run', state)
    assert faults > 0, 'no worker fault propagated'
    refusals = refusal_checks()
    reference_mode.banner(reference_mode.part(len(results), 200000, '001645D0 cases'))
    print('player hang-up reference: PASS -- %d 001645D0 cases (sub-states 0..4 and %d unhandled +6 '
          'values): all 0x320 actor bytes after the call and at every callee entry, and every '
          '001749A0 / 001FBD50 call with its arguments, identical; %d/%d conditional branch outcomes '
          'exercised; %d worker faults stop at the original bytes; %d missing-worker refusals'
          % (len(results), len(set(per_state) - set(range(5))), len(wanted) - len(missing), len(wanted),
             faults, refusals))


if __name__ == '__main__':
    main()
