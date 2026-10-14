#!/usr/bin/env python3
"""Randomized correctness check of a solver binary against bin/brute.
Usage: check_small.py SOLVER [COUNT=300] [NMAX=20] [SEED=1]
Solver is invoked as: SOLVER FILE and must print a line containing Z=<int> and X=<int>.
Expected: Z equal to brute's Z, X equal to brute's Xmin (min X among optimal solutions)."""
import sys, os, random, subprocess, re, tempfile
here = os.path.dirname(os.path.abspath(__file__))
BRUTE = os.path.join(here, '..', 'bin', 'brute')

def write(path, n, edges):
    A = [[0]*n for _ in range(n)]
    for i, j in edges: A[i][j] = A[j][i] = 1
    with open(path, 'w') as f:
        f.write(f"{n} {len(edges)}\n")
        for r in A: f.write(" ".join(map(str, r)) + "\n")

def special(rng):
    n = rng.randint(1, 18); kind = rng.choice(['empty','complete','path','star','cycle','cliques','bip','wheel'])
    E = set()
    if kind == 'complete': E = {(i,j) for i in range(n) for j in range(i+1,n)}
    elif kind == 'path': E = {(i,i+1) for i in range(n-1)}
    elif kind == 'cycle': E = {(i,i+1) for i in range(n-1)} | ({(0,n-1)} if n > 2 else set())
    elif kind == 'star': E = {(0,i) for i in range(1,n)}
    elif kind == 'wheel': E = {(0,i) for i in range(1,n)} | {(i,i+1) for i in range(1,n-1)} | ({(1,n-1)} if n > 3 else set())
    elif kind == 'cliques':
        s = rng.randint(1, 5); grp = [i // s for i in range(n)]
        E = {(i,j) for i in range(n) for j in range(i+1,n) if grp[i]==grp[j]}
    elif kind == 'bip':
        a = n // 2; E = {(i,j) for i in range(a) for j in range(a,n) if rng.random() < 0.7}
    perm = list(range(n)); rng.shuffle(perm)
    return n, [tuple(sorted((perm[i], perm[j]))) for i, j in E], kind

def main():
    solver = sys.argv[1]; cnt = int(sys.argv[2]) if len(sys.argv) > 2 else 300
    nmax = int(sys.argv[3]) if len(sys.argv) > 3 else 20; seed = int(sys.argv[4]) if len(sys.argv) > 4 else 1
    rng = random.Random(seed); bad = 0
    tmp = tempfile.mkdtemp()
    for t in range(cnt):
        if t % 5 == 4:
            n, E, kind = special(rng)
        else:
            n = rng.randint(1, nmax); p = rng.choice([0.05,0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.9,0.95, rng.random()])
            E = [(i,j) for i in range(n) for j in range(i+1,n) if rng.random() < p]; kind = f"p={p:.2f}"
        path = os.path.join(tmp, f"g{t}.txt"); write(path, n, E)
        b = subprocess.run([BRUTE, path], capture_output=True, text=True).stdout
        bz, bx = int(re.search(r"Z=(\d+)", b).group(1)), int(re.search(r"Xmin=(\d+)", b).group(1))
        try:
            s = subprocess.run([solver, path], capture_output=True, text=True, timeout=60).stdout
            sz, sx = int(re.search(r"Z=(\d+)", s).group(1)), int(re.search(r"X=(\d+)", s).group(1))
        except Exception as e:
            sz = sx = None; s = repr(e)
        if (sz, sx) != (bz, bx):
            bad += 1
            print(f"MISMATCH test {t} n={n} {kind}: brute Z={bz} X={bx} | solver: {s.strip()}  file={path}")
            if bad >= 10: break
    print(f"{cnt} tests, {bad} mismatches" + ("" if bad else " -> ALL OK"))
    sys.exit(1 if bad else 0)
main()
