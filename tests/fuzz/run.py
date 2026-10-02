#!/usr/bin/env python3
"""Run bounded parser fuzz campaigns, keeping mutable corpora outside tracked seeds."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bin-root', type=Path, default=ROOT / 'build-fuzz')
    parser.add_argument('--runs', type=int, default=20000)
    parser.add_argument('--seconds', type=int, default=30)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.runs < 1 or args.seconds < 1:
        parser.error('Budgets must be positive')
    parent = ROOT / 'benchmarks/work'
    parent.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix='fuzz-', dir=parent))
    results = []
    for lab in range(1, 5):
        corpus = work / f'lab{lab}'
        shutil.copytree(ROOT / f'tests/fuzz/corpus/lab{lab}', corpus)
        executable = args.bin_root.resolve() / f'fuzz_lab{lab}'
        command = [str(executable), str(corpus), f'-runs={args.runs}',
                   f'-max_total_time={args.seconds}', '-seed=1337', '-max_len=16384',
                   '-timeout=3', '-rss_limit_mb=512', f'-artifact_prefix={work}/']
        began = time.perf_counter()
        log = work / f'lab{lab}.log'
        with log.open('w') as stream:
            try:
                run = subprocess.run(command, stdout=stream, stderr=subprocess.STDOUT,
                                     timeout=args.seconds + 20)
                status = 'passed' if run.returncode == 0 else 'failed'
            except subprocess.TimeoutExpired:
                status = 'timeout'
        results.append(dict(lab=lab, status=status, command=command,
                            executable_sha256=hashlib.sha256(executable.read_bytes()).hexdigest(),
                            wall_seconds=time.perf_counter()-began,
                            log_tail=log.read_text()[-4000:], log=str(log)))
        print(f'Lab{lab:02}: {status}', flush=True)
    report = dict(schema_version=1, complete=True, results=results,
                  source_revision=subprocess.check_output(['git', 'rev-parse', 'HEAD'],
                                                          cwd=ROOT, text=True).strip(),
                  dirty=bool(subprocess.check_output(['git', 'status', '--porcelain'], cwd=ROOT)))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2)+'\n')
    raise SystemExit(0 if all(r['status'] == 'passed' for r in results) else 1)


if __name__ == '__main__':
    main()
