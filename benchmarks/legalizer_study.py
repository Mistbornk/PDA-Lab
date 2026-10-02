#!/usr/bin/env python3
"""Official score parity and runtime/quality comparison for legalization strategies."""
import argparse
import json
from pathlib import Path
import re
import statistics
import tempfile
from types import SimpleNamespace
from generated import legalization
from provenance import provenance
from run import ROOT, build_metadata, course, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repeat', type=int, default=3)
    parser.add_argument('--generated', action='store_true')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.repeat < 1:
        parser.error('Repeat must be positive')
    work = Path(tempfile.mkdtemp(prefix='legalizer-study-', dir=ROOT/'benchmarks/work'))
    cases = ([legalization(work/'inputs', d, h, 613+d+h) for d in [40,70,90] for h in [1,2]]
             if args.generated else course.cases('Lab03'))
    rows = []
    report = dict(schema_version=1, **provenance(ROOT), build=build_metadata(ROOT/'build'),
                  method='Serial rotating strategy order; deterministic repetitions are timing samples, not algorithm seeds. Original evaluator checks every step and total score.',
                  threads=1, family='generated' if args.generated else 'course', results=rows,
                  expected_runs=len(cases)*3*args.repeat, complete=False)
    for name, inputs in cases:
        for repeat in range(args.repeat):
            variants = ['legacy','minimum','repair']
            for variant in variants[repeat%3:]+variants[:repeat%3]:
                settings = SimpleNamespace(bin_root=ROOT/'build',alpha=.5,timeout=1800,
                    solver_arg=['--strategy',variant,'--stats'])
                row = run(('Lab03',name,inputs,len(rows)),settings,work)
                row.update(strategy=variant, timing_repeat=repeat)
                row['metrics'] = row['diagnostic_records'][-1] if row['diagnostic_records'] else {}
                row['outcome'] = ('valid' if row['valid'] else 'no-placement' if row['returncode']==3
                                  else 'unexpected-failure')
                if row['valid']:
                    for label, key in [('Move Times','move_times'),('Total Distance','total_distance')]:
                        line = next(s for s in row['validator_log'].splitlines() if re.match(r'\|\s*'+label+r'\s*\|',s))
                        row['evaluator_'+key] = float(line.split('|')[2].strip())
                    row['cost_agrees'] = (abs(row['objective']-row['metrics']['objective']) <= .011
                        and all(abs(row['evaluator_'+k]-row['metrics'][k]) <= .011 for k in ['move_times','total_distance']))
                rows.append(row)
                args.output.parent.mkdir(parents=True,exist_ok=True)
                args.output.write_text(json.dumps(report,indent=2)+'\n')
    report['summary'] = []
    for name,_ in cases:
        for variant in ['legacy','minimum','repair']:
            group = [r for r in rows if r['dataset']==name and r['strategy']==variant]
            valid = [r for r in group if r['valid']]
            report['summary'].append(dict(dataset=name,strategy=variant,legal=len(valid),attempts=len(group),
                median_objective=statistics.median(r['objective'] for r in valid) if valid else None,
                median_wall_seconds=statistics.median(r['wall_seconds'] for r in group),
                median_peak_rss_kib=statistics.median(r['peak_rss_kib'] for r in group),
                identical_solutions=len({r.get('output_sha256') for r in valid})<=1))
    report['complete'] = True
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    raise SystemExit(0 if all(r['outcome']!='unexpected-failure' and (not r['valid'] or r['cost_agrees']) for r in rows) else 1)


if __name__ == '__main__':
    main()
