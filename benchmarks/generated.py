"""Seeded, self-authored workloads; generated files stay in the experiment directory."""
import json
from pathlib import Path
import random


def legalization(directory: Path, density: int, height: int, seed: int):
    rng = random.Random(seed)
    name = f'placement-{density}-h{height}-{seed}'
    directory.mkdir(parents=True, exist_ok=True)
    paths = [directory/(name+'.lg'), directory/(name+'.opt')]
    lines = ['Alpha 5', 'Beta 1', 'DieSize 0 0 48 12']
    movable = []
    for y in range(0, 12, height):
        for x in range(48):
            if rng.randrange(100) >= density:
                continue
            fixed = rng.randrange(4) == 0
            cell = ('C' if fixed else 'FF_')+f'{x}_{y}'
            lines.append(f'{cell} {x} {y} 1 {height} '+('FIX' if fixed else 'NOTFIX'))
            if not fixed:
                movable.append(cell)
    lines += [f'PlacementRows 0 {y} 1 1 48' for y in range(12)]
    paths[0].write_text('\n'.join(lines)+'\n')
    steps = []
    for i in range(80):
        old = movable.pop(rng.randrange(len(movable)))
        new = f'FF_new{i}'
        x, y = rng.randrange(48), rng.randrange(12)
        steps.append(f'Banking_Cell: {old} --> {new} {x} {y} 1 {height}')
        movable.append(new)
    paths[1].write_text('\n'.join(steps)+'\n')
    return name, paths


def floorplan(directory: Path, count: int, seed: int):
    rng = random.Random(seed)
    rectangles = [(0, 0, 80, 80)]
    while len(rectangles) < count:
        index = max(range(len(rectangles)), key=lambda i: rectangles[i][2]*rectangles[i][3])
        x, y, w, h = rectangles.pop(index)
        vertical = w > 1 and (h == 1 or (w >= h if rng.randrange(2) else w > h/2))
        size = w if vertical else h
        split = rng.randint(max(1, size//3), min(size-1, 2*size//3))
        rectangles.extend([(x,y,split,h),(x+split,y,w-split,h)] if vertical else
                          [(x,y,w,split),(x,y+split,w,h-split)])
    name = f'generated-{count}-{seed}'
    directory.mkdir(parents=True, exist_ok=True)
    blocks, nets = directory/(name+'.block'), directory/(name+'.nets')
    blocks.write_text(f'Outline: 96 96\nNumBlocks: {count}\nNumTerminals: 4\n'+
                      ''.join(f'b{i} {w} {h}\n' for i,(_,_,w,h) in enumerate(rectangles))+
                      't0 terminal 0 0\nt1 terminal 96 0\nt2 terminal 0 96\nt3 terminal 96 96\n')
    names = [f'b{i}' for i in range(count)]+[f't{i}' for i in range(4)]
    net_count = min(499, 3*count)
    lines = []
    for _ in range(net_count):
        pins = rng.sample(names, min(len(names), rng.choice([2,4,8,12])))
        lines.append(f'NetDegree: {len(pins)}\n'+'\n'.join(pins)+'\n')
    nets.write_text(f'NumNets: {net_count}\n'+''.join(lines))
    (directory/(name+'.known-layout.json')).write_text(json.dumps(
        dict(seed=seed, outline=[96,96], blocks=rectangles,
             note='Disjoint recursive partition of 80x80; dimensions are feasible within the 96x96 outline.'),indent=2)+'\n')
    return name, [blocks,nets]


def routing(directory: Path, seed: int, rows=24, cols=24, nets=40):
    rng = random.Random(seed)
    name = f'congestion-{rows}x{cols}-{nets}-{seed}'
    directory.mkdir(parents=True, exist_ok=True)
    paths = [directory/(name+'.'+extension) for extension in ['gmp','gcl','cst']]
    width = cols//4
    possible = [(x,y) for y in range(rows) for x in range(width)]
    sources = rng.sample(possible,nets)
    targets = rng.sample(possible,nets)
    def chip(x, points):
        return f'.c\n{x} 0 {width} {rows}\n.b\n'+''.join(
            f'{i+1} {a} {b}\n' for i,(a,b) in enumerate(points))+'\n'
    paths[0].write_text(f'.ra\n0 0 {cols} {rows}\n.g\n1 1\n'+chip(0,sources)+chip(cols-width,targets))
    capacities = []
    for y in range(rows):
        for x in range(cols):
            # A limited central cut creates competition between the two die regions.
            left = (0 if y % 3 == 0 else 2) if x == cols//2 else rng.choice([1,2,3])
            capacities.append((left,rng.choice([1,2,3])))
    paths[1].write_text('.ec\n'+''.join(f'{a} {b}\n' for a,b in capacities))
    layers = [[rng.randint(1,8)/2 for _ in range(rows*cols)] for _ in range(2)]
    paths[2].write_text('.alpha 1\n.beta 30\n.gamma .2\n.delta 1\n.v\n1\n'+
                        ''.join('.l\n'+''.join(' '.join(map(str,layer[y*cols:(y+1)*cols]))+'\n' for y in range(rows)) for layer in layers))
    return name, paths
