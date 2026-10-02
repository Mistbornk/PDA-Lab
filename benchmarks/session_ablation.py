#!/usr/bin/env python3
"""Paired Lab03 transaction-filter ablation, with unchanged official output required."""
import argparse
import hashlib
import json
from pathlib import Path
import platform
import statistics
import tempfile
from types import SimpleNamespace
from run import ROOT, build_metadata, course, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repeat', type=int, default=3)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    work = Path(tempfile.mkdtemp(prefix='sessions-', dir=ROOT/'benchmarks/work'))
    case = next(item for item in course.cases('Lab03') if item[0] == 'testcase1_16900')
    variants = [('baseline', ROOT/'benchmarks/work/revision-616d52e77704/build'),
                ('transaction-hash', ROOT/'benchmarks/work/interview-m1-unordered'),
                ('transaction-epoch', ROOT/'build')]
    rows = []
    for repetition in range(args.repeat):
        for label, binary in variants[repetition % 3:]+variants[:repetition % 3]:
            folder = work/f'{label}-{repetition}'
            folder.mkdir()
            options = SimpleNamespace(bin_root=binary, alpha=.5, solver_arg=[], timeout=600)
            result = run(('Lab03', *case, repetition), options, folder)
            result['variant'] = label
            rows.append(result)
    report = dict(schema_version=1, platform=platform.platform(),
                  method='Serial runs; order rotates across repetitions. Verifier excluded from solver time.',
                  source_hashes={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest()
                                 for p in (ROOT/'Lab03').rglob('*.cpp')},
                  build=build_metadata(ROOT/'build'), results=rows,
                  identical_outputs=len({r['output_sha256'] for r in rows}) == 1,
                  summary={label:dict(median_wall_seconds=statistics.median(r['wall_seconds'] for r in rows if r['variant']==label),
                                      median_rss_kib=statistics.median(r['peak_rss_kib'] for r in rows if r['variant']==label))
                           for label, _ in variants})
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2)+'\n')
    raise SystemExit(0 if report['identical_outputs'] and all(r['valid'] for r in rows) else 1)


if __name__ == '__main__':
    main()
