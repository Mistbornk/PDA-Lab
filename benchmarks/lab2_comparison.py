#!/usr/bin/env python3
"""Paired engineering ablations and equal-CPU-budget floorplanning experiments."""
import argparse
from datetime import datetime, timezone
import json
from pathlib import Path
import platform
import statistics
import tempfile
from types import SimpleNamespace
from run import ROOT, run, course, build_metadata


def paired_quality(rows):
    """Compare costs only for paired feasible seeds, retaining feasibility changes."""
    summaries = []
    reference = 'baseline' if any(r['mode']=='baseline' for r in rows) else 'dense-snapshot'
    for dataset,budget in sorted({(r['dataset'],r['budget']) for r in rows}):
        group = [r for r in rows if r['dataset']==dataset and r['budget']==budget]
        pairs = []
        for seed in sorted({r['seed'] for r in group}):
            before = next(r for r in group if r['seed']==seed and r['mode']==reference)
            after = next(r for r in group if r['seed']==seed and r['mode']=='skyline-journal')
            pairs.append(dict(seed=seed, before_legal=before['valid'], after_legal=after['valid'],
                              before_objective=before.get('objective'), after_objective=after.get('objective')))
        both = [p for p in pairs if p['before_legal'] and p['after_legal']]
        summaries.append(dict(dataset=dataset, budget_cpu_seconds=budget, pairs=pairs,
                              baseline_legal=sum(p['before_legal'] for p in pairs),
                              optimized_legal=sum(p['after_legal'] for p in pairs), both_legal=len(both),
                              optimized_wins=sum(p['after_objective'] < p['before_objective'] for p in both),
                              ties=sum(p['after_objective'] == p['before_objective'] for p in both),
                              optimized_losses=sum(p['after_objective'] > p['before_objective'] for p in both),
                              paired_median_cost_change_percent=statistics.median(
                                  100*(p['after_objective']-p['before_objective'])/p['before_objective']
                                  for p in both) if both else None))
    return summaries


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--bin-root', type=Path, default=ROOT/'build')
    p.add_argument('--before', type=Path)
    p.add_argument('--suite', choices=['fixed', 'time'], default='fixed')
    p.add_argument('--seeds', type=int, nargs='+')
    p.add_argument('--budgets', type=float, nargs='+')
    p.add_argument('--case', action='append', default=[])
    p.add_argument('--output', type=Path, required=True)
    args = p.parse_args()
    seeds = args.seeds or ([1, 7, 19] if args.suite == 'fixed' else [1, 7, 19, 31, 43, 59, 71, 83, 97, 109])
    budgets = args.budgets or ([300000] if args.suite == 'fixed' else [3, 8])
    if any(s < 0 or s > 4294967295 for s in seeds) or any(b <= 0 for b in budgets): p.error('Invalid seed or budget')
    if args.suite == 'fixed' and any(b != int(b) for b in budgets): p.error('Iterations must be integers')
    cases = [(n, paths) for n, paths in course.cases('Lab02') if not args.case or n in args.case]
    if not cases or any(n not in [c[0] for c in cases] for n in args.case): p.error('Missing selected case')
    variants = [('dense-snapshot', args.bin_root, ['--packing', 'dense', '--rollback', 'snapshot']),
                ('skyline-snapshot', args.bin_root, ['--packing', 'skyline', '--rollback', 'snapshot']),
                ('skyline-journal', args.bin_root, ['--packing', 'skyline', '--rollback', 'journal'])]
    if args.before: variants.insert(0, ('baseline', args.before, []))
    if args.suite == 'time': variants = [variants[0], variants[-1]]
    (ROOT/'benchmarks/work').mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix='lab2-'+args.suite+'-', dir=ROOT/'benchmarks/work'))
    rows = []
    for name, inputs in cases:
        for budget in budgets:
            for pair, seed in enumerate(seeds):
                ordered = variants if pair % 2 == 0 else variants[::-1]
                for mode, binaries, extra in ordered:
                    flags = ['--seed', str(seed), '--iterations' if args.suite == 'fixed' else '--seconds',
                             str(int(budget)) if args.suite == 'fixed' else str(budget), '--stats', *extra]
                    settings = SimpleNamespace(bin_root=binaries.resolve(), solver_arg=flags, alpha=.5, timeout=max(300, budget*4 if args.suite == 'time' else 300))
                    row = run(('Lab02', name, inputs, len(rows)), settings, work)
                    row.update(seed=seed, budget=budget, mode=mode)
                    row['outcome'] = 'valid' if row['valid'] else ('budget-exhausted' if row['status']=='failed' and 'No legal floorplan found within budget' in row['solver_log'] else 'unexpected-failure')
                    rows.append(row)
                    # Save complete raw evidence incrementally, including failed seeds.
                    report = dict(timestamp=datetime.now(timezone.utc).isoformat(), platform=platform.platform(),
                                  build={v[0]:build_metadata(v[1].resolve()) for v in variants}, jobs=1, solver_threads=1,
                                  suite=args.suite, seeds=seeds, budgets=budgets, alpha=.5, work_dir=str(work),
                                  complete=False, expected_runs=len(cases)*len(budgets)*len(seeds)*len(variants),
                                  method='Serial paired runs; variant order alternates by seed. Timed runs budget CPU seconds per solve, not wall time. Include every failure; compare objectives only within a dataset.',
                                  results=rows)
                    args.output.parent.mkdir(parents=True, exist_ok=True)
                    args.output.write_text(json.dumps(report, indent=2)+'\n')
    checks = []
    if args.suite == 'fixed':
        for name, _ in cases:
            for budget in budgets:
                for seed in seeds:
                    group = [r for r in rows if r['dataset']==name and r['seed']==seed and r['budget']==budget]
                    equal = len({(r['outcome'], r.get('solution_sha256')) for r in group}) == 1
                    checks.append(dict(dataset=name, budget=budget, seed=seed, equal=equal))
        report['fixed_work_equivalence'] = checks
    report['summary'] = []
    for name, _ in cases:
        for budget in budgets:
            for mode, _, _ in variants:
                group = [r for r in rows if r['dataset']==name and r['budget']==budget and r['mode']==mode]
                legal = [r for r in group if r['valid']]
                report['summary'].append(dict(dataset=name, budget=budget, mode=mode, runs=len(group), valid=len(legal),
                                              median_wall_seconds=statistics.median(r['wall_seconds'] for r in group),
                                              median_peak_rss_kib=statistics.median(r['peak_rss_kib'] for r in group),
                                              valid_only_median_objective=statistics.median(r['objective'] for r in legal) if legal else None))
    if args.suite == 'time': report['paired_quality'] = paired_quality(rows)
    report['complete'] = True
    args.output.write_text(json.dumps(report, indent=2)+'\n')
    raise SystemExit(0 if all(r['outcome']!='unexpected-failure' for r in rows) and all(c['equal'] for c in checks) else 1)


if __name__ == '__main__': main()
