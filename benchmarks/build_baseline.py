#!/usr/bin/env python3
"""Build historical solvers with the same release flags, without changing checkout."""
import argparse
from pathlib import Path
import json
import subprocess

ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--revision',default='364870c')
p.add_argument('--output',type=Path,default=ROOT/'benchmarks/work/original-release')
a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=True)
revision=subprocess.check_output(['git','rev-parse',a.revision],cwd=ROOT,text=True).strip()
paths=subprocess.check_output(['git','ls-tree','-r','--name-only',revision],cwd=ROOT,text=True).splitlines()
for lab,exe,inc in [('Lab01','Lab1','header'),('Lab02','Lab2',''),('Lab03','Legalizer','include'),('Lab04','D2DGRter','inc')]:
    sources=[]
    for name in paths:
        path=Path(name)
        if path.parts[0]!=lab or path.suffix not in ['.cpp','.h','.hpp']:continue
        target=a.output/path;target.parent.mkdir(parents=True,exist_ok=True)
        target.write_bytes(subprocess.check_output(['git','show',revision+':'+name],cwd=ROOT))
        if path.suffix=='.cpp':sources.append(str(target))
    command=['g++','-std=c++17','-O3','-DNDEBUG','-I'+str(a.output/lab/inc),*sources,'-o',str(a.output/lab/exe)]
    subprocess.run(command,check=True)
(a.output/'build-info.json').write_text(json.dumps({'revision':revision,'compiler':subprocess.check_output(['g++','--version'],text=True).splitlines()[0],'build_type':'Release','flags':'-std=c++17 -O3 -DNDEBUG; dynamic linking'},indent=2)+'\n')
