#!/usr/bin/env python3
"""Independent small/reference checks. No downloaded binary is required."""
import argparse
import math
from pathlib import Path
import random
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
ARGS = None


def tiles(width, height, blocks):
    """Small raster oracle for maximal horizontal space tiles, independent of stitches."""
    result = list(blocks)
    active = {}
    for y in range(height):
        occupied = {x for _, bx, by, w, h in blocks if by <= y < by+h for x in range(bx, bx+w)}
        runs = []
        x = 0
        while x < width:
            if x in occupied:
                x += 1
                continue
            start = x
            while x < width and x not in occupied:
                x += 1
            runs.append((start, x-start))
        following = {}
        for run in runs:
            following[run] = active.pop(run, [0, run[0], y, run[1], 0])
            following[run][4] += 1
        result.extend(active.values())
        active = following
    result.extend(active.values())
    return result


def adjacent(a, b):
    _, x, y, w, h = a
    _, X, Y, W, H = b
    return ((x+w == X or X+W == x) and max(y,Y) < min(y+h,Y+H) or
            (y+h == Y or Y+H == y) and max(x,X) < min(x+w,X+W))


class Regression(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='pda-test-')
        self.work = Path(self.temp.name)
        self.addCleanup(self.temp.cleanup)

    def file(self, name, text):
        path = self.work / name
        path.write_text(text)
        return path

    def run_solver(self, lab, args, ok=True):
        names = {'Lab01':'Lab1','Lab02':'Lab2','Lab03':'Legalizer','Lab04':'D2DGRter'}
        command = [str(ARGS.bin_root/lab/names[lab]), *map(str,args)]
        result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                text=True, timeout=20, cwd=self.work)
        if ok:
            self.assertEqual(result.returncode, 0, result.stderr)
        else:
            self.assertGreater(result.returncode, 0, 'Must reject cleanly, not crash: '+result.stderr)
        self.assertNotIn('runtime error:', result.stderr)
        self.assertNotIn('AddressSanitizer', result.stderr)
        return result

    def test_cli_and_missing_files(self):
        for lab in ['Lab01','Lab02','Lab03','Lab04']:
            with self.subTest(lab=lab):
                self.run_solver(lab, [], False)
                arguments = {'Lab01':['missing','out'], 'Lab02':['0.5','missing','missing','out'],
                             'Lab03':['missing','missing','out'], 'Lab04':['missing','missing','missing','out']}
                self.run_solver(lab, arguments[lab], False)

    def test_lab1_official(self):
        cases = sorted((ROOT/'Lab01/tests/data').glob('case*.txt'))
        self.assertEqual(len(cases), 4, 'The four bundled Lab01 regressions must be present')
        for case in cases:
            out = self.work/'out.txt'
            self.run_solver('Lab01',[case,out])
            expected = ROOT/'Lab01/tests/expected'/case.name.replace('case','output')
            self.assertEqual(out.read_text().split(),expected.read_text().split())

    def test_lab1_raster_oracle(self):
        for seed in range(20):
            rng = random.Random(seed)
            width,height=12,10
            blocks=[];commands=[f'{width} {height}'];points=[]
            for i in range(25):
                x,y=rng.randrange(width),rng.randrange(height)
                containing=next(t for t in tiles(width,height,blocks) if t[1]<=x<t[1]+t[3] and t[2]<=y<t[2]+t[4])
                commands.append(f'P {x} {y}');points.append(containing[1:3])
                x,y=rng.randrange(width),rng.randrange(height)
                w,h=rng.randint(1,min(3,width-x)),rng.randint(1,min(3,height-y))
                if any(max(x,b[1])<min(x+w,b[1]+b[3]) and max(y,b[2])<min(y+h,b[2]+b[4]) for b in blocks):
                    continue
                b=[(i+1)*7,x,y,w,h];blocks.append(b);commands.append(' '.join(map(str,b)))
            final=tiles(width,height,blocks)
            expected=[str(len(final))]
            for b in sorted(blocks):
                neighbors=[t for t in final if adjacent(b,t)]
                expected.append(f'{b[0]} {sum(t[0]>0 for t in neighbors)} {sum(t[0]==0 for t in neighbors)}')
            expected.extend(' '.join(map(str,p)) for p in points)
            out=self.work/'out.txt'
            self.run_solver('Lab01',[self.file('input.txt','\n'.join(commands)+'\n'),out])
            self.assertEqual(out.read_text().split(),'\n'.join(expected).split(),f'seed {seed}')

    def test_lab1_bad_input(self):
        for text in ['', '0 10\n', '10 10\nP 10 0\n', '10 10\n1 0 0 1\n',
                     '10 10\n1 0 0 1 1\n1 2 2 1 1\n', '10 10\n1 1 1 3 3\n2 2 2 3 3\n']:
            self.run_solver('Lab01',[self.file('bad.txt',text),self.work/'out'],False)
        out=self.work/'empty.out'
        self.run_solver('Lab01',[self.file('empty.txt','10 10\n'),out])
        self.assertEqual(out.read_text(),'1\n')

    def test_lab2_single_macro_and_alpha(self):
        block=self.file('one.block','Outline: 10 10\nNumBlocks: 1\nNumTerminals: 1\nb 3 5\np terminal 10 0\n')
        nets=self.file('one.nets','NumNets: 1\nNetDegree: 2\nb\np\n')
        for alpha in [0,0.5,1]:
            outputs=[]
            for mode in ['ids','strings']:
                out=self.work/'floorplan.rpt'
                self.run_solver('Lab02',[alpha,block,nets,out,'--seed','7','--iterations','100','--hpwl',mode])
                lines=out.read_text().splitlines();tokens=lines[5].split()
                x,y,X,Y=map(int,tokens[1:]);self.assertEqual(sorted([X-x,Y-y]),[3,5])
                hpwl=abs(10-(x+(X-x)//2))+abs(y+(Y-y)//2)
                self.assertEqual(int(lines[1]),hpwl)
                self.assertEqual(int(lines[2]),X*Y)
                self.assertEqual(int(lines[0]),int(alpha*X*Y+(1-alpha)*hpwl))
                outputs.append(lines[:4]+lines[5:])
            self.assertEqual(*outputs)
        self.run_solver('Lab02',['nan',block,nets,self.work/'out'],False)
        self.run_solver('Lab02',[0.5,block,self.file('bad.nets','NumNets: 1\nNetDegree: 1\nunknown\n'),self.work/'out'],False)
        self.run_solver('Lab02',[0.5,self.file('bad.block','Outline: 10 10\nNumBlocks: 0\nNumTerminals: 0\n'),nets,self.work/'out'],False)

    def placement(self):
        return self.file('input.lg','Alpha 1\nBeta 1\nDieSize 10 20 30 24\n'
                         'a 10 20 2 1 NOTFIX\nb 14 20 2 1 NOTFIX\nwall 18 20 2 2 FIX\n'+
                         ''.join(f'PlacementRows 10 {y} 1 1 20\n' for y in range(20,24)))

    def test_lab2_tree_moves_and_large_area(self):
        sizes={'a':(3,5),'b':(5,7),'c':(4,4),'d':(3,1)}
        block=self.file('small.block','Outline: 50 50\nNumBlocks: 4\nNumTerminals: 1\n'+
                        ''.join(f'{name} {w} {h}\n' for name,(w,h) in sizes.items())+'p terminal -3 -5\n')
        nets=self.file('small.nets','NumNets: 2\nNetDegree: 3\na\nb\np\nNetDegree: 2\nc\nd\n')
        for alpha in [0,0.5,1]:
            solutions=[]
            for mode in ['ids','strings']:
                out=self.work/'small.rpt'
                self.run_solver('Lab02',[alpha,block,nets,out,'--seed','9','--iterations','1000','--hpwl',mode])
                lines=out.read_text().splitlines();placed={}
                for line in lines[5:]:
                    name,*values=line.split();self.assertNotIn(name,placed)
                    x,y,X,Y=map(int,values);self.assertEqual(sorted([X-x,Y-y]),sorted(sizes[name]))
                    self.assertTrue(0<=x<X<=50 and 0<=y<Y<=50);placed[name]=(x,y,X,Y)
                self.assertEqual(set(placed),set(sizes))
                for name,(x,y,X,Y) in placed.items():
                    self.assertFalse(any(other!=name and max(x,xx)<min(X,XX) and max(y,yy)<min(Y,YY)
                                         for other,(xx,yy,XX,YY) in placed.items()))
                pins={name:(x+(X-x)//2,y+(Y-y)//2) for name,(x,y,X,Y) in placed.items()};pins['p']=(-3,-5)
                hpwl=sum(max(pins[n][0] for n in net)-min(pins[n][0] for n in net)+
                         max(pins[n][1] for n in net)-min(pins[n][1] for n in net)
                         for net in [('a','b','p'),('c','d')])
                area=max(v[2] for v in placed.values())*max(v[3] for v in placed.values())
                self.assertEqual(list(map(int,lines[:3])),[int(alpha*area+(1-alpha)*hpwl),hpwl,area])
                solutions.append(lines[:4]+lines[5:])
            self.assertEqual(*solutions)
        block=self.file('large.block','Outline: 60000 60000\nNumBlocks: 1\nNumTerminals: 0\nb 50000 50000\n')
        nets=self.file('zero.nets','NumNets: 0\n');out=self.work/'large.rpt'
        self.run_solver('Lab02',[1,block,nets,out,'--iterations','2','--seed','1'])
        self.assertEqual(int(out.read_text().splitlines()[2]),2500000000)

    def test_lab3_fractional_row_and_dimensions(self):
        lg=self.file('fraction.lg','Alpha 0.5\nBeta 0.5\nDieSize 10 20.25 30 22.25\n'
                     'a 10 20.25 1.5 0.5 NOTFIX\n'+
                     ''.join(f'PlacementRows 10 {20.25+0.5*i} 1 0.5 20\n' for i in range(4)))
        opt=self.file('fraction.opt','Banking_Cell: a --> merged 10 20.25 2.5 0.5\n')
        out=self.work/'fraction.out';self.run_solver('Lab03',[lg,opt,out])
        self.assertEqual(out.read_text(),'10 20.25\n0\n')

    def test_lab3_strategies_and_append(self):
        lg=self.placement();opt=self.file('input.opt','Banking_Cell: a b --> joined 17 20 4 2\n')
        values=[]
        for search in ['point','interval']:
            out=self.file('post.lg','THIS MUST BE TRUNCATED\n')
            self.run_solver('Lab03',[lg,opt,out,'--strategy','first-fit','--search',search])
            self.assertEqual(out.read_text(),'10 20\n0\n');values.append(out.read_text())
        self.assertEqual(*values)
        # FIX follows its attribute, not name prefix; arbitrary cell names are supported.
        self.run_solver('Lab03',[lg,self.file('fixed.opt','Banking_Cell: wall --> joined 0 0 1 1\n'),self.work/'out'],False)
        self.run_solver('Lab03',[lg,self.file('broken.opt','Banking_Cell: a b\n'),self.work/'out'],False)
        self.run_solver('Lab03',[lg,self.file('unknown.opt','Banking_Cell: ghost --> joined 10 20 1 1\n'),self.work/'out'],False)
        self.run_solver('Lab03',[lg,self.file('huge.opt','Banking_Cell: a --> joined 10 20 30 1\n'),self.work/'out'],False)

    def test_lab3_random_interval_equivalence(self):
        for seed in range(10):
            rng=random.Random(seed)
            positions=rng.sample([(x,y) for x in range(20) for y in range(4)],25)
            text='Alpha 1\nBeta 1\nDieSize 0 0 20 4\n'
            text+=''.join(f'c{i} {x} {y} 1 1 NOTFIX\n' for i,(x,y) in enumerate(positions))
            text+=''.join(f'PlacementRows 0 {y} 1 1 20\n' for y in range(4))
            lg=self.file('random.lg',text)
            opt=self.file('random.opt',''.join(f'Banking_Cell: c{i} --> merged{i} {rng.randrange(20)} {rng.randrange(4)} 2 1\n' for i in range(10)))
            outputs=[]
            for mode in ['point','interval']:
                out=self.work/'result';self.run_solver('Lab03',[lg,opt,out,'--search',mode]);outputs.append(out.read_text())
            self.assertEqual(*outputs)
            live={f'c{i}':(x,y,1,1) for i,(x,y) in enumerate(positions)}
            rows=outputs[0].splitlines()
            for i in range(10):
                del live[f'c{i}'];x,y=map(float,rows[2*i].split());self.assertEqual(rows[2*i+1],'0')
                self.assertTrue(0<=x<=18 and 0<=y<=3 and x==int(x) and y==int(y))
                self.assertFalse(any(max(x,X)<min(x+2,X+w) and max(y,Y)<min(y+1,Y+h) for X,Y,w,h in live.values()))
                live[f'merged{i}']=(x,y,2,1)

    def route_input(self, target='0 0'):
        gmp=self.file('map.gmp',f'.ra\n10 20 20 20\n.g\n5 10\n.c\n0 0 20 20\n.b\n1 0 0\n.c\n0 0 20 20\n.b\n1 {target}\n')
        gcl=self.file('map.gcl','.ec\n'+'2 2\n'*8)
        cst=self.file('map.cst','.alpha 1\n.beta 1\n.gamma 1\n.delta 1\n.v\n1\n.l\n'+'1 '*8+'\n.l\n'+'1 '*8+'\n')
        return [gmp,gcl,cst]

    def test_lab4_single_cell_and_path(self):
        for target,end in [('0 0',(10,20)),('15 10',(25,30))]:
            out=self.work/'route.lg';self.run_solver('Lab04',[*self.route_input(target),out])
            lines=out.read_text().splitlines();self.assertEqual(lines[0],'n1');self.assertEqual(lines[-1],'.end')
            at=(10,20);layer=1
            for line in lines[1:-1]:
                if line=='via':layer=3-layer;continue
                tag,x,y,X,Y=line.split();x,y,X,Y=map(int,[x,y,X,Y]);self.assertEqual(tag,f'M{layer}')
                self.assertEqual((x,y),at)
                self.assertTrue(x==X if layer==1 else y==Y)
                for px,py in [(x,y),(X,Y)]:self.assertTrue(10<=px<30 and 20<=py<40 and (px-10)%5==0 and (py-20)%10==0)
                at=(X,Y)
            self.assertEqual(at,end);self.assertEqual(layer,1)
        inputs=self.route_input();inputs[1]=self.file('bad.gcl','.ec\n-1 0\n')
        self.run_solver('Lab04',[*inputs,self.work/'bad.out'],False)
        inputs=self.route_input();inputs[0]=self.file('bad.gmp','.ra\n0 0 10 10\n.g\n0 0\n')
        self.run_solver('Lab04',[*inputs,self.work/'bad.out'],False)
        # Manhattan distance can exceed INT_MAX even with a tiny 3x3 grid.
        gmp=self.file('large.gmp','.ra\n0 0 2100000000 2100000000\n.g\n700000000 700000000\n'
                      '.c\n0 0 2100000000 2100000000\n.b\n1 0 0\n'
                      '.c\n0 0 2100000000 2100000000\n.b\n1 1400000000 1400000000\n')
        gcl=self.file('large.gcl','.ec\n'+'2 2\n'*9)
        cst=self.file('large.cst','.alpha 1\n.beta 1\n.gamma 1\n.delta 1\n.v\n1\n.l\n'+
                      '1 '*9+'\n.l\n'+'1 '*9+'\n')
        self.run_solver('Lab04',[gmp,gcl,cst,self.work/'large.route'])


if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--bin-root',type=Path,required=True)
    ARGS,remaining=parser.parse_known_args();ARGS.bin_root=ARGS.bin_root.resolve()
    unittest.main(argv=[__file__,*remaining])
