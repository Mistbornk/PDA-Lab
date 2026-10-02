# Corner-stitching distribution study

Seventy-two serial runs cover dense coverage, narrow strips, shared boundaries and fragmented space at 64/256/1,024 insertions. Three timing repetitions compare indexed and full-scan stitch repair with the same spatial geometry index. All outputs pass an independent final-rectangle adjacency or spaced-grid oracle.

The full-coverage cases deliberately remove empty tiles; the fragmented case checks the known empty-space decomposition. Candidate counts and tile counts are deterministic. End-to-end time divided by insertion count includes parsing, point queries and reporting; it is not an isolated insertion timer.

| 1,024 insertions | Variant | Median wall s | Median RSS KiB | Stitch candidate visits |
|---|---|---:|---:|---:|
| dense | indexed | 0.0509 | 4144 | 564,955 |
| dense | scan | 0.1268 | 3992 | 11,089,680 |
| strips | indexed | 0.0344 | 4416 | 15,293 |
| strips | scan | 0.0752 | 3856 | 5,273,140 |
| shared-edges | indexed | 0.0573 | 4176 | 655,239 |
| shared-edges | scan | 0.1584 | 3924 | 16,391,316 |
| fragmented | indexed | 0.0425 | 4712 | 469,402 |
| fragmented | scan | 0.2056 | 4244 | 19,998,909 |

The indexed implementation reduces visits in these distributions, with some extra memory. Coordinate buckets still have a linear worst case; the finite sample is not an asymptotic guarantee. Subsecond timing is sensitive to process launch and host noise. No new arena or interval-tree rewrite is justified by these counts alone: allocation/cache profiling would be needed first.

[Raw records, inputs/executable hashes and all scales](../benchmarks/results/interview-lab1-distributions.json). Reproduce with:

```bash
python3 benchmarks/lab1_distributions.py --output benchmarks/work/distributions.json
```
