#!/usr/bin/env python3
"""Run a command and print a compact summary instead of its full output.

The port's checks (level smokes, oracle reference tests, fixtures) print up to
~1 M characters; an agent reading that pays for every character. This wrapper
changes only what is PRINTED, never what is checked: the command runs as it
is, its exit code is this wrapper's exit code, and its whole output (stdout and
stderr, in order) goes to a log file.

Printed on success: compiler warning lines (at most 10), one line per check line holding PASS/FAIL (cut to WIDTH
characters, a repeated check name once), then one footer line with the verdict,
the exit code, the log's size and its path. A run with no PASS/FAIL line shows
its last two lines. Printed on failure: the same, then the first failing detail
(the first line naming FAIL, a Traceback, an Error or an assertion, with its next
lines, uncut up to 600 characters a line) and the last 12 lines of the output.

  python3 tools/quiet.py [--name NAME] [--width N] -- COMMAND ARGS...
  python3 tools/quiet.py -c 'SHELL COMMAND LINE'          (as make's SHELL: see the Makefile)

Full output instead: EM_VERBOSE=1 (or V=1) in the environment, or --verbose.
The log is build/quiet/<name>.log (NAME defaults to the command's first words).
"""
import os
import re
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LOG_DIR = ROOT / 'build' / 'quiet'
WIDTH = 130
VERDICT = re.compile(r'\b(PASS|FAIL|FAILED)\b')
FAILING = re.compile(r'\bFAIL(ED)?\b|Traceback|Error\b|error:|AssertionError|assertion|\bfault\b|Abort|Segmentation',
                     re.I)


def cut(line, width):
    line = line.rstrip()
    return line if len(line) <= width else line[:width - 3] + '...'


def check_name(line):
    head = line.split(' PASS', 1)[0].split(' FAIL', 1)[0]
    head = re.sub(r'^level smoke: ', '', head)
    return head.rstrip(':').strip()


def summarize(lines, rc, width):
    shown, names = [], set()
    for line in lines:
        if not VERDICT.search(line):
            continue
        is_fail = bool(re.search(r'\bFAIL(ED)?\b', line))
        name = check_name(line)
        if not is_fail and name in names:
            continue
        names.add(name)
        shown.append(cut(line, width))
    if not shown:
        tail = [l for l in lines if l.strip()][-2:]
        shown = [cut(l, width) for l in tail]
    out = list(shown)
    warnings = [l for l in lines if 'warning:' in l]
    out.extend(cut(l, 200) for l in warnings[:10])
    if len(warnings) > 10:
        out.append(f'... {len(warnings) - 10} more compiler warnings in the log')
    if rc != 0:
        first = next((i for i, l in enumerate(lines) if FAILING.search(l)), None)
        if first is not None and first < len(lines) - 12:
            out.append('-- first failing detail:')
            out.extend(cut(l, 600) for l in lines[first:first + 4])
        out.append('-- last lines:')
        out.extend(cut(l, 300) for l in lines[-12:])
    return out


def log_path(name, argv):
    if not name:
        skip = ('python3', 'python', 'env', 'make', 'sh', 'time', 'mkdir', 'rm', 'cd', '&&', ';')
        words = [Path(a).name for a in argv if not a.startswith('-') and '=' not in a and a not in skip][:2]
        name = '_'.join(words) or 'run'
    name = re.sub(r'[^A-Za-z0-9_.-]+', '_', name)[:60]
    return LOG_DIR / f'{name}.log'


def main(argv):
    verbose = os.environ.get('EM_VERBOSE') == '1' or os.environ.get('V') == '1'
    name, width = None, WIDTH
    while argv and argv[0] in ('--name', '--width', '--verbose'):
        if argv[0] == '--verbose':
            verbose, argv = True, argv[1:]
        elif argv[0] == '--name':
            name, argv = argv[1], argv[2:]
        else:
            width, argv = int(argv[1]), argv[2:]
    if argv and argv[0] == '--':
        argv = argv[1:]
    shell_mode = bool(argv) and argv[0] == '-c'
    cmd = ['/bin/sh', '-c', argv[1]] if shell_mode else argv
    if not cmd:
        print(__doc__)
        return 2
    if verbose:
        os.execvp(cmd[0], cmd)
    LOG_DIR.mkdir(parents=True, exist_ok=True)
    log = log_path(name, argv[1].split() if shell_mode else argv)
    started = time.time()
    proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    text = proc.stdout.decode('utf-8', 'replace')
    lines = text.splitlines()
    rc = proc.returncode
    if not lines and rc == 0:
        return 0  # a silent step (mkdir, rm, a compile that printed nothing): nothing to report
    log.write_text(text)
    for line in summarize(lines, rc, width):
        print(line)
    print(f'[quiet] {"PASS" if rc == 0 else "FAIL"} rc={rc} {time.time() - started:.0f}s: '
          f'{len(lines)} lines {len(text)} bytes in {log.relative_to(ROOT)} (EM_VERBOSE=1 for all)')
    sys.stdout.flush()
    return rc


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
