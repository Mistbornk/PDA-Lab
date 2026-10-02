#!/usr/bin/env python3
"""Normalize interview studies, retain failures, and export reviewable charts/tables."""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import statistics
from provenance import provenance
from run import ROOT

STUDIES = ['routing-study','routing-generated','policy-evaluation','policy-instrumentation',
           'legalizer-study','legalizer-generated']
COLORS = ['#475569','#0369a1','#b45309']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=ROOT/'docs/experiments')
    args = parser.parse_args()
    args.output.mkdir(parents=True,exist_ok=True)
    studies = {}
    flat = []; artifacts = []
    for name in STUDIES:
        path = ROOT/'benchmarks/results'/f'interview-{name}.json'
        data = json.loads(path.read_text())
        if not data.get('complete'):
            raise ValueError(f'Incomplete study: {path}')
        studies[name] = data
        artifacts.append(dict(path=str(path.relative_to(ROOT)),sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                              revision=data.get('revision'),dirty=data.get('dirty'),method=data['method']))
        for r in data['results']:
            flat.append(dict(study=name,lab=r['lab'],dataset=r['dataset'],
                strategy=r.get('policy',r.get('variant',r.get('strategy'))),
                seed=r.get('seed'),repeat=r.get('timing_repeat',r['repeat']),
                budget=r.get('budget',r.get('budget_wall_seconds')),
                budget_unit='iterations' if name=='policy-instrumentation' else 'CPU seconds' if name=='policy-evaluation'
                            else 'routing wall seconds' if name.startswith('routing') else 'none',
                wall_seconds=r['wall_seconds'],cpu_seconds=r.get('user_seconds',0)+r.get('system_seconds',0),
                peak_rss_kib=r.get('peak_rss_kib'),valid=r['valid'],objective=r.get('objective'),
                outcome=r.get('outcome','valid' if r['valid'] else 'unclassified-failure'),threads=r['solver_threads'],executable_sha256=r['executable_sha256'],
                input_sha256=r['input_sha256'],parameters=r['parameters'],
                compiler=data['build']['compiler'],build_type=data['build']['build_type'],
                failure_reason=None if r['valid'] else '\n'.join(s for s in r['solver_log'].splitlines() if not s.startswith('{'))))
    report = dict(schema_version=1,generated_with=provenance(ROOT),artifacts=artifacts,results=flat,
                  note='Generation provenance is not retroactive run provenance. Exact run commands, flags, input/executable hashes, traces and validator logs remain in each linked raw artifact. Historical dirty trees are explicitly retained.')
    (args.output/'summary.json').write_text(json.dumps(report,indent=2)+'\n')
    with (args.output/'summary.csv').open('w',newline='') as stream:
        writer = csv.DictWriter(stream,fieldnames=list(flat[0]),lineterminator='\n')
        writer.writeheader()
        writer.writerows({k:json.dumps(v) if isinstance(v,(dict,list)) else v for k,v in r.items()} for r in flat)
    lines = ['# Measured interview experiments','',
        'Generated from complete raw reports with `python3 benchmarks/report.py`. All failed attempts remain in [JSON](summary.json) and [CSV](summary.csv).', '',
        'Timing repetitions estimate run-to-run variation; solver seeds measure search variation. Runs were serial on a shared Intel Xeon E5-2620 v4 host, GCC 11.4 Release. External load and CPU frequency were not controlled. No dedicated-runner performance threshold is claimed.', '',
        '![Routing quality and runtime](routing.svg)','![Floorplan anytime examples](floorplanning.svg)',
        '![Legalization cost and runtime](legalization.svg)','']
    for name,data in studies.items():
        lines += [f'## {name}', '', data['method'], '',
            f'[Raw evidence](../../benchmarks/results/interview-{name}.json). Runtime range is min–max across all attempts, including failures. Cost range includes only legal solutions.', '',
            '| Dataset | Strategy | Budget | Legal / attempts | Legal cost: median [min, max] | Wall seconds: median [min, max] | Peak RSS KiB: median |',
            '|---|---|---|---|---|---|---|']
        groups = {}
        for r in flat:
            if r['study']==name:
                groups.setdefault((r['dataset'],r['strategy'],r['budget']),[]).append(r)
        for (dataset,strategy,budget),group in groups.items():
            costs = [r['objective'] for r in group if r['valid']]
            times = [r['wall_seconds'] for r in group]
            def span(values):
                return f'{statistics.median(values):.4g} [{min(values):.4g}, {max(values):.4g}]' if values else '—'
            lines.append(f'| {dataset} | {strategy} | {budget if budget is not None else "—"} | {len(costs)}/{len(group)} | {span(costs)} | {span(times)} | {statistics.median(r["peak_rss_kib"] for r in group):g} |')
        lines += ['']
    lines += ['## Interpretation', '',
        '- Routing: official cases show no original-cost improvement and more runtime. Generated congestion cases improve under plain rerouting; history=1 is weaker in this sample. Best-result retention prevents returning a worse round within one run.',
        '- Floorplanning: progress and feasibility succeed in all 40 evaluated attempts each; legacy succeeds in 12/40. On the 12 mutually legal pairs each new policy wins 8 and loses 4. In vda317b, early feasibility sacrifices substantial quality. The default remains legacy.',
        '- Legalization: minimum is optimal only for a single insertion with all other cells fixed. Repair minimizes an immediate bounded candidate score; changed placements affect future steps, so a whole-sequence score can regress. Examine all three strategies and Move Times, not just new-cell distance.',
        '- Diagnostic ablation: fixed-seed solutions are identical with stats off/on. Sampling avoids reading component timers on every evaluation. Three repetitions are insufficient for a universal overhead claim.', '',
        'The six predeclared generated routing/policy seeds and the legalization distribution parameters remain in the generator scripts and raw input hashes. Generator-format rejection records and initial diagnostic experiments are retained separately, without counting rejected inputs as algorithm wins.', '']
    (args.output/'README.md').write_text('\n'.join(lines))

    # Standalone scientific artifacts; matplotlib is needed only for this report,
    # not for the solvers, tests or interactive offline demonstration.
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    plt.rcParams.update({'svg.hashsalt':'pda-lab','font.size':9,'axes.spines.top':False,'axes.spines.right':False})
    def save(fig,name):
        fig.tight_layout()
        fig.savefig(args.output/(name+'.svg'),metadata={'Date':None})
        path=args.output/(name+'.svg')
        path.write_text('\n'.join(line.rstrip() for line in path.read_text().splitlines())+'\n')
        plt.close(fig)
    from matplotlib.lines import Line2D
    fig,axes = plt.subplots(1,2,figsize=(11,5))
    for ax,(key,budget,title) in zip(axes,[('routing-study',2.,'Official cases'),('routing-generated',1.,'Generated congestion')]):
        data=studies[key]
        names=sorted({r['dataset'] for r in data['results']})
        for v,(variant,color) in enumerate(zip(['layered','reroute','history'],COLORS)):
            for i,dataset in enumerate(names):
                group=[r for r in data['results'] if r['dataset']==dataset and r['variant']==variant and r['budget_wall_seconds']==budget and r['valid']]
                if not group: continue
                base=[r['metrics']['objective'] for r in data['results'] if r['dataset']==dataset and r['variant']=='layered' and r['budget_wall_seconds']==budget and r['valid']]
                x=statistics.median(r['wall_seconds'] for r in group)
                y=statistics.median(r['metrics']['objective'] for r in group)/statistics.median(base)
                ax.scatter(x,y,color=color,marker=['o','s','^','D'][i],label=variant if i==0 else None)
        ax.set(title=title,xlabel='Solver wall seconds (median)',ylabel='Original cost / one-pass cost')
        ax.grid(alpha=.2)
        algorithms=ax.legend()
        ax.add_artist(algorithms)
        cases=[Line2D([],[],color='#475569',linestyle='none',marker=['o','s','^','D'][i],label=dataset.replace('congestion-24x24-40-','seed ')) for i,dataset in enumerate(names)]
        ax.legend(handles=cases,loc='upper center',bbox_to_anchor=(.5,-.22),ncol=2,fontsize=8)
    save(fig,'routing')
    fig,axes=plt.subplots(1,3,figsize=(12,3.8))
    for ax,dataset in zip(axes[:2],['ami33','vda317b']):
        for policy,color in zip(['legacy','progress','feasibility'],COLORS):
            r=next(r for r in studies['policy-evaluation']['results'] if r['dataset']==dataset and r['budget']==3 and r['seed']==101 and r['policy']==policy)
            trace=[p for p in r['diagnostics']['search']['trace'] if p['best_objective'] is not None]
            ax.step([p['cpu_seconds'] for p in trace],[p['best_objective'] for p in trace],where='post',label=policy,color=color,marker='o' if len(trace)==1 else None)
        ax.set(title=dataset+' · seed 101',xlabel='Search CPU seconds',ylabel='Best legal objective')
        ax.grid(alpha=.2);ax.legend()
    policies=['legacy','progress','feasibility']
    axes[2].bar(policies,[sum(r['valid'] for r in studies['policy-evaluation']['results'] if r['policy']==p) for p in policies],color=COLORS)
    axes[2].set(title='All five cases × four seeds × two budgets',ylabel='Legal attempts / 40',ylim=(0,42))
    save(fig,'floorplanning')
    fig,axes=plt.subplots(1,2,figsize=(11,4))
    data=studies['legalizer-study']; names=sorted({r['dataset'] for r in data['results']})
    for ax,metric,title in zip(axes,['objective','wall_seconds'],['Original total cost','Solver wall time']):
        for v,(variant,color) in enumerate(zip(['legacy','minimum','repair'],COLORS)):
            ratios=[]
            for dataset in names:
                group=[r[metric] for r in data['results'] if r['dataset']==dataset and r['strategy']==variant and r['valid']]
                base=[r[metric] for r in data['results'] if r['dataset']==dataset and r['strategy']=='legacy' and r['valid']]
                ratios.append(statistics.median(group)/statistics.median(base) if group and base else float('nan'))
            ax.bar([i+(v-1)*.25 for i in range(len(names))],ratios,width=.25,label=variant,color=color)
        ax.set(title=title,ylabel='Median / legacy median',xticks=range(len(names)),xticklabels=[s.replace('testcase','tc').replace('MBFF_LIB','MBFF') for s in names])
        ax.tick_params(axis='x',rotation=25);ax.legend();ax.grid(axis='y',alpha=.2)
    save(fig,'legalization')
    print(f'Wrote {len(flat)} attempts and three SVG figures to {args.output}')


if __name__ == '__main__':
    main()
