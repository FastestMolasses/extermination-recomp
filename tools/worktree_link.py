#!/usr/bin/env python3
"""Link a port worktree to the main checkout's ignored, locally generated
inputs, the way the lanes have been doing it by hand.

    python3 tools/worktree_link.py            # from inside a worktree
    python3 tools/worktree_link.py --check    # report only, change nothing

What it links (all ignored by .gitignore, all produced on this machine from
the user's own disc; nothing is copied):
  * data/                      one directory symlink
  * assets/                    real directories, one symlink per file, so an
                               exporter run in the worktree refuses to write
                               through a symlink instead of changing main's
                               files (export_status_pages and
                               level_asset_audit check for that)
  * build/start_saves, build/beat_rands   directory symlinks (recordings,
                               never deleted; read-only use)

Existing entries are left alone: a real file or a different symlink at a
path is reported and kept. Run it again after main gains new assets.
"""
import argparse
import os
import subprocess
import sys
from pathlib import Path

DIR_LINKS = ('data', 'build/start_saves', 'build/beat_rands')


def main_checkout(here):
    """The worktree list's first entry is the main checkout."""
    out = subprocess.run(['git', '-C', str(here), 'worktree', 'list', '--porcelain'],
                         check=True, capture_output=True, text=True).stdout
    first = next(line for line in out.splitlines() if line.startswith('worktree '))
    return Path(first[len('worktree '):]).resolve()


def link(dst, src, check, report):
    if os.path.lexists(dst):
        if dst.is_symlink() and Path(os.readlink(dst)) == src:
            return 0
        report.append('kept existing %s' % dst)
        return 0
    if not check:
        dst.parent.mkdir(parents=True, exist_ok=True)
        os.symlink(src, dst)
    return 1


def run(check=False):
    here = Path(subprocess.run(['git', 'rev-parse', '--show-toplevel'], check=True,
                               capture_output=True, text=True).stdout.strip()).resolve()
    main = main_checkout(here)
    if here == main:
        sys.exit('worktree_link: this is the main checkout; run it inside a worktree')
    report, made = [], 0
    for rel in DIR_LINKS:
        src = main / rel
        if src.exists():
            made += link(here / rel, src, check, report)
    assets = main / 'assets'
    for root, dirs, files in os.walk(assets):
        dirs.sort()
        rel = Path(root).relative_to(assets)
        for name in sorted(files):
            made += link(here / 'assets' / rel / name, Path(root) / name, check, report)
    for line in report[:20]:
        print(line)
    if len(report) > 20:
        print('... %d more kept' % (len(report) - 20))
    print('worktree_link: %s %d links from %s (%d kept as they were)'
          % ('would make' if check else 'made', made, main, len(report)))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    parser.add_argument('--check', action='store_true', help='report only')
    run(parser.parse_args().check)
