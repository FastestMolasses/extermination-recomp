#!/usr/bin/env python3
"""AREA03 route gaps: original code identity, ordered memory and worker replay.

Standalone and UNWIRED. Uses the existing level-14 comparator unchanged.
AREA14's 0x825AA0 (word-equal to 0x825290) runs every 0x825290 case again
on AREA14's overlay header and text loaded over the same images (no AREA14
capture exists): the translation em_area03_route_00825290 binds it.
AREA20's 0x825F20 is 0x825290 word for word as well; its cases run on
AREA20's overlay header and text the same way (lane PORT-A20).
EM_TEST_FULL=1 sweeps every failure site; the default still covers every
reachable word. EM_AREA03_ROUTE_ONLY is diagnostic, never certification.
"""
import ctypes as C
import os
import re
import struct
import test_area01_overlay_reference as A1
from pathlib import Path
import test_level14_port_reference as O
import reference_root as refs
ROOT = Path(__file__).resolve().parents[1]
U, I, F, S = O.U, O.I, O.F, O.S
FUNCS = {
    0x00823580: ('em_area03_route_00823580',0x20,[],None),
    0x00823930: ('em_area03_route_00823930',0x13F4,[U],None),
    0x00825290: ('em_area03_route_00825290',0x198,[U],None),
    0x00826270: ('em_area03_route_00826270',0xB8,[],I),
    0x00826330: ('em_area03_route_00826330',0x10,[U, U, U],I),
    0x00826860: ('em_area03_route_00826860',0x18,[],I),
    0x00826880: ('em_area03_route_00826880',0x2C,[U],I),
    0x008268B0: ('em_area03_route_008268B0',0x2C,[U],I),
    # AREA14's 0x825AA0 is 0x825290 word for word (no overlay-relocated
    # word in either); the same translation runs it, checked on AREA14's text
    0x00825AA0: ('em_area03_route_00825290',0x198,[U],None),
    # AREA20's 0x825F20: the same, checked on AREA20's text
    0x00825F20: ('em_area03_route_00825290',0x198,[U],None),
}
OVERLAY_OF = {entry:(0x0B if entry==0x825AA0 else 0x11 if entry==0x825F20 else 4) for entry in FUNCS}
OVERLAY_FILES = {4:ROOT.parent/'Extermination/extract/OVERLAY/AREA03.BIN',
                 0x0B:ROOT.parent/'Extermination/extract/OVERLAY/AREA14.BIN',
                 0x11:ROOT.parent/'Extermination/extract/OVERLAY/AREA20.BIN'}
HOOKS = [
    ('w_00102760',0x00102760,[('u', 4), ('u', 5)],None),
    ('w_001028D0',0x001028D0,[('u', 4), ('u', 5), ('u', 6)],None),
    ('w_00102948',0x00102948,[('u', 4), ('u', 5)],None),
    ('w_00102958',0x00102958,[('u', 4), ('u', 5)],None),
    ('w_0011DBB8',0x0011DBB8,[('f', 12)],'f'),
    ('w_0011DF78',0x0011DF78,[('f', 12)],'f'),
    ('w_0011E2A8',0x0011E2A8,[('f', 12)],'f'),
    ('w_0011E520',0x0011E520,[('f', 12)],'f'),
    ('w_0011E748',0x0011E748,[('f', 12)],'f'),
    ('w_00122BB8',0x00122BB8,[],'i'),
    ('w_001A2370',0x001A2370,[('u', 4), ('u', 5)],None),
    ('w_001AFA90',0x001AFA90,[('i', 4)],'u'),
    ('w_001AFC10',0x001AFC10,[('u', 4)],None),
    ('w_001B0FD0',0x001B0FD0,[('u', 4)],'i'),
    ('w_001B1190',0x001B1190,[('i', 4)],None),
    ('w_001B11E0',0x001B11E0,[('i', 4)],'i'),
    ('w_001B1470',0x001B1470,[('f', 12)],'f'),
    ('w_001B17A0',0x001B17A0,[('u', 4)],'i'),
    ('w_001C6380',0x001C6380,[('u', 4)],None),
    ('w_001EFD90',0x001EFD90,[('i', 4), ('u', 5), ('u', 6)],'i'),
    ('w_001EFE00',0x001EFE00,[('i', 4), ('u', 5)],'i'),
    ('w_001FBD50',0x001FBD50,[('u', 4), ('i', 5), ('i', 6), ('f', 12)],'i'),
    ('w_00824D30',0x00824D30,[('u', 4), ('u', 5)],'i'),
    ('w_00825200',0x00825200,[('u', 4)],None),
 ]
BEATS = ('a19e_07_door25','a03_00_elevator')
BEAT = BEATS[0]
# This 256 KiB window is zero in both AREA03 inputs; the loader asserts it.
FREE = 0x1E61000
A = FREE+0x1000


def configure():
    O.FREE = FREE
    O.FUNCS,O.HOOKS = FUNCS,HOOKS
    O.OVERLAY_OF,O.OVERLAY_FILES = OVERLAY_OF,OVERLAY_FILES
    O.OUT = ROOT/'build/levels/area03_route'
    O.SOURCES = ('em_area03_route_misc.c','em_area03_route_turret.c')
    O.BEATS = BEATS
    O.BEAT_DIRS = {b:refs.v263('s87/route_'+b.split('_')[0]+'/'+b,
        'fork route is not recorded from a04b_00; retained original captures explicitly selected') for b in BEATS}
    O.RESIDENT = {b:4 for b in BEATS}
    O.BY_ADDRESS = {a:(n,args,res) for n,a,args,res in HOOKS}
    O.BY_NAME = {n:a for n,a,_,_ in HOOKS}
    O.HOOK_NAMES = [h[0] for h in HOOKS]+['w_callback']
    O.Hooks = type('Hooks',(C.Structure,),{'_fields_':
        [('ctx',C.c_void_p),('bytes',O.BYTES_FN)] +
        [(name,O.hook_proto(args,res)) for name,_,args,res in HOOKS] +
        [('w_callback',O.CALLBACK_FN)]})
    O.SELF_ENTRIES,O.HEAVY = {0x823930,0x825290,0x825AA0,0x825F20},set()
    O.RETURN_SITES = O.RETURN_SITES_FULL = (0x826270,0x826330,0x826860,0x826880,0x8268B0)
    O.CONTRACT_UNREACHABLE = {}
    O.LOCAL_STORES = set()
    O.DEFAULT_ARGS = {e:[A]*len(kinds) for e,(_,_,kinds,_) in FUNCS.items()}
    O.image_for = lambda e:O.CAPTURES[('a14/'+BEAT) if e==0x825AA0 else ('a20/'+BEAT) if e==0x825F20 else BEAT][0]
    loaded = O.OVERLAY_FILES
    O.OVERLAY_FILES = {4:OVERLAY_FILES[4]}   # the captures hold AREA03
    O.CAPTURES.clear();O.CAPTURES.update(O.load_captures(O.read_elf()))
    O.OVERLAY_FILES = loaded
    O.JUMP_TABLES = {};O.TABLE_BYTES = frozenset()
    # Designed twin fixtures retain AREA01 actor data and load original
    # AREA03 text. They are not represented as original AREA03 captures.
    A1.CAPTURES.clear();A1.CAPTURES.update(A1.load_captures(O.read_elf()))
    A1.Scribble=O.Scribble
    data=OVERLAY_FILES[4].read_bytes();length=0x40+struct.unpack_from('<I',data,12)[0]
    for beat,(ram,spad) in A1.CAPTURES.items():
        designed=bytearray(ram);designed[O.ARENA:O.ARENA+length]=data[:length]
        assert designed[O.ARENA:O.ARENA+length]==data[:length]
        O.CAPTURES['designed/'+beat]=(bytes(designed),spad)
    # AREA14 (no capture yet): its overlay header and text over every image
    # above (the AREA03 captures and the designed AREA01 ones), so the same
    # nodes run AREA14's words
    data=OVERLAY_FILES[0x0B].read_bytes();length=0x40+struct.unpack_from('<I',data,12)[0]
    assert data[4]==0x0B
    plain=[k for k in O.CAPTURES]
    for key,(ram,spad) in list(O.CAPTURES.items()):
        designed=bytearray(ram);designed[O.ARENA:O.ARENA+length]=data[:length]
        O.CAPTURES['a14/'+key]=(bytes(designed),spad)
    # AREA20 (lane PORT-A20; no capture of these images holds it): the same
    data=OVERLAY_FILES[0x11].read_bytes();length=0x40+struct.unpack_from('<I',data,12)[0]
    assert data[4]==0x11
    for key in plain:
        ram,spad=O.CAPTURES[key]
        designed=bytearray(ram);designed[O.ARENA:O.ARENA+length]=data[:length]
        O.CAPTURES['a20/'+key]=(bytes(designed),spad)


def cases():
    out=[]
    b,h,w,f=O.b8,O.h16,O.w32,O.f32b
    def adapt(label,beat,entry,args,patches=(),spad=(),queues=None,default=None,run_set=A1.RUN):
        if entry not in (0x826D40,0x828850):return
        remap={0x8282F0:0x824D30,0x8287C0:0x825200}
        queues={remap.get(a,a):v for a,v in (queues or {}).items()}
        default={remap.get(a,a):v for a,v in (default or {}).items()}
        out.append((f'twin/{len(out)}/{label}','designed/'+beat,
                    0x823930 if entry==0x826D40 else 0x825290,tuple(args),list(patches),list(spad),
                    O.Script(queues,default),set(run_set)))
    A1.targeted_cases(adapt)
    for entry in (0x823580,0x826270,0x826330,0x826860,0x826880,0x8268B0):
        for flag in (0,1,2,0xFFFFFFFF,0x80000000):
            for raw in (0,0x80000000,0x3F800000,0x42D0FFFF,0x42D10000,0x7F800000,0xFF800000,0x7FC00000):
                patches=[(0x275CA0,w(flag)),(A+8,w(raw)),(0x810354,w(raw)),(0x8105D4,w(raw)),(0x8105E4,w(raw))]
                out.append((f'callback/{entry:x}/{flag:x}/{raw:x}',BEAT,entry,tuple(O.DEFAULT_ARGS[entry]),
                            patches,[],O.Script(),set()))
    for beat in BEATS:
        ram=O.CAPTURES[beat][0]
        for entry in (0x823930,0x825290):
            for node in O.owners(ram,entry):
                out.append((f'capture/{beat}/{entry:x}/{node:x}',beat,entry,(node,),
                            [(0x275B40,w(node+0x110))],[],O.Script(),set()))
    # AREA14's twin: every case of 0x825290 again on AREA14's words
    out+=[(f'a14/{label}','a14/'+beat,0x825AA0,args,patches,spad,script,run)
          for label,beat,entry,args,patches,spad,script,run in list(out) if entry==0x825290]
    # AREA20's twin 0x825F20 (lane PORT-A20): the same on AREA20's words
    out+=[(f'a20/{label}','a20/'+beat,0x825F20,args,patches,spad,script,run)
          for label,beat,entry,args,patches,spad,script,run in list(out) if entry==0x825290]
    return out


def header_checks():
    header = (ROOT/'src/game/em_area03_route.h').read_text()
    block = header.split('/* BEGIN GENERATED HOOKS */')[1].split('/* END GENERATED HOOKS */')[0]
    found = re.findall(r'int \(\*(w_[0-9A-F]{8})\)\(void \*ctx((?:, [^,)]+)*)\);', block)
    assert [n for n,_ in found] == [h[0] for h in HOOKS]
    types = {U:'uint32_t',I:'int32_t',F:'float'}
    for (name,params),(expected,address,args,result) in zip(found,HOOKS):
        actual = [re.sub(r'\s*\b\w+$','',p.strip()).replace(' ','') for p in params.split(',')[1:]]
        assert actual == [types[k] for k,_ in args] + ([types[result]+'*'] if result else [])
    internal = (ROOT/'src/game/em_area03_route_internal.h').read_text()
    for name,address,_,_ in HOOKS:
        bodies = re.findall(r'static inline int a03r_c_%s\(.*?\n}\n' % name[2:],internal,re.S)
        assert len(bodies)==1 and f'o->h->{name}(' in bodies[0] and f'0x{address:08X}u' in bodies[0]


def entry_guards():
    touched=[]
    def refuse(*_): touched.append(1); return -1
    keep=[O.BYTES_FN(refuse), O.CALLBACK_FN(refuse)]
    fields={'ctx':None,'bytes':keep[0],'w_callback':keep[1]}
    for name,_,args,result in HOOKS:
        fields[name]=O.hook_proto(args,result)(refuse); keep.append(fields[name])
    hooks=O.Hooks(**fields)
    for entry,(symbol,_,_,kind) in FUNCS.items():
        for table,fault,with_result in ((C.byref(hooks),O.Fault(0x1234,7),True),
                                       (None,O.Fault(),True),(C.byref(hooks),None,True),
                                       (C.byref(hooks),O.Fault(),False)):
            if not with_result and kind is None: continue
            out=C.c_int32(0x5A5A5A5A); touched.clear()
            args=[table]+O.native_args(entry,O.DEFAULT_ARGS[entry])
            if kind is not None: args.append(C.byref(out) if with_result else None)
            rc=getattr(O.NATIVE,symbol)(*args,C.byref(fault) if fault is not None else None)
            assert rc==-1 and not touched and out.value==0x5A5A5A5A
            if fault is not None:
                assert (fault.address,fault.code)==((0x1234,7) if table is not None and with_result else (0,0))


def main():
    configure()
    O.NATIVE = O.build(os.environ.get("EM_AREA03_ROUTE_SOURCE"))
    header_checks()
    entry_guards()
    selected = cases()
    only = os.environ.get('EM_AREA03_ROUTE_ONLY')
    if only: selected = [c for c in selected if c[0].startswith(only)]
    O.CASES[:] = selected
    assert len({c[0] for c in selected}) == len(selected)
    results = O.run_cases(selected)
    failures = [(label, errors) for label, _, errors, _, _, _ in results if errors]
    seen = set().union(*(r[3] for r in results))
    for entry, (_, size, _, _) in FUNCS.items():
        words = O.reachable_words(O.image_for(entry), entry, size)
        missing = words - seen
        print(f'{entry:08X}: {len(words & seen)}/{len(words)} reachable words')
        if missing and not only: failures.append(('coverage', [entry, sorted(missing)]))
    if not only:
        failures += [('callee inventory', p) for p in O.callee_checks()]
        if not failures:
            sites, missing = O.hook_sites(results)
            if missing: failures.append(('unreached hooks', missing))
            for problems, runs in O.reference_mode.parallel_map(O.hook_contract_site, sites):
                if problems: failures.append(('fail-stop contract', problems))
    for label, errors in failures[:int(os.environ.get('EM_AREA03_ROUTE_SHOW', '8'))]: print('FAIL', label, errors)
    assert not failures, f'{len(failures)} failures'
    print(f'PASS {len(selected)} cases: calls, ordered accesses, whole memory, poisoned starts and fail-stop')


if __name__ == '__main__': main()
