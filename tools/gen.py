#!/usr/bin/env python3
"""Instance generator for ParcalaR.
Usage:
  gen.py grid OUTDIR SEED            -> all n in 100..200 step 10, pct in 20..90 step 10, files OUTDIR/{n}{pct}.txt
  gen.py one  OUTFILE N PCT SEED     -> single G(n, M) instance, M = round(pct/100 * n(n-1)/2)
Format: first line "N M", then N lines of N space-separated 0/1 (symmetric, zero diagonal)."""
import random, sys, os

def gen(n, pct, seed):
    rng = random.Random(seed * 1000003 + n * 1009 + int(round(pct * 10)))
    pairs = [(i, j) for i in range(n) for j in range(i + 1, n)]
    m = int(round(pct / 100.0 * len(pairs)))
    E = rng.sample(pairs, m)
    A = [[0] * n for _ in range(n)]
    for i, j in E:
        A[i][j] = A[j][i] = 1
    return n, m, A

def write(path, n, m, A):
    with open(path, 'w') as f:
        f.write(f"{n} {m}\n")
        for row in A:
            f.write(" ".join(map(str, row)) + "\n")

if __name__ == '__main__':
    mode = sys.argv[1]
    if mode == 'grid':
        out, seed = sys.argv[2], int(sys.argv[3])
        os.makedirs(out, exist_ok=True)
        for n in range(100, 201, 10):
            for pct in range(20, 91, 10):
                write(os.path.join(out, f"{n}{pct}.txt"), *gen(n, pct, seed))
    elif mode == 'one':
        path, n, pct, seed = sys.argv[2], int(sys.argv[3]), float(sys.argv[4]), int(sys.argv[5])
        write(path, *gen(n, pct, seed))
