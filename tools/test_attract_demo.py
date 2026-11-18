#!/usr/bin/env python3
"""The attract demo against the original's capture (docs/ATTRACT_DEMO.md).

Runs the port headless from the boot (EM_STARTUP_TEST=attract: the intro
skipped, the title left idle until 001AC070 enters state 3), lets demo 0
(AREA06) play its recorded pads, holds CROSS at record 300 (the 0x9F0 exit
of 001ACA20 sub 1) and requires the title back. Records 1..300 are compared
with the original's capture (decomp build/demo_ref/run1/rows.jsonl, made by
tools/capture_attract_demo.py): the pad the demo fed, the area block, the
player's position and yaw, the fade and the rand() state, record by record.
EM_TEST_FULL=1 also plays each of demos 0, 1 and 2 to its end (0xE10
records; demos 1 and 2 as constructed runs, EM_ATTRACT_DEMO_ID) and requires
all 3600 records of each equal to the capture (section 4)."""
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CAPTURE = ROOT.parent / 'Extermination/build/demo_ref/run1/rows.jsonl'
BIN = ROOT / 'build/extermination'


def run(name, extra):
    out = ROOT / 'build/attract_demo_test'
    out.mkdir(parents=True, exist_ok=True)
    log = out / f'{name}.log'
    env = {k: v for k, v in os.environ.items() if not k.startswith('EM_')}
    env.update(EM_STARTUP_TEST='attract', EM_HEADLESS='1', EM_UNCAPPED='1', EM_MUTE='1', EM_NO_RENDER='1')
    env.update(extra)
    with log.open('w') as f:
        code = subprocess.run([str(BIN)], cwd=ROOT, env=env, stdout=f, stderr=subprocess.STDOUT, timeout=900).returncode
    text = log.read_text(errors='replace')
    if code != 0 or 'FAULT' in text or 'the title is back' not in text:
        sys.exit(f'test_attract_demo: {name}: the port did not return to the title (exit {code}); see {log}')
    return log


def compare(log, demo, first=None, require=None):
    cmd = [sys.executable, str(ROOT / 'tools/compare_attract_demo.py'), str(log), str(CAPTURE), '--demo', str(demo)]
    if first:
        cmd += ['--first', str(first)]
    r = subprocess.run(cmd, capture_output=True, text=True)
    print(r.stdout.rstrip())
    if require is not None and f'records 1..{require} compared: {require}/{require} equal' not in r.stdout:
        return 1
    return r.returncode


def main():
    if not CAPTURE.is_file():
        sys.exit(f'test_attract_demo: no original capture {CAPTURE} (run tools/capture_attract_demo.py --demos 3)')
    log = run('exit300', {'EM_ATTRACT_EXIT_AT': '300'})
    if compare(log, 0, 300) != 0:
        sys.exit('test_attract_demo: FAIL (demo 0, records 1..300)')
    if os.environ.get('EM_TEST_FULL') == '1':
        for demo, extra in ((0, {}), (1, {'EM_ATTRACT_DEMO_ID': '1'}), (2, {'EM_ATTRACT_DEMO_ID': '2'})):
            if compare(run(f'demo{demo}', extra), demo, require=3600) != 0:
                sys.exit(f'test_attract_demo: FAIL (demo {demo} to its end)')
    print('test_attract_demo: PASS (demo 0: records 1..300 equal to the original, CROSS exit back to the title' +
          ('; demos 0, 1 and 2: 3600/3600 records equal' if os.environ.get('EM_TEST_FULL') == '1' else '') + ')')


if __name__ == '__main__':
    main()
