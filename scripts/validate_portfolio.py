#!/usr/bin/env python3
"""Record strict builds, sanitizers, bounded fuzzing and static analysis locally."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'benchmarks'))
from provenance import provenance


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--jobs',type=int,default=4)
    parser.add_argument('--output',type=Path,default=ROOT/'docs/interview-final-validation.json')
    args=parser.parse_args()
    if args.jobs<1: parser.error('Jobs must be positive')
    parent=ROOT/'benchmarks/work';parent.mkdir(parents=True,exist_ok=True)
    work=Path(tempfile.mkdtemp(prefix='final-validation-',dir=parent))
    report=dict(schema_version=1,**provenance(ROOT),checks=[],complete=False)
    env=dict(os.environ,ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1',TSAN_OPTIONS='halt_on_error=1')
    commands=[]
    for name,compiler,kind,flags in [('build','c++','Release',[]),
            ('build-clang-strict','clang++-14','Release',[]),
            ('build-asan','c++','Debug',['-DPDA_SANITIZERS=ON'])]:
        commands += [(name+'-configure',['cmake','-S','.', '-B',name,'-DCMAKE_CXX_COMPILER='+compiler,
                    '-DCMAKE_BUILD_TYPE='+kind,'-DPDA_STRICT_WARNINGS=ON','-DPDA_WARNINGS_AS_ERRORS=ON',*flags]),
                     (name+'-compile',['cmake','--build',name,'--parallel',str(args.jobs)]),
                     (name+'-test',['ctest','--test-dir',name,'--output-on-failure'])]
    commands += [
        ('tsan-configure',['cmake','-S','.', '-B','build-tsan','-DCMAKE_CXX_COMPILER=clang++-14','-DCMAKE_BUILD_TYPE=RelWithDebInfo','-DPDA_THREAD_SANITIZER=ON','-DPDA_STRICT_WARNINGS=ON','-DPDA_WARNINGS_AS_ERRORS=ON']),
        ('tsan-build',['cmake','--build','build-tsan','--parallel',str(args.jobs),'--target','lab2_parallel_test','lab2_random_test','lab2_policy_test']),
        ('tsan-test',['ctest','--test-dir','build-tsan','-R','lab2_(parallel|random|policy)','--output-on-failure']),
        ('fuzz-configure',['cmake','-S','.', '-B','build-fuzz','-DCMAKE_CXX_COMPILER=clang++-14','-DCMAKE_BUILD_TYPE=RelWithDebInfo','-DPDA_FUZZING=ON','-DBUILD_TESTING=OFF','-DPDA_STRICT_WARNINGS=ON','-DPDA_WARNINGS_AS_ERRORS=ON']),
        ('fuzz-build',['cmake','--build','build-fuzz','--parallel',str(args.jobs),'--target','fuzz_lab1','fuzz_lab2','fuzz_lab3','fuzz_lab4']),
        ('fuzz-test',[sys.executable,'tests/fuzz/run.py','--runs','20000','--seconds','30','--output','benchmarks/results/interview-final-fuzz.json']),
        ('static-analysis',[sys.executable,'tests/analyze.py','--build','build-fuzz','--output','benchmarks/results/interview-final-analysis.json']),
        ('pinned-resources',[sys.executable,'tests/fetch_resources.py','--check']),
        ('whitespace',['git','diff','--check'])]
    for name,command in commands:
        log=work/(name+'.log');start=time.perf_counter()
        with log.open('w') as stream:
            try:
                result=subprocess.run(command,cwd=ROOT,stdout=stream,stderr=subprocess.STDOUT,env=env,timeout=1200)
                code=result.returncode
            except subprocess.TimeoutExpired:
                code=None
        report['checks'].append(dict(name=name,command=command,returncode=code,status='passed' if code==0 else 'failed',
                                   wall_seconds=time.perf_counter()-start,log=str(log.relative_to(ROOT)),log_tail=log.read_text()[-6000:]))
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(json.dumps(report,indent=2)+'\n')
        print(name,report['checks'][-1]['status'],flush=True)
        if code!=0:
            print(log.read_text()[-6000:])
            raise SystemExit(1)
    report['complete']=True
    args.output.write_text(json.dumps(report,indent=2)+'\n')


if __name__=='__main__': main()
