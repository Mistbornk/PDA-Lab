#!/usr/bin/env python3
"""Build a small, offline, independently checked floorplan/routing demonstration."""
import argparse
import colorsys
import hashlib
import html
import json
import math
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'benchmarks'))
sys.path.insert(0, str(ROOT/'tests'))
from generated import floorplan, routing
from lab4_oracle import parse_paths, edge_key, cell_cost


def check(condition, message):
    if not condition:
        raise ValueError(message)


def execute(command, log):
    result = subprocess.run(list(map(str, command)), capture_output=True, text=True, timeout=60)
    log.write_text(result.stdout+result.stderr)
    check(result.returncode == 0, f'Solver failed: {result.stderr}')
    return [json.loads(line) for line in result.stderr.splitlines() if line.startswith('{')]


def svg(body, title):
    return ('<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 600 600" role="img">'
            f'<title>{html.escape(title)}</title><rect width="600" height="600" fill="#f4f7fb"/>'
            + ''.join(body) + '</svg>\n')


def color(hue, lightness, saturation):
    return '#'+''.join(f'{round(channel*255):02x}' for channel in
                       colorsys.hls_to_rgb(hue/360,lightness,saturation))


def floorplan_view(inputs, report):
    lines = inputs[0].read_text().splitlines()
    width, height = map(int, lines[0].split()[1:])
    count = int(lines[1].split()[1])
    dimensions = {p[0]:tuple(map(int,p[1:])) for p in map(str.split,lines[3:3+count])}
    terminals = {p[0]:tuple(map(int,p[2:])) for p in map(str.split,lines[3+count:])}
    lines = report.read_text().splitlines()
    boxes = {p[0]:tuple(map(int,p[1:])) for p in map(str.split,lines[5:])}
    check(len(lines[5:]) == count and boxes.keys() == dimensions.keys(), 'Missing or duplicate macros')
    for name,(x,y,X,Y) in boxes.items():
        check(0 <= x < X <= width and 0 <= y < Y <= height, 'Macro outside outline')
        check(sorted((X-x,Y-y)) == sorted(dimensions[name]), 'Wrong macro dimensions')
    values = list(boxes.values())
    for i,(x,y,X,Y) in enumerate(values):
        for a,b,A,B in values[:i]:
            check(not (x < A and a < X and y < B and b < Y), 'Overlapping macros')
    centers = {n:(x+(X-x)//2,y+(Y-y)//2) for n,(x,y,X,Y) in boxes.items()}
    centers.update(terminals)
    tokens = iter(inputs[1].read_text().split())
    check(next(tokens) == 'NumNets:', 'Bad generated nets')
    nets = []
    for _ in range(int(next(tokens))):
        check(next(tokens) == 'NetDegree:', 'Bad net header')
        nets.append([centers[next(tokens)] for _ in range(int(next(tokens)))])
    check(next(tokens,None) is None, 'Trailing nets')
    hpwl = sum(max(x for x,y in net)-min(x for x,y in net)+max(y for x,y in net)-min(y for x,y in net) for net in nets)
    bounds = (max(X for x,y,X,Y in values),max(Y for x,y,X,Y in values))
    area = bounds[0]*bounds[1]
    objective = int(.5*area+.5*hpwl)
    check([int(s) for s in lines[:3]] == [objective,hpwl,area], 'Floorplan metrics differ from report')
    check(tuple(map(int,lines[3].split())) == bounds, 'Wrong floorplan bounding box')
    scale = 540/max(width,height)
    def xy(x,y): return 30+x*scale,570-y*scale
    body = [f'<rect x="30" y="{570-height*scale}" width="{width*scale}" height="{height*scale}" fill="white" stroke="#334155"/>']
    for i, net in enumerate(nets):
        x,y = min(x for x,y in net), max(y for x,y in net)
        X,Y = max(x for x,y in net), min(y for x,y in net)
        a,b = xy(x,y)
        body.append(f'<rect class="net-box" x="{a}" y="{b}" width="{(X-x)*scale}" height="{(y-Y)*scale}" fill="none" stroke="#cbd5e1" stroke-dasharray="2 3"><title>Net {i}: HPWL bounding box</title></rect>')
    for i,(name,(x,y,X,Y)) in enumerate(boxes.items()):
        a,b = xy(x,Y)
        fill = color((i*47)%360,.78,.48)
        body.append(f'<rect x="{a}" y="{b}" width="{(X-x)*scale}" height="{(Y-y)*scale}" fill="{fill}" stroke="#334155"><title>{html.escape(name)}: ({x},{y})–({X},{Y})</title></rect>')
        body.append(f'<text x="{a+(X-x)*scale/2}" y="{b+(Y-y)*scale/2+4}" text-anchor="middle" font-family="sans-serif" font-size="12">{html.escape(name)}</text>')
    for name,(x,y) in terminals.items():
        a,b = xy(x,y)
        body.append(f'<circle cx="{a}" cy="{b}" r="4" fill="#0f172a"><title>{name}</title></circle>')
    return svg(body,'Verified fixed-outline floorplan'),dict(objective=objective,hpwl=hpwl,area=area,macros=count,nets=len(nets))


def routing_view(inputs, report, summary):
    # Parse the self-generated unit-grid format, rather than regenerate a solution.
    gmp = inputs[0].read_text().splitlines()
    cols, rows = map(int,gmp[1].split()[2:])
    chips = []
    for segment in inputs[0].read_text().split('.c\n')[1:]:
        header, pins = segment.split('.b\n')
        cx,cy,_,_ = map(int,header.split())
        chips.append([(cy+y)*cols+cx+x for _,x,y in (map(int,s.split()) for s in pins.splitlines() if s.strip())])
    paths = parse_paths(report.read_text(),*chips,rows,cols,(1,1),(0,0))
    capacities = [tuple(map(int,line.split())) for line in inputs[1].read_text().splitlines()[1:]]
    head,*layers = inputs[2].read_text().split('.l\n')
    weights = []
    fields = head.split()
    for key in ['.alpha','.beta','.gamma','.delta','.v']:
        weights.append(float(fields[fields.index(key)+1]))
    costs = [list(map(float,layer.split())) for layer in layers]
    usage = {}; via_count = 0; wirelength = 0; cost = 0
    for path in paths:
        for (a,arrival),(b,departure) in zip(path,path[1:]):
            cost += cell_cost(costs,weights,a,arrival,departure)+weights[0]
            wirelength += 1; via_count += arrival != departure
            key = edge_key(a,b); usage[key] = usage.get(key,0)+1
        at,layer = path[-1]
        via_count += layer != 0
        cost += cell_cost(costs,weights,at,layer,0)
    capacity = {key:capacities[max(key)][int(abs(key[1]-key[0])==cols)] for key in usage}
    overflow = sum(max(0,value-capacity[key]) for key,value in usage.items())
    max_overflow = max((max(0,value-capacity[key]) for key,value in usage.items()),default=0)
    cost += weights[1]*max(map(max,costs))/2*overflow
    metrics = dict(objective=cost,wirelength=wirelength,vias=via_count,overflow=overflow,max_overflow=max_overflow)
    for key,value in metrics.items():
        check(math.isclose(value,summary[key],rel_tol=1e-10,abs_tol=1e-8), f'Routing {key} differs from trace')
    scale = 540/max(cols-1,rows-1)
    def xy(cell): return 30+(cell%cols)*scale,570-(cell//cols)*scale
    body = []
    for cell in range(rows*cols):
        x,y = xy(cell)
        body.append(f'<circle cx="{x}" cy="{y}" r="1.8" fill="#94a3b8"/>')
    for i,path in enumerate(paths):
        points = ' '.join(f'{x},{y}' for x,y in (xy(cell) for cell,_ in path))
        body.append(f'<g class="route" data-net="{i+1}"><polyline points="{points}" fill="none" stroke="{color(i*47%360,.4,.65)}" stroke-width="2" opacity=".7"><title>Net {i+1}</title></polyline></g>')
    body.append('<g id="congestion">')
    for key,value in usage.items():
        if value > capacity[key]:
            x,y = xy(key[0]); X,Y = xy(key[1])
            body.append(f'<line x1="{x}" y1="{y}" x2="{X}" y2="{Y}" stroke="#ef4444" stroke-width="6" opacity=".6"><title>Usage {value}, capacity {capacity[key]}</title></line>')
    body.append('</g>')
    for i,(source,target) in enumerate(zip(*chips),1):
        for cell in [source,target]:
            x,y = xy(cell)
            body.append(f'<circle cx="{x}" cy="{y}" r="4" fill="#0f172a"><title>Net {i} endpoint</title></circle>')
    metrics['nets'] = len(paths)
    return svg(body,'Verified two-layer routing and overflowing edges'),metrics


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bin-root',type=Path,default=ROOT/'build')
    parser.add_argument('--output',type=Path,default=ROOT/'build/demo')
    args = parser.parse_args()
    args.bin_root = args.bin_root.resolve(); args.output = args.output.resolve()
    args.output.mkdir(parents=True,exist_ok=True)
    _,fp = floorplan(args.output/'inputs',12,701)
    _,rt = routing(args.output/'inputs',101,rows=12,cols=12,nets=24)
    fexe,rexe = args.bin_root/'Lab02/Lab2',args.bin_root/'Lab04/D2DGRter'
    fflags = ['--seed','41','--iterations','100000','--policy','feasibility']
    rflags = ['--router','negotiated','--history','0','--rounds','10','--stats']
    execute([fexe,.5,*fp,args.output/'floorplan.rpt',*fflags],args.output/'floorplan.log')
    records = execute([rexe,*rt,args.output/'routing.lg',*rflags],args.output/'routing.log')
    fsvg,fmetrics = floorplan_view(fp,args.output/'floorplan.rpt')
    rsvg,rmetrics = routing_view(rt,args.output/'routing.lg',records[-1])
    (args.output/'floorplan.svg').write_text(fsvg)
    (args.output/'routing.svg').write_text(rsvg)
    metrics = dict(schema_version=1, floorplan=fmetrics,routing=rmetrics,parameters=dict(floorplan=fflags,routing=rflags),
                   validation='Independent report geometry/HPWL and emitted-path aggregate-cost checks; no downloaded evaluator required.',
                   executable_sha256={str(p.relative_to(args.bin_root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [fexe,rexe]},
                   input_sha256={str(p.relative_to(args.output)):hashlib.sha256(p.read_bytes()).hexdigest() for p in fp+rt})
    (args.output/'metrics.json').write_text(json.dumps(metrics,indent=2)+'\n')
    page = '''<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>PDA Lab — verified offline demo</title><style>body{font:16px system-ui;background:#f8fafc;color:#172033;max-width:1150px;margin:32px auto;padding:0 20px}h1{font-size:28px}main{display:grid;grid-template-columns:repeat(auto-fit,minmax(300px,1fr));gap:24px}section{background:white;padding:20px;border:1px solid #dce3ed;border-radius:12px}svg{width:100%;height:auto}select{font:inherit}small{color:#475569}code{word-break:break-all}</style>
<h1>Physical Design Automation: verified solver outputs</h1><p>Two independent course problems. Self-authored generated inputs; fixed solver seeds/work limits. Figures are parsed from emitted reports.</p><main>
<section><h2>Fixed-outline floorplanning</h2>FPSTATS<p>Colored rectangles: macros. Dotted boxes: net bounding boxes. Black dots: terminals.</p>FPSVG</section>
<section><h2>Two-layer routing</h2>RTSTATS<p><label>Route <select id="net"><option value="all">All nets</option>OPTIONS</select></label> <label><input id="overflow" type="checkbox" checked> Overflow</label></p>RTSVG<p>Red edges exceed capacity. Capacity overflow contributes to the original objective; it is not a connectivity failure.</p></section></main>
<p><a href="metrics.json">Verified metrics and hashes</a> · <a href="floorplan.svg">Floorplan SVG</a> · <a href="routing.svg">Routing SVG</a></p><small>Original course context: NYCU Physical Design Automation. This demo does not implement a complete industrial place-and-route flow.</small>
<script>document.querySelector('#net').onchange=e=>document.querySelectorAll('.route').forEach(g=>g.style.opacity=e.target.value==='all'||g.dataset.net===e.target.value?'1':'.06');document.querySelector('#overflow').onchange=e=>document.querySelector('#congestion').style.display=e.target.checked?'':'none';</script></html>'''
    page = page.replace('FPSTATS',f'<p>Cost {fmetrics["objective"]:,} · HPWL {fmetrics["hpwl"]:,} · Area {fmetrics["area"]:,}</p>')
    page = page.replace('RTSTATS',f'<p>Cost {rmetrics["objective"]:.2f} · Length {rmetrics["wirelength"]} · Vias {rmetrics["vias"]} · Overflow {rmetrics["overflow"]}</p>')
    page = page.replace('FPSVG',fsvg).replace('RTSVG',rsvg).replace('OPTIONS',''.join(f'<option>{i}</option>' for i in range(1,rmetrics['nets']+1)))
    (args.output/'index.html').write_text(page)
    print(json.dumps(dict(output=str(args.output/'index.html'),floorplan=fmetrics,routing=rmetrics)))


if __name__ == '__main__':
    main()
