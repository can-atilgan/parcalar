#!/usr/bin/env python3
"""Independent CP-SAT checker for ParcalaR (vertex integrity with tie info).
Model: x_v kept; y[v][b] (b<=v) = v belongs to component whose min-index vertex is b.
Adjacent kept vertices share a bin; bin sizes <= Y; maximize sum(x) - Y.
Then with sum(x)-Y == F fixed, maximize / minimize Y.
Prints: Z=<z> Xmin=<x> Xmax=<x>  (status info on stderr)"""
import sys, time
from ortools.sat.python import cp_model

def read(path):
    data = open(path).read()
    head, _, rest = data.partition('\n')
    n, m = map(int, head.split()[:2])
    bits = [c for c in rest if c in '01']
    A = [[0]*n for _ in range(n)]
    for i in range(n):
        for j in range(n):
            if bits[i*n+j] == '1' and i != j:
                A[i][j] = A[j][i] = 1
    return n, A

def build(n, A):
    md = cp_model.CpModel()
    x = [md.NewBoolVar(f"x{v}") for v in range(n)]
    y = [[md.NewBoolVar(f"y{v}_{b}") for b in range(v+1)] for v in range(n)]
    Y = md.NewIntVar(0, n, "Y")
    for v in range(n):
        md.Add(sum(y[v]) == x[v])
        for b in range(v):
            md.AddImplication(y[v][b], y[b][b])
    for u in range(n):
        for v in range(u+1, n):
            if A[u][v]:
                # both kept -> same bin. bins available to u: b<=u ; to v: b<=v (u<v)
                for b in range(u+1):
                    # y[u][b] & x[v] -> y[v][b]
                    md.AddBoolOr([y[u][b].Not(), x[v].Not(), y[v][b]])
                    md.AddBoolOr([y[v][b].Not(), x[u].Not(), y[u][b]])
                for b in range(u+1, v+1):
                    # v in bin b>u while u kept would require u in b: impossible
                    md.AddBoolOr([y[v][b].Not(), x[u].Not()])
    for b in range(n):
        md.Add(sum(y[v][b] for v in range(b, n)) <= Y)
    return md, x, Y

def solve(md, tl, workers=8):
    s = cp_model.CpSolver()
    s.parameters.max_time_in_seconds = tl
    s.parameters.num_workers = workers
    st = s.Solve(md)
    return s, st

if __name__ == '__main__':
    path = sys.argv[1]
    tl = float(sys.argv[2]) if len(sys.argv) > 2 else 600
    n, A = read(path)
    t0 = time.time()
    md, x, Y = build(n, A)
    md.Maximize(sum(x) - Y)
    s, st = solve(md, tl)
    if st != cp_model.OPTIMAL:
        print("UNKNOWN", s.StatusName(st), s.ObjectiveValue(), s.BestObjectiveBound(), file=sys.stderr)
        print(f"Z=? lb_f={int(s.ObjectiveValue())} ub_f={int(s.BestObjectiveBound())}")
        sys.exit(2)
    F = int(round(s.ObjectiveValue()))
    md.Add(sum(x) - Y == F)
    md.ClearObjective(); md.Maximize(Y)
    s2, st2 = solve(md, tl)
    Ymax = int(round(s2.ObjectiveValue())) if st2 == cp_model.OPTIMAL else None
    md.ClearObjective(); md.Add(Y >= 1); md.Minimize(Y)
    s3, st3 = solve(md, tl)
    Ymin = int(round(s3.ObjectiveValue())) if st3 == cp_model.OPTIMAL else None
    Z = n - F
    print(f"Z={Z} Xmin={Z-Ymax if Ymax is not None else '?'} Xmax={Z-Ymin if Ymin is not None else '?'}")
    print(f"time {time.time()-t0:.2f}s", file=sys.stderr)
