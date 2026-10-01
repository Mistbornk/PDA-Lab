#!/usr/bin/env python3
"""Measure spatial candidate selection against full scans, using known grid answers."""
import argparse
import hashlib
import json
from pathlib import Path
import platform
import tempfile
from lab1_stitches import grid
from run import ROOT, measured, build_metadata


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--bin-root', type=Path, default=ROOT/'build')
    p.add_argument('--repeat', type=int, default=3)
    p.add_argument('--output', type=Path, default=ROOT/'benchmarks/results/lab1-geometry.json')
    args = p.parse_args()
    work = Path(tempfile.mkdtemp(prefix='geometry-', dir=ROOT/'benchmarks/work'))
    exe = args.bin_root.resolve()/'Lab01/Lab1'
    rows = []
    for side in [20, 60, 100]:
        path, expected = grid(side, work)
        for repeat in range(args.repeat):
            # Alternate execution order to avoid always warming the same variant.
            modes = ['scan', 'spatial'] if repeat % 2 == 0 else ['spatial', 'scan']
            for mode in modes:
                case = work/f'{side}-{repeat}-{mode}'; case.mkdir()
                output = case/'result.txt'
                command = [exe, path, output, '--geometry', mode, '--stats']
                row = measured(command, case, 180)
                row.update(dataset=f'grid-{side*side}', repeat=repeat, mode=mode,
                           command=list(map(str, command)), solver_threads=1,
                           executable_sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),
                           input_sha256=hashlib.sha256(path.read_bytes()).hexdigest())
                row['valid'] = row['status'] == 'completed' and output.read_text().split() == expected
                if output.exists(): row['output_sha256'] = hashlib.sha256(output.read_bytes()).hexdigest()
                row['diagnostic'] = (case/'solver.log').read_text()
                rows.append(row)
                print(row['dataset'], mode, row.get('time_wall_seconds'), row['valid'], flush=True)
    report = dict(platform=platform.platform(), build=build_metadata(args.bin_root), jobs=1,
                  generator_seed='1701 + side', work_dir=str(work),
                  method='Three serial repetitions per mode. Mode order alternates. Independent known grid output.', results=rows)
    args.output.write_text(json.dumps(report, indent=2)+'\n')
    raise SystemExit(0 if all(r['valid'] for r in rows) else 1)


if __name__ == '__main__': main()
