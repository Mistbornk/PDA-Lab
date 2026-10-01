#!/usr/bin/env python3
"""Profile-guided Lab01 comparison with independently known spaced-grid results."""
import argparse
import hashlib
import json
from pathlib import Path
import platform
import random
import statistics
import subprocess
import tempfile
import time
from run import ROOT, measured


def grid(side, directory):
    rng = random.Random(1701 + side)
    blocks = [(4*x+1, 4*y+1, 2, 2) for x in range(side) for y in range(side)]
    rng.shuffle(blocks)
    path = directory / f'grid-{side*side}.txt'
    path.write_text(f'{side*4} {side*4}\n' + ''.join(
        f'{i+1} {x} {y} {w} {h}\nP {x} {y}\n'
        for i, (x, y, w, h) in enumerate(blocks)))
    # Every isolated solid has four empty neighbors. Horizontal empty strips span
    # the outline; each occupied row has side+1 empty gaps.
    expected = [str(2*side*side+2*side+1)]
    expected += [f'{i+1} 0 4' for i in range(side*side)]
    expected += [f'{x} {y}' for x, y, _, _ in blocks]
    return path, '\n'.join(expected).split()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--before', type=Path, required=True)
    p.add_argument('--after', type=Path, default=ROOT/'build/Lab01/Lab1')
    p.add_argument('--repeat', type=int, default=3)
    p.add_argument('--output', type=Path, default=ROOT/'benchmarks/results/lab1-stitches.json')
    args = p.parse_args()
    parent = ROOT/'benchmarks/work'; parent.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix='stitches-', dir=parent))
    datasets = [('case7', ROOT/'Lab01/tests/data/case7.txt',
                 (ROOT/'Lab01/tests/expected/output7.txt').read_text().split())]
    datasets += [(f'grid-{n*n}', *grid(n, work)) for n in [20, 40, 60]]
    results = []
    for name, path, expected in datasets:
        for repeat in range(args.repeat):
            for mode, exe, options in [('before', args.before, []),
                                       ('scan', args.after, ['--stitches', 'scan', '--geometry', 'scan', '--stats']),
                                       ('indexed', args.after, ['--stitches', 'indexed', '--geometry', 'scan', '--stats'])]:
                case = work/f'{name}-{repeat}-{mode}'; case.mkdir()
                output = case/'result.txt'
                command = [exe.resolve(), path, output, *options]
                row = measured(command, case, 180)
                row.update(dataset=name, repeat=repeat, mode=mode, command=list(map(str, command)),
                           solver_threads=1, executable_sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),
                           input_sha256=hashlib.sha256(path.read_bytes()).hexdigest())
                row['valid'] = row['status']=='completed' and output.read_text().split()==expected
                if output.exists(): row['output_sha256'] = hashlib.sha256(output.read_bytes()).hexdigest()
                row['diagnostic'] = (case/'solver.log').read_text()
                results.append(row)
                print(name, mode, row.get('time_wall_seconds'), row['valid'], flush=True)
    report = dict(platform=platform.platform(), compiler=subprocess.check_output(['g++', '--version'], text=True).splitlines()[0],
                  flags='-std=c++17 -O3 -DNDEBUG', build_type='Release', jobs=1, generator_seed='1701 + side',
                  work_dir=str(work), results=results)
    args.output.write_text(json.dumps(report, indent=2)+'\n')
    raise SystemExit(0 if all(r['valid'] for r in results) else 1)


if __name__ == '__main__': main()
