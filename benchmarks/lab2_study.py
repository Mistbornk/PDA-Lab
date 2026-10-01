#!/usr/bin/env python3
"""Paired HPWL variants across seeds, datasets and deterministic work budgets."""
import argparse
from datetime import datetime, timezone
import json
from pathlib import Path
import platform
import statistics
import tempfile
from types import SimpleNamespace
from run import ROOT, build_metadata, course, run


def summarize(rows):
    summary = []
    for dataset, budget in sorted({(r['dataset'], r['iterations']) for r in rows}):
        selected = [r for r in rows if (r['dataset'], r['iterations']) == (dataset, budget)]
        group = {'dataset': dataset, 'iterations': budget}
        for mode in ['strings', 'ids']:
            runs = [r for r in selected if r['hpwl_mode'] == mode]
            legal = [r['objective'] for r in runs if r['valid']]
            group[mode] = {
                'attempts': len(runs), 'valid': len(legal),
                'budget_exhausted': sum(r['outcome'] == 'budget_exhausted' for r in runs),
                'objective_min_median_max_among_valid': [min(legal), statistics.median(legal), max(legal)] if legal else None,
                'median_wall_seconds_all_attempts': statistics.median(r['time_wall_seconds'] for r in runs if 'time_wall_seconds' in r),
                'median_peak_rss_kib': statistics.median(r['peak_rss_kib'] for r in runs if 'peak_rss_kib' in r),
            }
        ratios = []
        for seed in sorted({r['seed'] for r in selected}):
            pair = {r['hpwl_mode']: r for r in selected if r['seed'] == seed}
            if set(pair) == {'strings', 'ids'} and pair['ids'].get('time_wall_seconds', 0) > 0:
                ratios.append(pair['strings']['time_wall_seconds']/pair['ids']['time_wall_seconds'])
        group['median_paired_speed_ratio'] = statistics.median(ratios) if ratios else None
        summary.append(group)
    return summary


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--bin-root', type=Path, default=ROOT/'build')
    p.add_argument('--seeds', type=int, nargs='+', default=[1, 7, 19])
    p.add_argument('--iterations', type=int, nargs='+', default=[100000, 300000])
    p.add_argument('--case', action='append', default=[])
    p.add_argument('--alpha', type=float, default=.5)
    p.add_argument('--timeout', type=float, default=300)
    p.add_argument('--output', type=Path, default=ROOT/'benchmarks/results/lab2-study.json')
    args = p.parse_args()
    if not 0 <= args.alpha <= 1 or args.timeout <= 0 or any(n < 0 or n > 2**32-1 for n in args.seeds) or any(n <= 0 for n in args.iterations):
        p.error('Invalid alpha, timeout, seed or work budget')
    if len(set(args.seeds)) != len(args.seeds) or len(set(args.iterations)) != len(args.iterations):
        p.error('Duplicate seeds/budgets would bias the paired summary')
    datasets = [(n, inputs) for n, inputs in course.cases('Lab02') if not args.case or n in args.case]
    if not datasets or any(name not in [n for n, _ in datasets] for name in args.case):
        p.error('Missing dataset')
    parent = ROOT/'benchmarks/work'; parent.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix='lab2-study-', dir=parent))
    args.bin_root = args.bin_root.resolve()
    report = {'timestamp': datetime.now(timezone.utc).isoformat(), 'build': build_metadata(args.bin_root),
              'platform': platform.platform(), 'jobs': 1, 'solver_threads': 1,
              'cpu': next((line.split(':', 1)[1].strip() for line in Path('/proc/cpuinfo').read_text().splitlines() if line.startswith('model name')), None),
              'seeds': args.seeds, 'budgets': args.iterations, 'alpha': args.alpha, 'work_dir': str(work),
              'method': 'One serial run per seed/budget/mode. Mode order alternates by pair. All failures retained; valid-only objective summaries are labeled. No cross-dataset objective averaging.',
              'results': [], 'pairs': [], 'complete': False}
    unexpected = False
    for name, inputs in datasets:
        for budget in args.iterations:
            for seed in args.seeds:
                modes = ['strings', 'ids'] if len(report['pairs']) % 2 == 0 else ['ids', 'strings']
                pair = []
                for mode in modes:
                    options = ['--seed', str(seed), '--iterations', str(budget), '--hpwl', mode, '--stats']
                    settings = SimpleNamespace(bin_root=args.bin_root, alpha=args.alpha, solver_arg=options, timeout=args.timeout)
                    row = run(('Lab02', name, inputs, len(report['results'])), settings, work)
                    row.update(seed=seed, iterations=budget, hpwl_mode=mode)
                    exhausted = row['returncode'] == 1 and 'No legal floorplan found within budget' in row['solver_log']
                    row['outcome'] = 'valid' if row['valid'] else 'budget_exhausted' if exhausted else 'unexpected_failure'
                    unexpected |= row['outcome'] == 'unexpected_failure'
                    report['results'].append(row); pair.append(row)
                same = (pair[0]['outcome'] == pair[1]['outcome'] and
                        (pair[0]['outcome'] == 'budget_exhausted' or
                         (pair[0]['valid'] and pair[0]['solution_sha256'] == pair[1]['solution_sha256'])))
                report['pairs'].append(dict(dataset=name, iterations=budget, seed=seed, equivalent=same, outcome=pair[0]['outcome']))
                unexpected |= not same
                report['summary'] = summarize(report['results'])
                args.output.parent.mkdir(parents=True, exist_ok=True)
                args.output.write_text(json.dumps(report, indent=2)+'\n')
                print(f'PAIR {name} seed={seed} budget={budget}: {pair[0]["outcome"]}, equivalent={same}', flush=True)
    report['complete'] = True
    report['all_pairs_equivalent'] = all(r['equivalent'] for r in report['pairs'])
    args.output.write_text(json.dumps(report, indent=2)+'\n')
    raise SystemExit(1 if unexpected else 0)


if __name__ == '__main__': main()
