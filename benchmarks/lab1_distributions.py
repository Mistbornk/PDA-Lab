#!/usr/bin/env python3
"""Geometry distributions with independent final-adjacency answers; no arena rewrite."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import tempfile
from lab1_stitches import grid
from provenance import provenance
from run import ROOT, measured, build_metadata


def partition(kind, side, work):
    count = side*side
    if kind == 'dense':
        width = height = 4*side
        boxes = [(4*x,4*y,4,4) for y in range(side) for x in range(side)]
    elif kind == 'strips':
        width,height = count,32
        boxes = [(x,0,1,height) for x in range(count)]
    else:
        width = height = 128
        boxes = [(0,0,width,height)]
        rng = random.Random(811+count)
        while len(boxes) < count:
            i = max(range(len(boxes)),key=lambda j:boxes[j][2]*boxes[j][3])
            x,y,w,h = boxes.pop(i)
            vertical = w>1 and (h==1 or rng.randrange(2))
            split = (w if vertical else h)//2
            boxes.extend([(x,y,split,h),(x+split,y,w-split,h)] if vertical else
                         [(x,y,w,split),(x,y+split,w,h-split)])
    random.Random(919+count).shuffle(boxes)
    path = work/f'{kind}-{count}.txt'
    path.write_text(f'{width} {height}\n'+''.join(f'{i} {x} {y} {w} {h}\nP {x} {y}\n' for i,(x,y,w,h) in enumerate(boxes,1)))
    # The final layout completely tiles the outline: no empty tiles remain.
    # Count shared positive-length boundaries directly from final rectangles.
    expected = [str(count)]
    for i,(x,y,w,h) in enumerate(boxes,1):
        neighbors = sum((((x+w==a or a+W==x) and max(y,b)<min(y+h,b+H)) or
                         ((y+h==b or b+H==y) and max(x,a)<min(x+w,a+W))) for a,b,W,H in boxes)
        expected.append(f'{i} {neighbors} 0')
    expected += [f'{x} {y}' for x,y,w,h in boxes]
    return path,'\n'.join(expected).split()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repeat',type=int,default=3)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    if args.repeat<1: parser.error('Repeat must be positive')
    work=Path(tempfile.mkdtemp(prefix='distributions-',dir=ROOT/'benchmarks/work'))
    exe=ROOT/'build/Lab01/Lab1'; rows=[]
    report=dict(schema_version=1,**provenance(ROOT),build=build_metadata(ROOT/'build'),results=rows,complete=False,
                method='Serial alternating stitch variants. Full-coverage box adjacency oracle, or spaced-grid known answer. Per-insert time is end-to-end wall time divided by insert count, including parsing/reporting.')
    for kind in ['dense','strips','shared-edges','fragmented']:
        for side in [8,16,32]:
            path,expected=grid(side,work) if kind=='fragmented' else partition(kind,side,work)
            for repeat in range(args.repeat):
                for variant in (['indexed','scan'] if repeat%2==0 else ['scan','indexed']):
                    folder=work/f'{kind}-{side}-{repeat}-{variant}';folder.mkdir()
                    output=folder/'result.txt'
                    row=measured([exe,path,output,'--stitches',variant,'--stats'],folder,120)
                    row.update(dataset=f'{kind}-{side*side}',inserts=side*side,variant=variant,repeat=repeat,
                        executable_sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),input_sha256=hashlib.sha256(path.read_bytes()).hexdigest())
                    row['valid']=row['status']=='completed' and output.read_text().split()==expected
                    if row['valid']:
                        row['tile_count']=int(output.read_text().split()[0])
                        row['statistics']=json.loads((folder/'solver.log').read_text())
                    row['end_to_end_us_per_insert']=row['wall_seconds']*1e6/(side*side)
                    rows.append(row)
                    args.output.parent.mkdir(parents=True,exist_ok=True)
                    args.output.write_text(json.dumps(report,indent=2)+'\n')
                    print(row['dataset'],variant,row['valid'],flush=True)
    report['complete']=True
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    raise SystemExit(0 if all(r['valid'] for r in rows) else 1)


if __name__=='__main__': main()
