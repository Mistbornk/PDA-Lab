#!/usr/bin/env python3
"""Independent four-state (entry/exit x layer) Dijkstra oracle for the layered router."""
import argparse
import heapq
import json
import math
from pathlib import Path
import random
import subprocess
import tempfile
import unittest

ARGS = None


def cell_cost(costs, weights, cell, arrival, departure):
    if arrival == departure:
        return weights[2] * costs[arrival][cell]
    return weights[2] * sum(layer[cell] for layer in costs)/2 + weights[3]*weights[4]


def edge_key(a, b):
    return tuple(sorted((a, b)))


def neighbors(cell, rows, cols):
    y, x = divmod(cell, cols)
    for dy, dx in [(1, 0), (-1, 0), (0, 1), (0, -1)]:
        if 0 <= y+dy < rows and 0 <= x+dx < cols:
            yield (y+dy)*cols+x+dx, int(dx != 0)


def oracle(source, target, rows, cols, costs, weights, capacity, usage, pitch):
    # Build an explicit four-state graph, separate from the C++ two-state A*.
    graph = {}
    for cell in range(rows*cols):
        for layer in range(2):
            entry = (cell, 0, layer)
            graph[entry] = [((cell, 1, out), cell_cost(costs, weights, cell, layer, out))
                            for out in range(2)]
            graph[(cell, 1, layer)] = []
            for other, direction in neighbors(cell, rows, cols):
                if direction != layer:
                    continue
                key = edge_key(cell, other)
                # Direct difference in aggregate overflow; includes newly added net.
                before = max(0, usage.get(key, 0)-capacity[key])
                after = max(0, usage.get(key, 0)+1-capacity[key])
                extra = (after-before)*max(map(max, costs))/2
                cost = weights[0]*pitch[layer] + weights[1]*extra
                graph[(cell, 1, layer)].append(((other, 0, layer), cost))
    start, goal = (source, 0, 0), (target, 1, 0)
    queue = [(0, start)]; distance = {start: 0}
    while queue:
        value, node = heapq.heappop(queue)
        if value != distance[node]:
            continue
        if node == goal:
            return value
        for successor, weight in graph[node]:
            candidate = value + weight
            if candidate < distance.get(successor, math.inf):
                distance[successor] = candidate
                heapq.heappush(queue, (candidate, successor))
    raise AssertionError('Finite grid must be connected')


def parse_paths(text, sources, targets, rows, cols, pitch, origin):
    paths = []
    lines = iter(text.splitlines())
    for net, (source, target) in enumerate(zip(sources, targets), 1):
        assert next(lines) == f'n{net}'
        at = source; layer = 0; path = [(at, layer)]; vias = []
        for line in lines:
            if line == '.end':
                break
            if line == 'via':
                vias.append(at); layer = 1-layer
                continue
            metal, *xy = line.split()
            x, y, X, Y = map(int, xy)
            assert metal == f'M{layer+1}'
            assert (x == X if layer == 0 else y == Y)
            def position(xx, yy):
                assert (xx-origin[0]) % pitch[1] == (yy-origin[1]) % pitch[0] == 0
                row, col = (yy-origin[1])//pitch[0], (xx-origin[0])//pitch[1]
                assert 0 <= row < rows and 0 <= col < cols
                return row*cols+col
            assert position(x, y) == at
            end = position(X, Y)
            step = (cols if layer == 0 else 1) * (1 if end > at else -1)
            while at != end:
                at += step; path.append((at, layer))
        else:
            raise AssertionError('Missing .end')
        assert at == target and layer == 0
        expected_vias = [cell for (cell, arrival), (_, departure) in zip(path, path[1:]) if arrival != departure]
        if path[-1][1] != 0:
            expected_vias.append(target)
        assert vias == expected_vias
        paths.append(path)
    assert next(lines, None) is None
    return paths


class LayeredOracle(unittest.TestCase):
    def test_generated_cost_and_legality(self):
        with tempfile.TemporaryDirectory(prefix='pda-layered-') as directory:
            work = Path(directory)
            for seed in range(80):
                with self.subTest(seed=seed):
                    rng = random.Random(seed)
                    rows, cols = rng.randint(1, 5), rng.randint(1, 5)
                    pitch = (rng.randint(1, 9), rng.randint(1, 9))  # vertical, horizontal
                    origin = (11, 23); count = rows*cols
                    if seed == 79:  # Distances larger than signed int, without a huge allocation.
                        rows = cols = 3; count = 9; pitch = (700000000, 700000000); origin = (0, 0)
                    sources = [rng.randrange(count) for _ in range(5)]
                    targets = [rng.randrange(count) for _ in range(5)]
                    targets[0] = sources[0]  # same-cell path must charge exactly one M1 cell
                    costs = [[rng.randint(0, 12)/2 for _ in range(count)] for _ in range(2)]
                    weights = [rng.choice([0, .15, .7, 2]) for _ in range(4)] + [rng.randint(0, 7)]
                    capacities = [[rng.randrange(3), rng.randrange(3)] for _ in range(count)]
                    capacity = {}
                    for cell in range(count):
                        for other, layer in neighbors(cell, rows, cols):
                            capacity[edge_key(cell, other)] = capacities[max(cell, other)][1-layer]
                    def bumps(values):
                        return ''.join(f'{i+1} {(cell%cols)*pitch[1]} {(cell//cols)*pitch[0]}\n'
                                       for i, cell in enumerate(values))
                    chip = f'.c\n0 0 {cols*pitch[1]} {rows*pitch[0]}\n.b\n'
                    (work/'case.gmp').write_text(f'.ra\n{origin[0]} {origin[1]} {cols*pitch[1]} {rows*pitch[0]}\n.g\n{pitch[1]} {pitch[0]}\n'+chip+bumps(sources)+chip+bumps(targets))
                    (work/'case.gcl').write_text('.ec\n'+''.join(f'{a} {b}\n' for a, b in capacities))
                    (work/'case.cst').write_text(''.join(f'.{name} {value}\n' for name, value in zip(['alpha', 'beta', 'gamma', 'delta'], weights))+
                                                 f'.v\n{weights[4]}\n'+''.join('.l\n'+' '.join(map(str, layer))+'\n' for layer in costs))
                    result = subprocess.run([str(ARGS.bin_root/'Lab04/D2DGRter'), *[str(work/f'case.{ext}') for ext in ['gmp', 'gcl', 'cst', 'lg']], '--router', 'layered', '--stats'],
                                            capture_output=True, text=True, timeout=20)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    self.assertNotIn('runtime error:', result.stderr)
                    self.assertNotIn('AddressSanitizer', result.stderr)
                    stats = [json.loads(line) for line in result.stderr.splitlines()]
                    paths = parse_paths((work/'case.lg').read_text(), sources, targets, rows, cols, pitch, origin)
                    self.assertEqual(len(stats), len(paths))
                    usage = {}
                    for i, path in enumerate(paths):
                        expected = oracle(sources[i], targets[i], rows, cols, costs, weights, capacity, usage, pitch)
                        measured = 0
                        for (a, arrival), (b, departure) in zip(path, path[1:]):
                            measured += cell_cost(costs, weights, a, arrival, departure)
                            measured += weights[0]*pitch[departure]
                            key = edge_key(a, b)
                            if usage.get(key, 0) >= capacity[key]:
                                measured += weights[1]*max(map(max, costs))/2
                        measured += cell_cost(costs, weights, targets[i], path[-1][1], 0)
                        self.assertTrue(math.isclose(measured, expected, rel_tol=1e-10, abs_tol=1e-8), (seed, i, measured, expected))
                        self.assertTrue(math.isclose(stats[i]['incremental_cost'], expected, rel_tol=1e-10, abs_tol=1e-8))
                        for (a, _), (b, _) in zip(path, path[1:]):
                            key = edge_key(a, b); usage[key] = usage.get(key, 0)+1


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--bin-root', type=Path, required=True)
    ARGS, rest = parser.parse_known_args(); ARGS.bin_root = ARGS.bin_root.resolve()
    unittest.main(argv=[__file__, *rest])
