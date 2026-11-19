#!/usr/bin/env python3
"""Run every reference oracle (tools/test_*_reference.py) in quick mode and report rot.

    python3 tools/test_sweep.py                 # all oracles, 4 jobs, 120 s each
    python3 tools/test_sweep.py -k area01 -j 2  # name filter (substring, repeatable)
    python3 tools/test_sweep.py --make          # also run the cheap `make test-*` C targets
    python3 tools/test_sweep.py --update-baseline   # rewrite tests/sweep_baseline.json
                                                    # (keeps existing reasons/owners)

Per test: PASS, FAIL (non-zero exit), TIMEOUT, or BUILD (the bridge/C compile or link
failed, so no assertion ran). Results go to build/test_sweep/<timestamp>/results.json
(plus one log per test). tests/sweep_baseline.json lists the tests known to fail
({name: {status, reason, owner}}). The exit code is 1 only for NEW failures: a test
that fails and is not in the baseline, or whose status got worse (e.g. FAIL -> BUILD).
A baselined test that passes is reported as FIXED (remove it from the baseline).
Tests that fail outside the baseline are re-run once serially, so a shared-build race
between parallel jobs is not reported as rot. COMMANDS runs an oracle the way its make
target does (a fresh log from the binary, or an expected-failure wrapper).

EM_TEST_FULL is never set here: this is the quick mode of CLAUDE.md "Tests". Nothing
opens a window.
"""
import argparse, concurrent.futures as cf, glob, json, os, re, subprocess, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BASELINE = os.path.join(ROOT, 'tests', 'sweep_baseline.json')

# Oracles that need arguments to run their default check.
# name -> list of (job suffix, args); one job per entry.
ARGS = {
    'level14_shared_player': [(f':{s}', ['--suite', s]) for s in ('closure', 'ladder', 'major2', 'heal')],
}
# Original-capture fixtures that are not named test_*_reference.py but run the same way.
FIXTURES = {
    'camera_interaction_fixture': 'test_camera_interaction_fixture.py',
}
# Oracles that read a log their make target writes from the current binary: the
# sweep runs the target (it rebuilds the binary first, incrementally), never a stale log.
COMMANDS = {
    'player_pose_live': ['make', '-s', '-j8', 'INC=1', 'test-first-control-reference'],
    # The strict 0015BCF0 oracle fails by design (docs/LEVEL_ORACLE_AUDIT.md
    # "Requests for main: protected player wrapper"); its make target runs the
    # expected-failure check that pins the exact failure signature.
    'level14_player_frame': [sys.executable, os.path.join(ROOT, 'tools', 'test_level14_player_frame_expected_failure.py')],
}
# Oracles that need a local capture the repo cannot ship (disc-derived): reported SKIP.
SKIP = {
    'camera': 'needs --ee/--track over a local opening_ee.bin capture (docs/CAMERA_LIVE.md)',
}
# Oracles whose quick run legitimately exceeds the default cap (name -> seconds).
# Measured under load 25+ (quick mode, solo CPU time is about a third of these).
TIMEOUTS = {'area11_sfx': 900, 'area21_groups': 300, 'far_light_readback': 900, 'area01_revisit_runtime': 240, 'area00_delivery': 240}
SEVERITY = {'PASS': 0, 'SKIP': 0, 'FAIL': 1, 'TIMEOUT': 2, 'BUILD': 3}

BUILD_RE = re.compile(r"Undefined symbols|ld: |linker command failed|error: |"
                      r"CalledProcessError: Command '\[?'(cc|clang|gcc|make)'")


def classify(rc, out):
    if rc == 0:
        return 'PASS'
    if BUILD_RE.search(out) and 'Traceback' in out and "died with" not in out:
        return 'BUILD'
    return 'FAIL'


def run_one(name, cmd, cap, logdir):
    env = dict(os.environ)
    env.pop('EM_TEST_FULL', None)
    env.setdefault('EM_TEST_JOBS', '2')
    env.setdefault('EM_HEADLESS', '1')
    t0 = time.time()
    try:
        p = subprocess.run(cmd, cwd=ROOT, env=env, stdin=subprocess.DEVNULL,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                           text=True, errors='replace', timeout=cap)
        status, out, rc = classify(p.returncode, p.stdout), p.stdout, p.returncode
    except subprocess.TimeoutExpired as e:
        out = e.stdout.decode('utf8', 'replace') if isinstance(e.stdout, bytes) else (e.stdout or '')
        status, rc = 'TIMEOUT', None
    secs = round(time.time() - t0, 1)
    with open(os.path.join(logdir, name + '.log'), 'w') as f:
        f.write(out)
    tail = [l for l in out.strip().splitlines() if l.strip()][-1:] or ['']
    return {'name': name, 'status': status, 'rc': rc, 'secs': secs, 'last': tail[0][:160]}


def make_targets():
    text = open(os.path.join(ROOT, 'Makefile')).read()
    return sorted(set(re.findall(r'^(test-[A-Za-z0-9_-]+):', text, re.M)))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('-j', '--jobs', type=int, default=4)
    ap.add_argument('-t', '--timeout', type=int, default=120, help='per-test cap in seconds')
    ap.add_argument('-k', '--filter', action='append', default=[], help='run names containing this')
    ap.add_argument('--make', action='store_true', help='also run `make test-*` targets (opt-in; slower)')
    ap.add_argument('--baseline', default=BASELINE)
    ap.add_argument('--update-baseline', action='store_true')
    ap.add_argument('--no-retry', action='store_true')
    a = ap.parse_args()

    jobs, skipped = [], []
    for path in sorted(glob.glob(os.path.join(ROOT, 'tools', 'test_*_reference.py'))):
        name = os.path.basename(path)[5:-len('_reference.py')]
        if name in SKIP:
            skipped.append(name)
            continue
        for suffix, extra in ARGS.get(name, [('', [])]):
            cmd = COMMANDS.get(name) or [sys.executable, path] + extra
            jobs.append((name + suffix, cmd, TIMEOUTS.get(name, a.timeout)))
    for name, script in sorted(FIXTURES.items()):
        if name not in SKIP:
            jobs.append((name, [sys.executable, os.path.join(ROOT, 'tools', script)], TIMEOUTS.get(name, a.timeout)))
    if a.make:
        for t in make_targets():
            jobs.append(('make:' + t[5:], ['make', t], TIMEOUTS.get('make:' + t[5:], a.timeout)))
    if a.filter:
        jobs = [j for j in jobs if any(k in j[0] for k in a.filter)]

    stamp = time.strftime('%Y%m%d-%H%M%S')
    outdir = os.path.join(ROOT, 'build', 'test_sweep', stamp)
    logdir = os.path.join(outdir, 'logs')
    os.makedirs(logdir)
    baseline = json.load(open(a.baseline)) if os.path.exists(a.baseline) else {}

    results = {}
    with cf.ThreadPoolExecutor(max_workers=a.jobs) as ex:
        futs = [ex.submit(run_one, n, c, t, logdir) for n, c, t in jobs]
        for i, f in enumerate(cf.as_completed(futs), 1):
            r = f.result()
            results[r['name']] = r
            if r['status'] != 'PASS':
                print(f"[{i}/{len(jobs)}] {r['status']:7} {r['name']}", flush=True)

    if not a.no_retry:
        for n, c, t in jobs:
            r = results[n]
            b = baseline.get(n)
            if r['status'] != 'PASS' and (not b or SEVERITY[r['status']] > SEVERITY[b['status']]):
                r2 = run_one(n, c, t, logdir)
                if r2['status'] != r['status']:
                    print(f"retry {n}: {r['status']} -> {r2['status']}")
                r2['retried'] = True
                results[n] = r2

    new, fixed = [], []
    for n, r in results.items():
        b = baseline.get(n)
        if r['status'] == 'PASS':
            if b:
                fixed.append(n)
        elif not b or SEVERITY[r['status']] > SEVERITY[b['status']]:
            new.append(n)
        r['baseline'] = b['status'] if b else None
    counts = {s: sum(1 for r in results.values() if r['status'] == s) for s in SEVERITY}
    json.dump({'stamp': stamp, 'counts': counts, 'new_failures': new, 'fixed': fixed,
               'results': results}, open(os.path.join(outdir, 'results.json'), 'w'), indent=1)

    print(f"\n{len(results)} tests (skipped, need local data: {', '.join(skipped) or 'none'}): " + ', '.join(f'{k} {v}' for k, v in counts.items()))
    bad = [r for r in results.values() if r['status'] != 'PASS']
    for r in sorted(bad, key=lambda r: (r['name'] not in new, r['name'])):
        tag = 'NEW   ' if r['name'] in new else 'known '
        print(f"  {tag}{r['status']:7} {r['name']:44} {r['secs']:5}s  {r['last'][:70]}")
    for n in fixed:
        print(f"  FIXED  {n} passes now; remove it from the baseline")
    print(f"results: {os.path.relpath(outdir, ROOT)}/results.json")
    # Installed assets made by an older exporter (tools/asset_freshness.py): a warning, never a failure.
    fresh = subprocess.run([sys.executable, os.path.join(ROOT, 'tools', 'asset_freshness.py')], cwd=ROOT,
                           capture_output=True, text=True)
    lines = (fresh.stdout or fresh.stderr or 'asset_freshness: no output').strip().splitlines()
    print(('WARNING ' if ' 0 stale' not in lines[0] else '') + '\n'.join(lines[:12]))

    if a.update_baseline:
        nb = {}
        for n, r in sorted(results.items()):
            if r['status'] != 'PASS':
                old = baseline.get(n, {})
                nb[n] = {'status': r['status'], 'reason': old.get('reason', 'TODO'),
                         'owner': old.get('owner', 'TODO')}
        json.dump(nb, open(a.baseline, 'w'), indent=1, sort_keys=True)
        open(a.baseline, 'a').write('\n')
        print(f'baseline rewritten: {len(nb)} entries')
        return 0
    if new:
        print(f"NEW FAILURES: {', '.join(new)}")
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
