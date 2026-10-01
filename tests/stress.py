#!/usr/bin/env python3
"""Generated scaling checks with independent legality and analytic cost oracles."""
import argparse
import hashlib
import json
from pathlib import Path
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'benchmarks'))
from run import measured, build_metadata
from lab1_stitches import grid


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--bin-root', type=Path, default=ROOT/'build')
    p.add_argument('--scale', choices=['smoke', 'large'], default='smoke')
    p.add_argument('--output', type=Path, required=True)
    args = p.parse_args(); args.bin_root = args.bin_root.resolve()
    parent = ROOT/'benchmarks/work'; parent.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix='stress-', dir=parent))
    rows = []; large = args.scale == 'large'

    def execute(lab, exe, inputs, options, dimensions):
        directory = work/lab; directory.mkdir()
        output = directory/'result.txt'
        command = [args.bin_root/lab/exe, *inputs, output, *options]
        record = measured(command, directory, 240)
        record.update(lab=lab, dimensions=dimensions, command=list(map(str, command)), solver_threads=1,
                      executable_sha256=hashlib.sha256(command[0].read_bytes()).hexdigest(),
                      inputs={str(path):hashlib.sha256(path.read_bytes()).hexdigest() for path in inputs if isinstance(path, Path)})
        rows.append(record)
        assert record['status']=='completed', (lab, (directory/'solver.log').read_text())
        record['output_sha256'] = hashlib.sha256(output.read_bytes()).hexdigest()
        return output.read_text(), record

    side = 100 if large else 12
    path, expected = grid(side, work)
    text, record = execute('Lab01', 'Lab1', [path], [], dict(blocks=side*side))
    assert text.split() == expected
    record['valid'] = True

    count = 499 if large else 40
    sizes = [(100000+i%7*200, 200000+i%11*300) for i in range(count)]
    block = work/'scale.block'; net = work/'scale.nets'
    block.write_text(f'Outline: 2000000000 2000000000\nNumBlocks: {count}\nNumTerminals: 0\n'+
                     ''.join(f'b{i} {w} {h}\n' for i,(w,h) in enumerate(sizes)))
    nets = [[f'b{(i+j)%count}' for j in range(8)] for i in range(min(100,count))]
    net.write_text(f'NumNets: {len(nets)}\n'+''.join('NetDegree: 8\n'+'\n'.join(n)+'\n' for n in nets))
    text, record = execute('Lab02', 'Lab2', ['0.5', block, net], ['--iterations','2000','--seed','43'],
                           dict(macros=count, outline=2000000000))
    lines = text.splitlines(); placed = {}
    for row in lines[5:]:
        name,*values = row.split(); x,y,X,Y = map(int,values)
        assert name not in placed
        assert sorted([X-x,Y-y]) == sorted(sizes[int(name[1:])])
        assert 0 <= x < X <= 2000000000 and 0 <= y < Y <= 2000000000
        for a,b,A,B in placed.values(): assert not (max(x,a)<min(X,A) and max(y,b)<min(Y,B))
        placed[name] = (x,y,X,Y)
    assert set(placed) == {f'b{i}' for i in range(count)}
    centers = {n:(x+(X-x)//2,y+(Y-y)//2) for n,(x,y,X,Y) in placed.items()}
    hpwl = sum(max(centers[n][0] for n in group)-min(centers[n][0] for n in group)+
               max(centers[n][1] for n in group)-min(centers[n][1] for n in group) for group in nets)
    area = max(b[2] for b in placed.values())*max(b[3] for b in placed.values())
    assert list(map(int,lines[:3])) == [int(.5*area+.5*hpwl), hpwl, area]
    record.update(valid=True, objective=int(lines[0]), hpwl=hpwl, area=area)

    count = 200000 if large else 2000
    columns = 2000 if large else 100
    row_count = count//columns
    steps = 5000 if large else 100
    lg = work/'cells.lg'; opt = work/'cells.opt'
    lg.write_text(f'Alpha 1\nBeta 1\nDieSize 0 0 {columns*2} {row_count}\n'+
                  ''.join(f'c{i} {2*(i%columns)} {i//columns} 1 1 NOTFIX\n' for i in range(count))+
                  ''.join(f'PlacementRows 0 {y} 1 1 {columns*2}\n' for y in range(row_count)))
    opt.write_text(''.join(f'Banking_Cell: c{i} --> m{i} {2*(i%columns)} {i//columns} 2 1\n' for i in range(steps)))
    text, record = execute('Lab03', 'Legalizer', [lg,opt], ['--strategy','first-fit'], dict(cells=count, banking_steps=steps))
    occupied = {(2*(i%columns),i//columns) for i in range(count)}
    lines = text.splitlines(); assert len(lines) == 2*steps
    for i in range(steps):
        occupied.remove((2*(i%columns),i//columns))
        x,y = map(float,lines[2*i].split())
        assert x == int(x) and y == int(y) and lines[2*i+1] == '0'
        x,y = int(x),int(y)
        assert 0 <= x < columns*2-1 and 0 <= y < row_count
        assert (x,y) not in occupied and (x+1,y) not in occupied
        occupied.update([(x,y),(x+1,y)])
    record['valid'] = True

    cols, rows_count = (1000,600) if large else (40,30)
    n = cols*rows_count
    sources = [(0,0),(cols-1,0)]; targets = [(cols-1,rows_count-1),(0,rows_count-1)]
    gmp = work/'grid.gmp'; gcl = work/'grid.gcl'; cst = work/'grid.cst'
    gmp.write_text(f'.ra\n11 17 {2*cols} {3*rows_count}\n.g\n2 3\n'+''.join(
        f'.c\n0 0 {2*cols} {3*rows_count}\n.b\n'+''.join(f'{i+1} {2*x} {3*y}\n' for i,(x,y) in enumerate(points))
        for points in [sources,targets]))
    gcl.write_text('.ec\n'+'100 100\n'*n)
    cst.write_text('.alpha 1\n.beta 1\n.gamma 1\n.delta 1\n.v\n3\n.l\n'+'1 '*n+'\n.l\n'+'1 '*n+'\n')
    text, record = execute('Lab04','D2DGRter',[gmp,gcl,cst],['--router','layered','--stats'],dict(cols=cols, rows=rows_count, nets=2))
    sections = text.split('.end\n'); assert sections[-1] == '' and len(sections) == 3
    objectives = []
    for i,section in enumerate(sections[:-1]):
        lines = section.splitlines(); assert lines[0] == f'n{i+1}'
        at = (11+2*sources[i][0],17+3*sources[i][1]); layer = 1; vias = steps_count = wire = 0
        for line in lines[1:]:
            if line == 'via': layer = 3-layer; vias += 1; continue
            tag,x,y,X,Y = line.split(); x,y,X,Y = map(int,[x,y,X,Y])
            assert tag == f'M{layer}' and (x,y) == at
            assert x == X if layer == 1 else y == Y
            for xx,yy in [(x,y),(X,Y)]:
                assert 11 <= xx < 11+2*cols and 17 <= yy < 17+3*rows_count
                assert (xx-11)%2 == 0 and (yy-17)%3 == 0
            wire += abs(X-x)+abs(Y-y); steps_count += abs(X-x)//2+abs(Y-y)//3
            at = X,Y
        assert at == (11+2*targets[i][0],17+3*targets[i][1]) and layer == 1
        cost = wire + steps_count+1 + 3*vias
        expected = 2*(cols-1)+3*(rows_count-1)+(cols-1)+(rows_count-1)+1+6
        assert cost == expected
        objectives.append(cost)
    stats = [json.loads(line) for line in (work/'Lab04/solver.log').read_text().splitlines()]
    assert [s['incremental_cost'] for s in stats] == objectives
    record.update(valid=True, objective=sum(objectives))
    report = dict(scale=args.scale, build=build_metadata(args.bin_root), jobs=1, work_dir=str(work),
                  oracle='Known corner-stitch grid; pairwise floorplan overlap + HPWL; per-step occupied sites; analytic shortest routing costs.', results=rows)
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2)+'\n')
    for row in rows: print(row['lab'], row['dimensions'], row['valid'], row['wall_seconds'])


if __name__ == '__main__': main()
