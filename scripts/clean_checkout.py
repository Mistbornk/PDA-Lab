#!/usr/bin/env python3
"""Build/test an archived committed tree, then check all four compatibility Makefiles."""
import argparse
import json
from pathlib import Path
import subprocess
import tempfile
import time

ROOT=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--revision',default='HEAD')
    parser.add_argument('--jobs',type=int,default=4)
    parser.add_argument('--output',type=Path,default=ROOT/'docs/interview-clean-checkout.json')
    args=parser.parse_args()
    if args.jobs<1: parser.error('Jobs must be positive')
    revision=subprocess.check_output(['git','rev-parse','--verify',args.revision+'^{commit}'],cwd=ROOT,text=True).strip()
    parent=ROOT/'benchmarks/work';parent.mkdir(parents=True,exist_ok=True)
    work=Path(tempfile.mkdtemp(prefix='clean-checkout-',dir=parent));source=work/'source';source.mkdir()
    archive=work/'source.tar'
    subprocess.run(['git','archive','--format=tar','-o',str(archive),revision],cwd=ROOT,check=True)
    subprocess.run(['tar','-xf',str(archive),'-C',str(source)],check=True)
    archive.unlink()
    report=dict(schema_version=1,revision=revision,source=str(source.relative_to(ROOT)),complete=False,
                method='Fresh git archive of the committed tree; no ignored datasets, binaries or build cache copied. Root CMake/CTest includes the offline demo. Compatibility Makefiles run only in the isolated archive.',checks=[])
    commands=[('configure',['cmake','-S','.', '-B','build','-DCMAKE_BUILD_TYPE=Release','-DPDA_STRICT_WARNINGS=ON','-DPDA_WARNINGS_AS_ERRORS=ON']),
              ('build',['cmake','--build','build','--parallel',str(args.jobs)]),
              ('test',['ctest','--test-dir','build','--output-on-failure'])]
    commands += [(lab+'-make',['make','-B','-C',lab]) for lab in ['Lab01','Lab02','Lab03','Lab04']]
    for name,command in commands:
        log=work/(name+'.log');began=time.perf_counter()
        with log.open('w') as stream:
            result=subprocess.run(command,cwd=source,stdout=stream,stderr=subprocess.STDOUT,timeout=1200)
        report['checks'].append(dict(name=name,command=command,returncode=result.returncode,
            status='passed' if result.returncode==0 else 'failed',wall_seconds=time.perf_counter()-began,
            log=str(log.relative_to(ROOT)),log_tail=log.read_text()[-6000:]))
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(json.dumps(report,indent=2)+'\n')
        print(name,report['checks'][-1]['status'],flush=True)
        if result.returncode:
            print(log.read_text()[-6000:]);raise SystemExit(1)
    report['complete']=True
    args.output.write_text(json.dumps(report,indent=2)+'\n')


if __name__=='__main__':main()
