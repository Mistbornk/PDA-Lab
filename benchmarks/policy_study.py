#!/usr/bin/env python3
"""Predeclared paired policy experiments; separate tuning and held-out seeds/cases."""
import argparse
import json
from pathlib import Path
import statistics
import tempfile
from types import SimpleNamespace
from generated import floorplan
from provenance import provenance
from run import ROOT, build_metadata, course, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--suite', choices=['tuning','evaluation','instrumentation'], default='evaluation')
    parser.add_argument('--budgets', type=float, nargs='+', default=[1,3])
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if any(value <= 0 for value in args.budgets):
        parser.error('Budgets must be positive')
    work = Path(tempfile.mkdtemp(prefix='policy-study-', dir=ROOT/'benchmarks/work'))
    cases = list(course.cases('Lab02'))
    if args.suite == 'tuning':
        cases = [case for case in cases if case[0] == 'ami33']
        seeds = [1,7]
    else:
        cases += [floorplan(work/'inputs', 35, 503), floorplan(work/'inputs', 90, 509)]
        seeds = [101,211,307,409]
    variants = [('legacy', ['--policy','legacy']), ('progress',['--policy','progress']),
                ('feasibility',['--policy','feasibility'])]
    if args.suite == 'instrumentation':
        cases = [case for case in cases if case[0] == 'ami33']; seeds = [1,7,19]
        variants = [('plain',['--policy','feasibility']),
                    ('stats',['--policy','feasibility','--stats']),
                    ('trace',['--policy','feasibility','--trace-every','5000'])]
    rows = []
    budgets = [300000] if args.suite == 'instrumentation' else args.budgets
    expected = len(cases)*len(seeds)*len(budgets)*len(variants)
    report = dict(schema_version=1, suite=args.suite, **provenance(ROOT),
                  build=build_metadata(ROOT/'build'), threads=1, expected_runs=expected,
                  seeds=seeds, budgets=budgets, generated_seeds=[503,509], results=rows,
                  method='Serial paired seeds, rotating policy order. Single restart. Fixed CPU budgets for quality; fixed iterations for instrumentation. All failures retained; cost pairs require both legal.',
                  complete=False)
    for name, inputs in cases:
        for budget in budgets:
            for order, seed in enumerate(seeds):
                for variant, flags in variants[order % len(variants):]+variants[:order % len(variants)]:
                    extra = ['--seed',str(seed), '--iterations' if args.suite == 'instrumentation' else '--seconds', str(budget)]
                    if args.suite != 'instrumentation': extra += ['--trace-every','5000']
                    settings = SimpleNamespace(bin_root=ROOT/'build',alpha=.5,
                        solver_arg=flags+extra,timeout=max(120, budget*4 if args.suite!='instrumentation' else 120))
                    row = run(('Lab02',name,inputs,len(rows)),settings,work)
                    row.update(policy=variant,seed=seed,budget=budget)
                    row['outcome'] = 'valid' if row['valid'] else (
                        'budget-exhausted' if row['status']=='failed' and 'No legal floorplan found within budget' in row['solver_log'] else 'unexpected-failure')
                    rows.append(row)
                    args.output.parent.mkdir(parents=True,exist_ok=True)
                    args.output.write_text(json.dumps(report,indent=2)+'\n')
    report['summary'] = []
    for name, _ in cases:
        for budget in budgets:
            for policy, _ in variants:
                group = [r for r in rows if r['dataset']==name and r['budget']==budget and r['policy']==policy]
                valid = [r for r in group if r['valid']]
                first = [r['diagnostics']['search']['first_legal_cpu_seconds'] for r in valid if 'diagnostics' in r]
                report['summary'].append(dict(dataset=name,budget=budget,policy=policy,attempts=len(group),legal=len(valid),
                    median_legal_objective=statistics.median(r['objective'] for r in valid) if valid else None,
                    median_first_legal_seconds=statistics.median(x for x in first if x is not None) if any(x is not None for x in first) else None,
                    median_wall_seconds=statistics.median(r['wall_seconds'] for r in group),
                    median_peak_rss_kib=statistics.median(r['peak_rss_kib'] for r in group)))
    pairs = []
    if args.suite != 'instrumentation':
        for name,_ in cases:
            for budget in budgets:
                for policy in ['progress','feasibility']:
                    changes=[];wins=ties=losses=0
                    for seed in seeds:
                        a=next(r for r in rows if (r['dataset'],r['budget'],r['seed'],r['policy'])==(name,budget,seed,'legacy'))
                        b=next(r for r in rows if (r['dataset'],r['budget'],r['seed'],r['policy'])==(name,budget,seed,policy))
                        if a['valid'] and b['valid']:
                            wins+=b['objective']<a['objective'];ties+=b['objective']==a['objective'];losses+=b['objective']>a['objective']
                            if a['objective']:changes.append(100*(b['objective']-a['objective'])/a['objective'])
                    pairs.append(dict(dataset=name,budget=budget,policy=policy,wins=wins,ties=ties,losses=losses,
                                      paired_percent_changes=changes))
    report['paired_quality']=pairs
    report['complete']=True
    if args.suite=='instrumentation':
        report['identical_solutions']=all(len({r.get('solution_sha256') for r in rows if r['seed']==seed})==1 for seed in seeds)
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    raise SystemExit(0 if all(r['outcome']!='unexpected-failure' for r in rows) and report.get('identical_solutions',True) else 1)


if __name__ == '__main__':
    main()
