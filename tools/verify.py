#!/usr/bin/env python3
"""Independent feasibility check of a ParcalaR answer.
Usage: verify.py GRAPHFILE [OUTPUTFILE]   (output read from stdin if OUTPUTFILE is omitted)
The output must contain `Z=<z>` `X=<x>` and a line `S: i j k ...` listing the destroyed
computers (1-based). Recomputes X=|S| and Y = largest connected component of G - S and
checks X+Y == Z. Prints the component sizes of the remaining network."""
import sys, re
from collections import deque

def read_graph(path):
    data = open(path).read()
    head, _, rest = data.partition('\n')
    n = int(head.split()[0])
    bits = [c for c in rest if c in '01']
    adj = [set() for _ in range(n)]
    for i in range(n):
        for j in range(n):
            if i != j and bits[i * n + j] == '1':
                adj[i].add(j); adj[j].add(i)
    return n, adj

def main():
    n, adj = read_graph(sys.argv[1])
    out = open(sys.argv[2]).read() if len(sys.argv) > 2 else sys.stdin.read()
    Z = int(re.search(r"Z=(\d+)", out).group(1)); X = int(re.search(r"X=(\d+)", out).group(1))
    m = re.search(r"^S:(.*)$", out, re.M)
    S = [int(t) for t in m.group(1).split()] if m else []
    ok = True
    if len(set(S)) != len(S) or any(not (1 <= v <= n) for v in S):
        print("HATA: S listesinde tekrar eden ya da aralik disi indis var"); ok = False
    rem = set(range(n)) - {v - 1 for v in S}
    seen, sizes = set(), []
    for s in sorted(rem):
        if s in seen: continue
        q = deque([s]); seen.add(s); c = 0
        while q:
            u = q.popleft(); c += 1
            for w in adj[u]:
                if w in rem and w not in seen:
                    seen.add(w); q.append(w)
        sizes.append(c)
    Y = max(sizes) if sizes else 0
    sizes.sort(reverse=True)
    print(f"n={n}  |S|={len(S)}  kalan={len(rem)}  bilesen boyutlari={sizes}")
    print(f"hesaplanan: X={len(S)} Y={Y} Z={len(S) + Y}   |   program: Z={Z} X={X}")
    if len(S) != X: print("HATA: |S| != X"); ok = False
    if len(S) + Y != Z: print("HATA: X+Y != Z"); ok = False
    print("UYGUN (feasible) - tutarli" if ok else "UYGUN DEGIL / tutarsiz")
    sys.exit(0 if ok else 1)
main()
