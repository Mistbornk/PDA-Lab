#!/usr/bin/env python3
"""Run course cases with the original validators; keep outputs and JSON evidence."""
import argparse
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import signal
import shutil
import subprocess
import threading
import time

ROOT = Path(__file__).resolve().parents[1]
EXECUTABLES = {'Lab01': 'Lab1', 'Lab02': 'Lab2', 'Lab03': 'Legalizer', 'Lab04': 'D2DGRter'}
LIMITS = {'Lab01': 60, 'Lab02': 300, 'Lab03': 1800, 'Lab04': 1200}
ANSI = re.compile(r'\x1b\[[0-9;]*m')


def cases(lab):
    data = ROOT / lab / 'tests/data'
    if lab == 'Lab01':
        return [(p.stem, [p]) for p in sorted(data.glob('case*.txt'))]
    if lab == 'Lab02':
        return [(p.stem, [p, p.with_suffix('.nets')]) for p in sorted(data.glob('*/*.block'))]
    if lab == 'Lab03':
        return [(p.stem, [p, p.with_suffix('.opt')]) for p in sorted(data.glob('*.lg'))]
    return [(p.stem, [p, p.with_suffix('.gcl'), p.with_suffix('.cst')])
            for p in sorted(data.glob('*/*.gmp'))]


def execute(command, log, timeout, cwd):
    start = time.perf_counter()
    with log.open('w') as stream:
        try:
            process = subprocess.Popen([str(x) for x in command], cwd=cwd, stdout=stream,
                                       stderr=subprocess.STDOUT, start_new_session=True)
            expired = threading.Event()

            def terminate():
                try:
                    os.killpg(process.pid, signal.SIGKILL)
                    expired.set()
                except ProcessLookupError:
                    pass

            timer = threading.Timer(timeout, terminate)
            timer.start()
            try:
                code = process.wait()
            finally:
                timer.cancel()
                timer.join()
            if expired.is_set():
                code, status = None, 'timeout'
            else:
                status = 'completed' if code == 0 else 'failed'
        except OSError as error:
            stream.write(str(error) + '\n')
            code, status = None, 'error'
    return {'command': [str(x) for x in command], 'returncode': code,
            'status': status, 'wall_seconds': time.perf_counter() - start, 'log': str(log)}


def verdict(lab, record, log):
    """Upstream tools sometimes return zero on errors: require positive evidence."""
    if record['status'] != 'completed':
        return False
    text = ANSI.sub('', log.read_text(errors='replace'))
    if re.search(r'\b(fail(?:ed)?|error)\b', text, re.IGNORECASE):
        return False
    if lab == 'Lab02':
        return all(re.search(label + r'\s*:\s*Pass\b', text, re.IGNORECASE)
                   for label in ['HPWL', 'AREA', 'COST', 'OVERLAP', 'OUT_OF_RANGE'])
    if lab == 'Lab03':
        return all(label in text for label in ['Move Times', 'Total Distance', '|', 'Total'])
    return '|    Total' in text and all('Pass ' + label + ' check' in text for label in
               ['routing direction', 'routing area', 'all-net-routed', 'connecticity', 'alignment'])


def run_case(task, args, output_root):
    lab, name, inputs = task
    work = output_root / lab / name
    work.mkdir(parents=True)
    exe = (args.bin_root / lab / EXECUTABLES[lab]).resolve()
    timeout = args.timeout or LIMITS[lab]
    output = work / (name + ('.txt' if lab == 'Lab01' else '.rpt' if lab == 'Lab02' else '.lg'))
    command = [exe] + ([str(args.alpha)] if lab == 'Lab02' else []) + inputs + [output]
    result = {'lab': lab, 'case': name, 'alpha': args.alpha if lab == 'Lab02' else None,
              'timeout_seconds': timeout, 'inputs': {str(p.relative_to(ROOT)):
              hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs},
              'executable_sha256': hashlib.sha256(exe.read_bytes()).hexdigest(),
              'output': str(output)}
    result['solver'] = execute(command, work / 'solver.log', timeout, work)
    result['passed'] = False
    if result['solver']['status'] == 'completed' and output.exists() and output.stat().st_size:
        if lab == 'Lab01':
            expected = ROOT / lab / 'tests/expected' / (name.replace('case', 'output') + '.txt')
            expected_rows = [line.split() for line in expected.read_text().splitlines() if line.strip()]
            actual_rows = [line.split() for line in output.read_text().splitlines() if line.strip()]
            result['passed'] = actual_rows == expected_rows
            result['validation'] = {'method': 'ordered rows/tokens against course expected output',
                                    'expected': str(expected)}
        else:
            if lab == 'Lab02':
                check = [ROOT / lab / 'tests/tools/verifier', str(args.alpha), *inputs, output]
            elif lab == 'Lab03':
                check = [ROOT / lab / 'tests/tools/upstream/Evaluator', *inputs, output]
            else:
                for path in inputs:
                    shutil.copy2(path, work / path.name)
                check = [ROOT / lab / 'tests/tools/upstream/Evaluator/Evaluator', work, name]
            log = work / 'validator.log'
            result['validation'] = execute(check, log, args.validator_timeout, work)
            result['passed'] = verdict(lab, result['validation'], log)
    (work / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
    print(f"{lab}/{name}: {'PASS' if result['passed'] else 'FAIL'} "
          f"(solver {result['solver']['wall_seconds']:.3f}s, {result['solver']['status']})", flush=True)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--lab', choices=['all', *EXECUTABLES], default='all')
    parser.add_argument('--case', help='Run only this exact case name')
    parser.add_argument('--bin-root', type=Path, default=ROOT,
                        help='Directory containing Lab01/Lab1, Lab02/Lab2, etc.')
    parser.add_argument('--alpha', type=float, default=0.5)
    parser.add_argument('--timeout', type=float, help='Override solver wall-time limit per case')
    parser.add_argument('--validator-timeout', type=float, default=300)
    parser.add_argument('--jobs', type=int, default=1, help='Independent case processes (default 1)')
    parser.add_argument('--output', type=Path, help='New, nonexistent result directory')
    parser.add_argument('--list', action='store_true')
    args = parser.parse_args()
    if not 0 <= args.alpha <= 1 or args.jobs < 1 or args.validator_timeout <= 0 or (
            args.timeout is not None and args.timeout <= 0):
        parser.error('Require 0 <= alpha <= 1, jobs >= 1, and positive timeouts')
    labs = list(EXECUTABLES) if args.lab == 'all' else [args.lab]
    tasks = [(lab, name, paths) for lab in labs for name, paths in cases(lab)
             if not args.case or name == args.case]
    if not tasks or any(not cases(lab) for lab in labs):
        parser.error('No cases found for a selected lab/case; run tests/fetch_resources.py first')
    if args.list:
        for lab, name, _ in tasks:
            print(f'{lab}/{name}')
        return
    output = (args.output or ROOT / 'tests/results' /
              datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ')).resolve()
    if output.exists():
        parser.error('--output must not exist (each run keeps isolated output and validation evidence)')
    for lab, _, paths in tasks:
        for path in [args.bin_root / lab / EXECUTABLES[lab], *paths]:
            if not path.is_file():
                parser.error(f'Missing {path}; build the lab / fetch resources first')
    output.mkdir(parents=True)
    print('Results:', output, flush=True)
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        results = list(pool.map(lambda task: run_case(task, args, output), tasks))
    summary = {'created_utc': datetime.now(timezone.utc).isoformat(), 'jobs': args.jobs,
               'note': 'Wall time includes scheduling contention; parallel runs are not performance baselines.',
               'passed': sum(r['passed'] for r in results), 'total': len(results), 'results': results}
    (output / 'summary.json').write_text(json.dumps(summary, indent=2) + '\n')
    print(f"Passed {summary['passed']}/{summary['total']}; {output / 'summary.json'}")
    raise SystemExit(0 if summary['passed'] == summary['total'] else 1)


if __name__ == '__main__':
    main()
