#!/usr/bin/env python3
"""Measured solver runs, validated by course tools. Outputs reproducible JSON."""
import argparse
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime,timezone
import hashlib
import importlib.util
import json
from pathlib import Path
import platform
import re
import subprocess
import tempfile
import time

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('course',ROOT/'tests/run.py')
course=importlib.util.module_from_spec(spec);spec.loader.exec_module(course)


def measured(command,work,timeout):
    timing=work/'time.txt'
    record=course.execute(['/usr/bin/time','-f','%e %U %S %M','-o',str(timing),*map(str,command)],
                          work/'solver.log',timeout,work)
    code=record['returncode']
    if code is not None and timing.exists():
        values=timing.read_text().splitlines()[-1].split()
        if len(values)==4:
            record.update(zip(['time_wall_seconds','user_seconds','system_seconds','peak_rss_kib'],map(float,values)))
    return record


def run(task,args,root):
    lab,name,inputs,repeat=task
    work=root/f'{lab}-{name}-{repeat}';work.mkdir()
    output=work/(name+('.txt' if lab=='Lab01' else '.rpt' if lab=='Lab02' else '.lg'))
    exe=(args.bin_root/lab/course.EXECUTABLES[lab]).resolve()
    command=[exe]+([str(args.alpha)] if lab=='Lab02' else [])+inputs+[output]+args.solver_arg
    result={'lab':lab,'dataset':name,'repeat':repeat,'executable':str(exe),
            'executable_sha256':hashlib.sha256(exe.read_bytes()).hexdigest(),
            'input_sha256':{str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs},
            'parameters':args.solver_arg,'alpha':args.alpha if lab=='Lab02' else None,
            'solver_threads':1,'command':list(map(str,command))}
    result.update(measured(command,work,args.timeout));result['valid']=False
    if result['status']=='completed' and output.exists():
        text=output.read_text();result['output_sha256']=hashlib.sha256(output.read_bytes()).hexdigest()
        if lab=='Lab01':
            expected=ROOT/lab/'tests/expected'/(name.replace('case','output')+'.txt')
            result['valid']=text.split()==expected.read_text().split()
        else:
            if lab=='Lab02':check=[ROOT/lab/'tests/tools/verifier',str(args.alpha),*inputs,output]
            elif lab=='Lab03':check=[ROOT/lab/'tests/tools/upstream/Evaluator',*inputs,output]
            else:
                import shutil
                for path in inputs:shutil.copy2(path,work/path.name)
                check=[ROOT/lab/'tests/tools/upstream/Evaluator/Evaluator',work,name]
            log=work/'validator.log';validation=course.execute(check,log,300,work)
            result['valid']=course.verdict(lab,validation,log)
            message=course.ANSI.sub('',log.read_text());result['validator_log']=message
            if lab=='Lab02' and result['valid']:
                rows=text.splitlines();result['objective']=int(rows[0]);result['hpwl']=int(rows[1]);result['area']=int(rows[2])
                result['solution_sha256']=hashlib.sha256(('\n'.join(rows[:4]+rows[5:])).encode()).hexdigest()
            else:
                for row in message.splitlines():
                    if re.match(r'\|\s*Total\s*\|',row): result['objective']=float(row.split('|')[4].strip())
    result['solver_log']=(work/'solver.log').read_text(errors='replace')
    if lab=='Lab02':
        for line in result['solver_log'].splitlines():
            if line.startswith('{'):
                result['diagnostics']=json.loads(line)
                result['solver_threads']=result['diagnostics'].get('threads',1)
    print(f'{lab}/{name} #{repeat}: {result["status"]}, valid={result["valid"]}, {result["wall_seconds"]:.3f}s',flush=True)
    return result


def build_metadata(bin_root):
    build_info=bin_root/'build-info.json'
    if build_info.exists():
        metadata=json.loads(build_info.read_text())
    else:
        cache=bin_root/'CMakeCache.txt'
        values={line.split(':',1)[0]:line.split('=',1)[1] for line in cache.read_text().splitlines()
                if '=' in line and ':' in line and not line.startswith(('#','//'))} if cache.exists() else {}
        compiler=values.get('CMAKE_CXX_COMPILER')
        metadata={'compiler':subprocess.check_output([compiler,'--version'],text=True).splitlines()[0] if compiler else 'unknown (no build metadata)',
                  'build_type':values.get('CMAKE_BUILD_TYPE','unknown'),
                  'flags':{key:values.get(key) for key in ['CMAKE_CXX_FLAGS','CMAKE_CXX_FLAGS_RELEASE','CMAKE_CXX_FLAGS_DEBUG','PDA_SANITIZERS','PDA_STRICT_WARNINGS','PDA_WARNINGS_AS_ERRORS']}}
    return metadata


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--bin-root',type=Path,default=ROOT/'build')
    p.add_argument('--lab',choices=list(course.EXECUTABLES),required=True)
    p.add_argument('--case',action='append',default=[])
    p.add_argument('--repeat',type=int,default=3)
    p.add_argument('--jobs',type=int,default=1)
    p.add_argument('--solver-arg',action='append',default=[])
    p.add_argument('--alpha',type=float,default=.5)
    p.add_argument('--timeout',type=float,default=1800)
    p.add_argument('--label',required=True)
    p.add_argument('--output',type=Path,required=True)
    args=p.parse_args()
    if args.jobs<1 or args.repeat<1 or args.timeout<=0 or not 0<=args.alpha<=1:p.error('Invalid count, timeout or alpha')
    args.bin_root=args.bin_root.resolve()
    selected=[(name,paths) for name,paths in course.cases(args.lab) if not args.case or name in args.case]
    if not selected or any(name not in [n for n,_ in selected] for name in args.case):p.error('Missing dataset; fetch resources first')
    tasks=[(args.lab,name,paths,i) for i in range(args.repeat) for name,paths in selected]
    work_parent=ROOT/'benchmarks/work';work_parent.mkdir(parents=True,exist_ok=True)
    work=Path(tempfile.mkdtemp(prefix='run-',dir=work_parent))
    started=time.perf_counter()
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:results=list(pool.map(lambda t:run(t,args,work),tasks))
    metadata=build_metadata(args.bin_root)
    report={'label':args.label,'timestamp':datetime.now(timezone.utc).isoformat(),'platform':platform.platform(),
            'cpu':next((line.split(':',1)[1].strip() for line in Path('/proc/cpuinfo').read_text().splitlines() if line.startswith('model name')),None),
            'build':metadata,'jobs':args.jobs,'batch_wall_seconds':time.perf_counter()-started,'work_dir':str(work),
            'note':'jobs measures independent case concurrency; solver_threads records internal parallelism. Validator time excluded from per-solver timing; included in batch timing.',
            'results':results}
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2)+'\n')
    raise SystemExit(0 if all(r['valid'] for r in results) else 1)

if __name__=='__main__':main()
