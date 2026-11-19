#!/usr/bin/env python3
"""Record the comparison-only drive replay of Claude's kept guided takes.

The guided variant of tools/record_drive_replay.py (docs/IOP_STREAM.md "Drive
replay", "Guided takes"). Each named take of the decomp's build/guided_play is
replayed hidden on the PCSX2 fork from its own start state, pad and card events,
exactly as the decomp's tools/guided_take_probe.py and record_guided_run.py
verify do (under the decomp's run lock build/.pcsx2.lock; the session is shut
down when the take ends). One more watch table, sampled at the main-loop top
(trigger 'tick', the trigger of the take's own rows.tbl, so its counter is
take.rec's), holds the main-loop counter 0x70003B64, the read sequencer
001FA0D0's block (decomp src/func_001FA0D0.c: phase D_00282157, lane
D_00282158) and the three lane records at D_00281FD0 (stride 0x60, sector
+0x30, count +0x34, state byte +3).

A read is issued in tick k-1 when the phase is 1 at sample k-1 and 2 at
sample k; its key is tick k-1's own counter (take.rec's counter of the tick
that issued it: the input playback maps the run's counter onto take.rec's tick
by tick, IOP_STREAM.md "Guided takes"; record_drive_replay.py's level-smoke
tables key one later, on the first sample that shows phase 2, under their own
anchors). Its seek fields are the following samples still in phase 2 on that
lane, ending when 001FA0D0 marks the lane's byte 2 (done). A phase-2 exit
without the mark (a break) is left out. Samples run 40 ticks past the take's
end so a read still in flight there is measured (the drive's time does not
depend on the pad); a read issued in the take's last tick is this take's.

The replay must reproduce the take: its machine at the take's last vsync
(EE RAM, scratchpad, VU, IOP hashes and the EE cycle) is compared with the
take's end.json. A take whose replay differs gets its table as
<take>.reads.diverged, which no tool reads.

Output (decomp build/drive_replay_guided/<take>/; disc-derived timing data,
never committed, never deleted: a recording rule of the user's):
  drv.tbl(.json)  the raw per-tick samples (the fork's watch table)
  <take>.reads    "# ..." header, then "counter sector count fields" lines
  receipt.json    the replay's check, the reads' summary, the seconds
--reextract derives <take>.reads again from drv.tbl; an earlier table that
differs is kept beside it as <take>.reads.superseded-<time>.

The port reads them through tools/drive_replay.py guided_table (watch_takes.py
and guided_chain.py --drive-replay; the developer switch EM_PS2_DRIVE_REPLAY).

LOADER (--loader, lane PORT-CARD-CHAINS 2026-11-19): the screen-module loader's
disc reads (00200780 -> sceCdRead 00112440: the area streamer 001FFCD0 / 001FF590
and the bank loads 001FF830 / 001FF3F0 / 00200360) and their polls (00200730's
00112D18(1) result, at its return 0x00200740) are logged by two log probes on a
second replay of the take (the probes are inline: no observer effect; the replay
is checked against end.json as above). A read issued in tick i whose first poll
answering "done" (v0 == 0) runs in tick j took j - i - 1 busy polls: the port's
module-loader drive answers busy for that many fields after the read's own
(docs/MODULE_LOADER.md 1.7; the New Game's measured rows count the same way).
The key is tick i's counter (the replay's rows.tbl maps the probe's tick onto
0x70003B64); the file and the sector inside it come from the user's disc
(INDEX.IDX = 0, DATA.DAT = 1, OVERLAY/AREAnn.BIN = 0x10 + nn: the port's
measured-table ids). Output beside the take's stream table: loader_calls.bin,
loader_rows.tbl(.json) (the raw recording, never deleted), <take>.loader
("L counter file sector sectors busy") and loader_receipt.json. Without it the
port's area loads answer at host speed (MODULE_LOADER.md 1.7 "Any other read"),
so its loads pass fewer frames than the original's and the chains' frames sit in
the other bank after a load with an odd difference (AREA21_PORT.md "Step K's Z
read-back").

BANK (--bank, lane PORT-LOAD-TIMING 2026-11-19): each area load's sound-bank
transfer (001FB370 -> 001FB3E0 -> 001FB910, IOP_STREAM.md "The sound-bank
transfer"). Its pace is set by two answers the port models at host speed: the
kernel's sceSifDmaStat in 001FB910 (pending while the SIF DMA runs) and
00119450(0) in 001FB3E0 (the IOP's acknowledgement of command 0x20, the SPU
upload). Log probes on a replay of the take (the start of 001FB3E0's state 0,
the returns of those calls and of 001FB370's call sites) give, per load, the
answers in call order: <take>.bank ("B counter end calls answers": the counter
of the load's first call, the bank's end 001FB370 returns, the recording's
001FB370 calls; answers p / d pending / done, 0 / 1 / n not yet /
acknowledged / negative), bank_calls.bin and bank_rows.tbl (the raw recording,
never deleted) and bank_receipt.json. The port's em_sound_bank serves them
under the drive replay ("B" lines). --loader --bank records both in one replay.

Usage (native arm64 macOS; run with the decomp's .venv python, which has numpy):
  ../Extermination/.venv/bin/python tools/record_drive_replay_guided.py g36_area00_area14 [more takes]
  ../Extermination/.venv/bin/python tools/record_drive_replay_guided.py --all       # every kept take not yet recorded
  ../Extermination/.venv/bin/python tools/record_drive_replay_guided.py --loader g73_area04_area20 [more]
  ../Extermination/.venv/bin/python tools/record_drive_replay_guided.py --loader --all
  ../Extermination/.venv/bin/python tools/record_drive_replay_guided.py --bank g73_area04_area20 [more]
  ../Extermination/.venv/bin/python tools/record_drive_replay_guided.py --loader --bank g01_title_newgame [more]
"""
import argparse
import json
import shutil
import struct
import sys
import time
from pathlib import Path

DECOMP = Path('/Users/abe/Documents/Extermination.nosync/Extermination')
sys.path.insert(0, str(DECOMP / 'tools'))
GUIDED = DECOMP / 'build/guided_play'
OUT = DECOMP / 'build/drive_replay_guided'
# The user's own takes (build/human_play; --human): read only, replayed as the
# decomp's record_original_play.py verify does, tables in their own folder.
HUMAN = DECOMP / 'build/human_play'
HUMAN_OUT = DECOMP / 'build/drive_replay_takes'
SCRATCH = DECOMP / 'build/drive_replay_guided_scratch'

LANES, LANE_STRIDE, SEQ = 0x281FD0, 0x60, 0x282150
EXTEND = 40            # ticks sampled past the take's end for a read still in flight
SPANS = [dict(name='counter', src='ee', addr='0x70003B64', len=4),
         dict(name='seq', src='ee', addr=hex(SEQ), len=0x40),
         dict(name='lanes', src='ee', addr=hex(LANES), len=LANE_STRIDE * 3)]


def chain_takes():
    """The kept takes of the guided chain, in order (chain.json)."""
    return [t['name'] for t in json.loads((GUIDED / 'chain.json').read_text())['takes']
            if (GUIDED / t['name'] / 'end.json').exists()]


def extract(samples, last_counter):
    """samples: [(counter, seq bytes, lane bytes)] per main-loop tick (its top).
    Returns (reads, omitted): reads (counter, sector, count, fields) issued in a
    tick at or before the take's last, keyed by the issuing tick's counter."""
    reads, omitted = [], []
    for k in range(1, len(samples)):
        (counter, a, _), (_, b, lanes) = samples[k - 1], samples[k]
        if not (a[7] == 1 and b[7] == 2):
            continue
        if counter > last_counter:
            continue                # the next take's
        lane = b[8]
        sector, count = struct.unpack_from('<ii', lanes, LANE_STRIDE * lane + 0x30)
        j = k + 1
        while j < len(samples) and samples[j][1][7] == 2 and samples[j][1][8] == lane:
            j += 1
        if j >= len(samples):
            omitted.append(dict(counter=counter, sector=sector, count=count, why='in flight at the end'))
            continue
        if samples[j][2][LANE_STRIDE * lane + 3] != 2:
            omitted.append(dict(counter=counter, sector=sector, count=count, why='phase 2 left without done'))
            continue
        reads.append((counter, sector, count, j - k - 1))
    return reads, omitted


def table_rows(path):
    import pcsx2_session as session
    if str(session.FORK_PY) not in sys.path:
        sys.path.insert(0, str(session.FORK_PY))
    from pcsx2dbg.tables import Table
    return Table(path).rows()


def u32(b):
    return struct.unpack('<I', bytes(b))[0]


def write_reads(name, out, exact, replay, seconds, takes=GUIDED):
    """<take>.reads (or .reads.diverged) and receipt.json from the kept raw table
    out/drv.tbl. An earlier table that differs is kept beside as
    <file>.superseded-<time> (a recording is never deleted)."""
    take = takes / name
    start, end = (json.loads((take / f).read_text()) for f in ('start.json', 'end.json'))
    # the take's per-tick table with the counter (an older user take has it in its
    # trace's table: record_original_play.py trace)
    take_rows = next(rows for rows in (table_rows(take / f) for f in ('rows.tbl', 'trace_rows.tbl')
                                       if (take / f).exists()) if 'counter' in rows[0])
    first_counter, last_counter = u32(take_rows[0]['counter']), u32(take_rows[-1]['counter'])
    samples = [(u32(r['counter']), bytes(r['seq']), bytes(r['lanes'])) for r in table_rows(out / 'drv.tbl')]
    # The table's counters must be take.rec's: row k of the take's rows.tbl.
    same = sum(1 for a, b in zip(samples, take_rows) if a[0] == u32(b['counter']))
    reads, omitted = extract(samples, last_counter)
    fname = f'{name}.reads' if exact else f'{name}.reads.diverged'
    text = f'# {name}: counter sector count fields (fork replay of the kept take; comparison only)\n' + \
        ''.join('%d %d %d %d\n' % x for x in reads)
    path = out / fname
    if path.exists() and path.read_text() != text:
        path.rename(out / f'{fname}.superseded-{time.strftime("%Y%m%d-%H%M%S")}')
    path.write_text(text)
    info = dict(take=name, tool='tools/record_drive_replay_guided.py', recorded=time.strftime('%Y-%m-%d %H:%M:%S'),
                exact=exact, start_vsync=start['vsync'], end_vsync=end['vsync'], ticks=len(take_rows),
                samples=len(samples), samples_on_take_counters=same, first_counter=first_counter,
                last_counter=last_counter, reads=len(reads), omitted=omitted,
                fields=sorted({x[3] for x in reads}), seconds=seconds, replay=replay)
    (out / 'receipt.json').write_text(json.dumps(info, indent=1) + '\n')
    print(f'{name}: {"EXACT" if exact else "DIFFERS"} replay, {len(take_rows)} ticks, {len(reads)} reads '
          f'(fields {info["fields"]}), {len(omitted)} omitted, {same}/{min(len(samples), len(take_rows))} '
          f'samples on take.rec\'s counters, {seconds} s', flush=True)
    return info


def record(name):
    import record_guided_run as rgr
    rop = rgr.rop
    take = rgr.take_dir(name)
    out = OUT / name
    if (out / 'drv.tbl').exists():
        print(f'{name}: already recorded ({out}); kept as it is', flush=True)
        return None
    start, end = (json.loads((take / f).read_text()) for f in ('start.json', 'end.json'))
    rows = rop.records(take)
    card = take / 'card_start.ps2'
    scratch = SCRATCH / f'{name}-{time.strftime("%Y%m%d-%H%M%S")}'
    scratch.mkdir(parents=True)
    t0 = time.monotonic()
    with rgr.GuidedSession(Path(start['state_abs']), scratch, card if card.exists() else None, 'replay',
                           blocks=rgr.blocks_of(start)) as s:
        c = s._client
        got0 = s.start_machine
        assert {k: got0[k] for k in ('vsync', 'hashes')} == {k: start[k] for k in ('vsync', 'hashes')}, \
            f'{name}: the replay does not start from the take\'s machine'
        c.call('input_timeline', entries=rgr.timeline(rows, start['vsync'], rgr.SHIFT), index='vsync',
               mode='replace')
        rop.schedule_card_events(c, take, s.server_version)
        c.call('table_add', name='drv', path='drv.tbl', trigger='tick', spans=SPANS)
        c.call('run', timeout=86400 + 60, until={'vsyncs': end['vsync'] - start['vsync']}, timeout_s=86400)
        got = rop.machine(c)
        c.call('run', timeout=3600, until={'vsyncs': EXTEND}, timeout_s=3600)
        c.call('table_remove', name='drv')
        rop.disarm(c, s.root, scratch, 'replay_')
        out.mkdir(parents=True, exist_ok=True)
        for f in ('drv.tbl', 'drv.tbl.json'):        # the raw recording, kept with the table
            shutil.copyfile(s.root / f, out / f)
    exact = got['hashes'] == end['hashes'] and got['ee_cycle'] == end['ee_cycle']
    return write_reads(name, out, exact, dict(hashes=got['hashes'], ee_cycle=got['ee_cycle'], vsync=got['vsync']),
                       round(time.monotonic() - t0, 1))


# LOADER: the two log probes (sceCdRead's entry; 00200730 right after its
# 00112D18(1) call returns, v0 = busy) and the call records' layout
# (pcsx2-fork docs FORMATS.md section 4: 64 bytes).
LOADER_READ_PC, LOADER_POLL_PC = 0x00112440, 0x00200740
CALL = struct.Struct('<IIIIQIIIIIIIIII')     # seq vsync tick cpu cycle pc ra sp a0 a1 a2 a3 v0 id reserved


def loader_files():
    """{file id: (first lsn, sectors)} of the user's disc: 0 INDEX.IDX, 1 DATA.DAT,
    0x10 + n the area file D_0028A3C0[n] (the boot's table from the pinned ELF's
    names: the port's em_module_loader file ids)."""
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    import export_module_loader as eml
    disc = eml.Disc(DECOMP / 'Extermination-rebuilt.iso')
    files = {0: (disc.index[0], (disc.index[1] + 2047) // 2048), 1: (disc.data[0], (disc.data[1] + 2047) // 2048)}
    areas, _names, _arena, _bases = eml.boot_tables(disc, DECOMP / 'config/SCUS_971.12')
    for n, (lsn, size) in enumerate(areas):
        files[0x10 + n] = (lsn, (size + 2047) // 2048)
    return files


def loader_extract(calls, tick_counter, last_counter, files):
    """(reads, omitted): reads (counter, file, sector, sectors, busy) issued at or
    before the take's last counter, in counter order."""
    reads, omitted, open_read = [], [], None
    for rec in calls:
        tick, pc, a0, a1, v0 = rec[2], rec[5], rec[8], rec[9], rec[12]
        if pc == LOADER_READ_PC:
            if open_read is not None:
                omitted.append(dict(open_read, why='a new read before this one\'s done poll'))
            open_read = dict(tick=tick, lsn=a0, sectors=a1)
        elif pc == LOADER_POLL_PC and open_read is not None and v0 == 0:
            i, j = tick_counter.get(open_read['tick']), tick_counter.get(tick)
            read, open_read = open_read, None
            if i is None or j is None:
                omitted.append(dict(read, why='tick not in the replay\'s rows'))
                continue
            if i > last_counter:
                continue                    # the next take's
            fid = next((f for f, (lsn, n) in files.items() if lsn <= read['lsn'] < lsn + n), None)
            if fid is None:
                omitted.append(dict(read, counter=i, why='lsn in no loader file'))
                continue
            reads.append((i, fid, read['lsn'] - files[fid][0], read['sectors'], j - i - 1))
    if open_read is not None:
        omitted.append(dict(open_read, why='in flight at the end'))
    return sorted(reads), omitted


def write_loader(name, out, exact=None, replay=None, seconds=None):
    """<take>.loader (or .loader.diverged) and loader_receipt.json from the kept raw
    recording (loader_calls.bin, loader_rows.tbl)."""
    take = GUIDED / name
    receipt = out / 'loader_receipt.json'
    if exact is None:
        old = json.loads(receipt.read_text())
        exact, replay, seconds = old['exact'], old['replay'], old['seconds']
    take_rows = table_rows(take / 'rows.tbl')
    last_counter = u32(take_rows[-1]['counter'])
    rows = table_rows(out / 'loader_rows.tbl')
    tick_counter = {int(r['tick']): u32(r['counter']) for r in rows}
    blob = (out / 'loader_calls.bin').read_bytes()
    calls = sorted((CALL.unpack_from(blob, k) for k in range(0, len(blob) - CALL.size + 1, CALL.size)),
                   key=lambda r: r[0])
    reads, omitted = loader_extract(calls, tick_counter, last_counter, loader_files())
    fname = f'{name}.loader' if exact else f'{name}.loader.diverged'
    text = f'# {name}: L counter file sector sectors busy (fork replay of the kept take; comparison only)\n' + \
        ''.join('L %d %d %d %d %d\n' % x for x in reads)
    path = out / fname
    if path.exists() and path.read_text() != text:
        path.rename(out / f'{fname}.superseded-{time.strftime("%Y%m%d-%H%M%S")}')
    path.write_text(text)
    info = dict(take=name, tool='tools/record_drive_replay_guided.py --loader',
                recorded=time.strftime('%Y-%m-%d %H:%M:%S'), exact=exact, calls=len(calls), reads=len(reads),
                omitted=omitted, busy=sorted({x[4] for x in reads}), seconds=seconds, replay=replay)
    receipt.write_text(json.dumps(info, indent=1) + '\n')
    print(f'{name}: {"EXACT" if exact else "DIFFERS"} replay, {len(reads)} loader reads (busy {info["busy"]}), '
          f'{len(omitted)} omitted, {seconds} s', flush=True)
    return info


def probe_replay(name, pcs, binname, tag, split=None):
    """Replays the kept take on the fork with inline log probes at pcs (no
    observer effect) into out/<binname>, with the replay's rows as
    out/<tag>_rows.tbl(.json). With split {file: (tag, pcs)} the log is written
    as one file per entry instead (its records at those pcs), each with the
    rows as <tag>_rows.tbl. Returns (out, exact, replay, seconds)."""
    import record_guided_run as rgr
    rop = rgr.rop
    take = rgr.take_dir(name)
    out = OUT / name
    start, end = (json.loads((take / f).read_text()) for f in ('start.json', 'end.json'))
    rows = rop.records(take)
    card = take / 'card_start.ps2'
    scratch = SCRATCH / f'{name}-{tag}-{time.strftime("%Y%m%d-%H%M%S")}'
    scratch.mkdir(parents=True)
    t0 = time.monotonic()
    with rgr.GuidedSession(Path(start['state_abs']), scratch, card if card.exists() else None, 'replay',
                           blocks=rgr.blocks_of(start)) as s:
        c = s._client
        got0 = s.start_machine
        assert {k: got0[k] for k in ('vsync', 'hashes')} == {k: start[k] for k in ('vsync', 'hashes')}, \
            f'{name}: the replay does not start from the take\'s machine'
        c.call('input_timeline', entries=rgr.timeline(rows, start['vsync'], rgr.SHIFT), index='vsync',
               mode='replace')
        rop.schedule_card_events(c, take, s.server_version)
        c.call('probe_add', pcs=pcs, action='log', path=binname)
        c.call('run', timeout=86400 + 60, until={'vsyncs': end['vsync'] - start['vsync']}, timeout_s=86400)
        got = rop.machine(c)
        c.call('run', timeout=3600, until={'vsyncs': EXTEND}, timeout_s=3600)
        c.call('probe_remove', all=True)
        rop.disarm(c, s.root, scratch, 'replay_')
        out.mkdir(parents=True, exist_ok=True)
        if split:
            blob = (s.root / binname).read_bytes()
            recs = [blob[k:k + CALL.size] for k in range(0, len(blob) - CALL.size + 1, CALL.size)]
            for fname, (ftag, fpcs) in split.items():
                (out / fname).write_bytes(b''.join(r for r in recs if CALL.unpack(r)[5] in fpcs))
                for f in ('rows.tbl', 'rows.tbl.json'):
                    shutil.copyfile(scratch / f'replay_{f}', out / f'{ftag}_{f}')
        else:
            shutil.copyfile(s.root / binname, out / binname)
            for f in ('rows.tbl', 'rows.tbl.json'):
                shutil.copyfile(scratch / f'replay_{f}', out / f'{tag}_{f}')
    exact = got['hashes'] == end['hashes'] and got['ee_cycle'] == end['ee_cycle']
    return out, exact, dict(hashes=got['hashes'], ee_cycle=got['ee_cycle'], vsync=got['vsync']), \
        round(time.monotonic() - t0, 1)


def record_loader(name):
    if (OUT / name / 'loader_calls.bin').exists():
        print(f'{name}: loader already recorded ({OUT / name}); kept as it is', flush=True)
        return None
    out, exact, replay, seconds = probe_replay(name, [LOADER_READ_PC, LOADER_POLL_PC], 'loader_calls.bin',
                                               'loader')
    return write_loader(name, out, exact, replay, seconds)


# BANK: the area loads' sound-bank transfer (001FB370 -> 001FB3E0 -> 001FB910;
# IOP_STREAM.md "The sound-bank transfer"). Log probes at the start of
# 001FB3E0's state 0 (the bank's first call: jtbl_0026EBF0's case 0), at the
# return of 001FB910's sceSifDmaStat call (v0 >= 0: pending), at the returns of
# 001FB3E0's three 00119450(0) calls (the IOP's acknowledgement: v0 1 acknowledged,
# 0 not yet) and at the returns of 001FB370's four call sites in the boot ELF
# (v0 != 0: the bank's end, the load's last call). The answers, in call order,
# are what the port's model answers at host speed (and, for the New Game's
# 0x492D0-byte transfer, by the measured rule); a load's sequence is the fork's.
BANK_START_PC, BANK_STAT_PC = 0x001FB450, 0x001FB9AC
BANK_ACK_PCS = (0x001FB5DC, 0x001FB700, 0x001FB720)
BANK_END_PCS = (0x001AB868, 0x001AC574, 0x001FF7F8, 0x001FFBA4)


def bank_extract(calls, tick_counter, last_counter):
    """(loads, opened, cont, omitted). loads: (counter, end, calls, answers,
    counters) of the loads whose state 0 ran at or before the take's last
    counter and that ended in the recording. answers: p / d the status query
    pending / done, 0 / 1 / n the acknowledgement query not yet / acknowledged
    / negative. A load still running at the take's last counter (it ends in
    the next take) is `opened` (counter, calls, answers) with its calls and
    answers up to that counter; the end of a load begun in the previous take
    is `cont` (end, calls, answers) from the take's first tick (guided_table
    joins the two)."""
    loads, omitted, cur, cont, opened = [], [], None, None, None
    head = dict(calls=0, answers='')               # a load begun before the take
    for rec in calls:
        tick, pc, v0 = rec[2], rec[5], rec[12]
        sv0 = v0 - (1 << 32) if v0 & 0x80000000 else v0
        counter = tick_counter.get(tick)
        if pc == BANK_START_PC:
            if cur is not None:
                omitted.append(dict(cur, why='a new bank before this one ended'))
            head = None
            cur = dict(tick=tick, calls=0, answers='', calls_in=0, answers_in='')
            continue
        tgt = cur if cur is not None else head
        if tgt is None:
            continue
        inside = counter is not None and counter <= last_counter
        if pc == BANK_STAT_PC or pc in BANK_ACK_PCS:
            a = ('p' if sv0 >= 0 else 'd') if pc == BANK_STAT_PC else '1' if sv0 == 1 else '0' if sv0 == 0 else 'n'
            tgt['answers'] += a
            if cur is not None and inside:
                cur['answers_in'] += a
        elif pc in BANK_END_PCS:
            tgt['calls'] += 1
            if cur is not None and inside:
                cur['calls_in'] += 1
            if v0 == 0:
                continue
            if cur is None:                         # the end of the previous take's load
                if head['calls']:
                    cont = (v0, head['calls'], head['answers'])
                head = None
                continue
            load, cur = cur, None
            i = tick_counter.get(load['tick'])
            if i is None:
                omitted.append(dict(load, why='tick not in the replay\'s rows'))
                continue
            if i > last_counter:
                continue                            # the next take's
            if counter is None or counter > last_counter:
                opened = (i, load['calls_in'], load['answers_in'])   # ends in the next take
                continue
            loads.append((i, v0, load['calls'], load['answers'], counter - i))
    if cur is not None:
        i = tick_counter.get(cur['tick'])
        if i is not None and i <= last_counter:
            opened = (i, cur['calls_in'], cur['answers_in'])
        elif i is None:
            omitted.append(dict(cur, why='not ended at the end'))
        # else: begun past the take's last counter, the next take's
    return sorted(loads), opened, cont, omitted


def write_bank(name, out, exact=None, replay=None, seconds=None):
    """<take>.bank (or .bank.diverged) and bank_receipt.json from the kept raw
    recording (bank_calls.bin, bank_rows.tbl). Lines: "B counter end calls
    answers" a load inside the take; "O counter calls answers" a load still
    running at the take's end (its part up to the take's last counter); "C end
    calls answers" the rest of the previous take's load, from the take's start."""
    take = GUIDED / name
    receipt = out / 'bank_receipt.json'
    if exact is None:
        old = json.loads(receipt.read_text())
        exact, replay, seconds = old['exact'], old['replay'], old['seconds']
    last_counter = u32(table_rows(take / 'rows.tbl')[-1]['counter'])
    tick_counter = {int(r['tick']): u32(r['counter']) for r in table_rows(out / 'bank_rows.tbl')}
    blob = (out / 'bank_calls.bin').read_bytes()
    calls = sorted((CALL.unpack_from(blob, k) for k in range(0, len(blob) - CALL.size + 1, CALL.size)),
                   key=lambda r: r[0])
    loads, opened, cont, omitted = bank_extract(calls, tick_counter, last_counter)
    fname = f'{name}.bank' if exact else f'{name}.bank.diverged'
    text = f'# {name}: B counter end calls answers (fork replay of the kept take; comparison only; ' \
        'answers p/d status pending/done, 0/1/n acknowledgement; O: running at the take\'s end, C: the ' \
        'previous take\'s load\'s rest)\n' + \
        ('C %d %d %s\n' % cont if cont else '') + \
        ''.join('B %d %d %d %s\n' % x[:4] for x in loads) + \
        ('O %d %d %s\n' % opened if opened else '')
    path = out / fname
    if path.exists() and path.read_text() != text:
        path.rename(out / f'{fname}.superseded-{time.strftime("%Y%m%d-%H%M%S")}')
    path.write_text(text)
    info = dict(take=name, tool='tools/record_drive_replay_guided.py --bank',
                recorded=time.strftime('%Y-%m-%d %H:%M:%S'), exact=exact, calls=len(calls),
                loads=[dict(counter=x[0], end=x[1], calls=x[2], answers=x[3], counters=x[4]) for x in loads],
                opened=opened, cont=cont, omitted=omitted, seconds=seconds, replay=replay)
    receipt.write_text(json.dumps(info, indent=1) + '\n')
    print(f'{name}: {"EXACT" if exact else "DIFFERS"} replay, {len(loads)} bank loads '
          f'({[(x[2], x[4]) for x in loads]} calls/counters){", one running at the end" if opened else ""}'
          f'{", the previous take\'s load ended" if cont else ""}, {len(omitted)} omitted, {seconds} s', flush=True)
    return info


def record_loader_bank(name):
    """--loader --bank: one replay with both probe sets, written as the two
    recordings (loader_calls.bin, bank_calls.bin, each with its rows)."""
    have = [f for f in ('loader_calls.bin', 'bank_calls.bin') if (OUT / name / f).exists()]
    if len(have) == 2:
        print(f'{name}: loader and bank already recorded ({OUT / name}); kept as they are', flush=True)
        return None
    if have:
        return record_bank(name) if have == ['loader_calls.bin'] else record_loader(name)
    lpcs, bpcs = (LOADER_READ_PC, LOADER_POLL_PC), (BANK_START_PC, BANK_STAT_PC, *BANK_ACK_PCS, *BANK_END_PCS)
    out, exact, replay, seconds = probe_replay(name, [*lpcs, *bpcs], 'loader_bank_calls.bin', 'loader',
                                               split={'loader_calls.bin': ('loader', lpcs),
                                                      'bank_calls.bin': ('bank', bpcs)})
    a = write_loader(name, out, exact, replay, seconds)
    b = write_bank(name, out, exact, replay, seconds)
    return dict(exact=a['exact'] and b['exact'])


def record_bank(name):
    if (OUT / name / 'bank_calls.bin').exists():
        print(f'{name}: bank already recorded ({OUT / name}); kept as it is', flush=True)
        return None
    out, exact, replay, seconds = probe_replay(
        name, [BANK_START_PC, BANK_STAT_PC, *BANK_ACK_PCS, *BANK_END_PCS], 'bank_calls.bin', 'bank')
    return write_bank(name, out, exact, replay, seconds)


def reextract(name, human=False):
    """The table again from the kept raw recording (no emulator)."""
    out = (HUMAN_OUT if human else OUT) / name
    old = json.loads((out / 'receipt.json').read_text())
    return write_reads(name, out, old['exact'], old['replay'], old['seconds'], HUMAN if human else GUIDED)


def record_human(name):
    """One of the user's takes: its replay as record_original_play.py's (that
    tool's session, card and timeline rule), its files read only; every output
    goes to the scratch folder and HUMAN_OUT."""
    import record_original_play as rop
    take = HUMAN / name
    out = HUMAN_OUT / name
    if (out / 'receipt.json').exists():
        print(f'{name}: already recorded ({out}); kept as it is', flush=True)
        return None
    if (out / 'drv.tbl').exists():             # a recording whose table was never written: kept beside
        stamp = time.strftime('%Y%m%d-%H%M%S')
        for f in ('drv.tbl', 'drv.tbl.json'):
            (out / f).rename(out / f'{f}.incomplete-{stamp}')
    start, end = (json.loads((take / f).read_text()) for f in ('start.json', 'end.json'))
    rows = rop.records(take)
    scratch = SCRATCH / f'{name}-{time.strftime("%Y%m%d-%H%M%S")}'
    scratch.mkdir(parents=True)
    t0 = time.monotonic()
    with rop.Session(lock_wait=rop.LOCK_WAIT) as s:
        cfg = rop.replay_config(Path(start['state']), rop.blocks_of(start))
        rop.use_card(s, take, cfg)
        inst = s.launch('replay', cfg)
        c = inst.client()
        c.wait_vm(120)
        rop.arm(c)
        got0 = rop.machine(c)
        assert {k: got0[k] for k in ('vsync', 'hashes')} == {k: start[k] for k in ('vsync', 'hashes')}, \
            f'{name}: the replay does not start from the take\'s machine'
        c.call('input_timeline', entries=rop.timeline(rows), index='vsync', mode='replace')
        c.call('table_add', name='drv', path='drv.tbl', trigger='tick', spans=SPANS)
        c.call('run', timeout=86400 + 60, until={'vsyncs': end['vsync'] - start['vsync']}, timeout_s=86400)
        got = rop.machine(c)
        c.call('run', timeout=3600, until={'vsyncs': EXTEND}, timeout_s=3600)
        c.call('table_remove', name='drv')
        root = Path(inst.info().get('root') or inst.scratch / 'root')
        rop.disarm(c, root, scratch, 'replay_')
        out.mkdir(parents=True, exist_ok=True)
        for f in ('drv.tbl', 'drv.tbl.json'):
            shutil.copyfile(root / f, out / f)
    exact = got['hashes'] == end['hashes'] and got['ee_cycle'] == end['ee_cycle']
    return write_reads(name, out, exact, dict(hashes=got['hashes'], ee_cycle=got['ee_cycle'], vsync=got['vsync']),
                       round(time.monotonic() - t0, 1), HUMAN)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('takes', nargs='*')
    ap.add_argument('--all', action='store_true', help='every kept take of chain.json not yet recorded')
    ap.add_argument('--human', action='store_true', help='the named takes are the user\'s own (build/human_play; '
                    'read only), tables in build/drive_replay_takes/')
    ap.add_argument('--reextract', action='store_true', help='derive the named takes\' tables again from their kept '
                    'raw recordings (drv.tbl), no emulator')
    ap.add_argument('--loader', action='store_true', help='record (or, with --reextract, derive again) the named '
                    'takes\' screen-module loader reads instead (<take>.loader: the area loads\' and the bank '
                    'loads\' per-read busy polls; LOADER below)')
    ap.add_argument('--bank', action='store_true', help='record (or, with --reextract, derive again) the named '
                    'takes\' sound-bank transfers instead (<take>.bank: each area load\'s 001FB370 answers; BANK '
                    'below)')
    a = ap.parse_args()
    takes = chain_takes() if a.all else a.takes
    if not takes:
        ap.error('name takes or --all')
    bad = 0
    t0 = time.monotonic()
    for name in takes:
        if a.bank and a.loader and not a.reextract:
            assert not a.human, '--loader --bank records Claude\'s guided takes only'
            info = record_loader_bank(name)
        elif a.bank:
            assert not a.human, '--bank records Claude\'s guided takes only'
            info = write_bank(name, OUT / name) if a.reextract else record_bank(name)
        elif a.loader:
            assert not a.human, '--loader records Claude\'s guided takes only'
            info = write_loader(name, OUT / name) if a.reextract else record_loader(name)
        else:
            info = reextract(name, a.human) if a.reextract else record_human(name) if a.human else record(name)
        bad += info is not None and not info['exact']
    print(f'{len(takes)} take(s) in {time.monotonic() - t0:.0f} s; {bad} replay(s) differ', flush=True)
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
