#!/usr/bin/env python3
"""Equal wall-budget routing study with original-objective verifier checks."""
import argparse
import json
from pathlib import Path
import statistics
import tempfile
from types import SimpleNamespace
from generated import routing
from provenance import provenance
from run import ROOT, build_metadata, course, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--budgets', type=float, nargs='+', default=[.5, 2.])
    parser.add_argument('--repeat', type=int, default=3)
    parser.add_argument('--generated', action='store_true', help='Use held-out generated congestion cases')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.repeat < 1 or any(value <= 0 for value in args.budgets):
        parser.error('Invalid experiment budget')
    variants = [('layered', ['--router', 'layered']),
                ('reroute', ['--router', 'negotiated', '--history', '0']),
                ('history', ['--router', 'negotiated', '--history', '1'])]
    work = Path(tempfile.mkdtemp(prefix='routing-study-', dir=ROOT/'benchmarks/work'))
    rows = []
    environment = provenance(ROOT)
    cases = [routing(work/'inputs', seed) for seed in [101,211,307,409]] if args.generated else course.cases('Lab04')
    for name, inputs in cases:
        for budget in args.budgets:
            for repeat in range(args.repeat):
                for variant, flags in variants[repeat % 3:]+variants[:repeat % 3]:
                    settings = SimpleNamespace(bin_root=ROOT/'build', alpha=.5,
                        solver_arg=[*flags, '--seconds', str(budget), '--stats'], timeout=120)
                    folder = work/f'{name}-{budget}-{repeat}-{variant}'
                    folder.mkdir()
                    row = run(('Lab04', name, inputs, repeat), settings, folder)
                    row.update(variant=variant, budget_wall_seconds=budget)
                    summaries = [r for r in row['diagnostic_records'] if r.get('kind') == 'summary']
                    if summaries:
                        row['metrics'] = summaries[-1]
                        if row['valid']:
                            row['cost_agrees'] = abs(row['objective']-row['metrics']['objective']) <= .011
                    row['outcome'] = 'valid' if row['valid'] else (
                        'budget-exhausted' if row['status'] == 'failed' and 'Routing search budget exhausted' in row['solver_log']
                        else 'unexpected-failure')
                    rows.append(row)
    groups = sorted({(r['dataset'], r['budget_wall_seconds'], r['variant']) for r in rows})
    summary = []
    for name, budget, variant in groups:
        group = [r for r in rows if (r['dataset'], r['budget_wall_seconds'], r['variant']) == (name,budget,variant)]
        valid = [r for r in group if r['valid']]
        summary.append(dict(dataset=name, budget_wall_seconds=budget, variant=variant,
            legal=len(valid), attempts=len(group),
            median_objective=statistics.median(r['metrics']['objective'] for r in valid) if valid else None,
            median_wall_seconds=statistics.median(r['wall_seconds'] for r in group),
            median_peak_rss_kib=statistics.median(r['peak_rss_kib'] for r in group)))
    report = dict(schema_version=1, complete=True,
        **environment, build=build_metadata(ROOT/'build'),
        method='Serial rotating variant order. Equal maximum routing wall budgets include initial routing, exclude parsing/reporting. Wrapper wall time recorded separately. Solvers may finish early; failures retained.',
        dataset_family='held-out-generated-congestion' if args.generated else 'course',
        threads=1, results=rows, summary=summary)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    raise SystemExit(0 if all(r['outcome'] != 'unexpected-failure' and (not r['valid'] or r.get('cost_agrees',False)) for r in rows) else 1)


if __name__ == '__main__':
    main()
