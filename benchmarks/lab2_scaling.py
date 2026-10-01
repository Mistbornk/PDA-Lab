#!/usr/bin/env python3
"""Same eight annealing trajectories at 1/2/4/8 internal worker threads."""
import argparse
import json
from pathlib import Path
import platform
import statistics
import tempfile
from types import SimpleNamespace
from run import ROOT, run, course, build_metadata


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--bin-root', type=Path, default=ROOT/'build')
    p.add_argument('--case', default='ami49')
    p.add_argument('--iterations', type=int, default=300000)
    p.add_argument('--repeat', type=int, default=3)
    p.add_argument('--output', type=Path, default=ROOT/'benchmarks/results/lab2-scaling.json')
    args = p.parse_args()
    inputs = next(paths for name,paths in course.cases('Lab02') if name == args.case)
    parent = ROOT/'benchmarks/work'; parent.mkdir(parents=True,exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix='multistart-',dir=parent)); rows = []
    for repeat in range(args.repeat):
        threads = [1,2,4,8] if repeat%2 == 0 else [8,4,2,1]
        for count in threads:
            flags = ['--seed','1','--iterations',str(args.iterations),'--restarts','8','--threads',str(count),'--stats']
            settings = SimpleNamespace(bin_root=args.bin_root.resolve(),alpha=.5,solver_arg=flags,timeout=600)
            row = run(('Lab02',args.case,inputs,len(rows)),settings,work)
            row.update(solver_threads=count, repeat=repeat)
            rows.append(row)
    assert all(r['valid'] for r in rows)
    assert len({r['solution_sha256'] for r in rows}) == 1
    attempts = [[(a['seed'],a['legal'],a['iterations'],a.get('objective')) for a in r['diagnostics']['attempts']] for r in rows]
    assert all(a == attempts[0] for a in attempts)
    summary = []
    serial = statistics.median(r['wall_seconds'] for r in rows if r['solver_threads']==1)
    for count in [1,2,4,8]:
        group = [r for r in rows if r['solver_threads']==count]
        seconds = statistics.median(r['wall_seconds'] for r in group)
        summary.append(dict(threads=count, median_wall_seconds=seconds, speedup=serial/seconds,
                            median_peak_rss_kib=statistics.median(r['peak_rss_kib'] for r in group)))
    report = dict(platform=platform.platform(), build=build_metadata(args.bin_root), jobs=1,
                  restarts=8, seeds=list(range(1,9)), iterations_per_restart=args.iterations,
                  method='Fixed total work; serial benchmark runs; alternating worker-count order. Every restart outcome and selected placement identical across thread counts.',
                  results=rows, summary=summary)
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(summary)


if __name__ == '__main__': main()
