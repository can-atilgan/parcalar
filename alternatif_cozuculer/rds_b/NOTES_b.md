# rds/b: exact ParcalaR solver, the "canonical balanced anchor" variant

Files: `solver_b.c` (C99, one file, `gcc -O2 -Wall` clean, also builds without
`-march=native`), `solver_b` (built with `gcc -O2 -march=native`), `tools/verified.sh`
(wrapper for `check_small.py` that also runs `tools/verify.py` on the `S:` line),
`bench_results.txt`.

Usage: `./solver_b FILE [-v] [-t SEC]`, or `./solver_b N M` (opens `<N><M>.txt`, e.g.
`./solver_b 100 20` reads `10020.txt`), or `./solver_b` with no file argument (reads N
and M from stdin; the prompts go to stderr). Output:

```
Z=<z>  X=<x>
S: <destroyed computers, 1-based, ascending>
```

`-t SEC` is an optional CPU-time limit. It is off by default, and the solver is then
exact. If the limit is hit, the best solution found so far is printed (it is still a
valid, verified witness), stderr says "optimality NOT proven", and the exit code is 3.

`-v` prints a profile to stderr: the heuristic value, the passes, and per anchor size the
number of sets visited, evaluated, skipped and cut, plus kd nodes. Test-only flags:
`-H0` (no warm start), `-E0` (no anchor extension), `-o K` (vertex order: 0 degree
ascending, 1 descending, >=2 random), `-hr/-hk/-hm` (heuristic budget), `-ho`
(heuristic only, not exact).

This variant was split off from `solvers/rds/solver.c` (the twin instance owns that file).
Both share the anchor enumeration and `kd`. Only this file has the balanced or canonical
evaluation, the extension tests at level Ymax, and the repair passes.

## 1. Why not plain Russian Doll Search

Approach B asked for a Russian Doll Search over `R_k[i] = keep_k(G[v_i..v_n])` plus a
k-loop. I built two prototypes (`../proto0.c` with a vertex-by-vertex search, and
`../proto1.c` with a component-first search). Both had the RDS bound
`min_t (t + R_k[w_t])` and reused the previous k. Measured on G(n, 20%):

| instance | RDS proto1, k = 1..6 (sum) | anchor enumeration (all k) |
|---|---|---|
| n=70 20% | 25 s (k=6 alone 13.4 s) | 4.5 s |
| n=150 50% | k <= 6: 55 s | 2.4 s |
| n=200 50% | k <= 4 already 44 s | 25 s |

RDS cost grows by about 5x per +10 vertices and for every k. It also still needs a
separate argument to stop the k-loop. So I switched to the anchor formulation below. It
covers all k in one enumeration, and its stop rule comes for free.

## 2. Algorithm

**Anchor reformulation (exact).** Take a kept set K with components s1 >= s2 >= ... and
a largest component C. Then the rest R = K \ C lies in W(C) = V \ N[C] and has
components of size <= |C|, with f(K) = |R|. Conversely, any connected C plus any such R
is a kept set with largest component C. Therefore

    F    = max over connected C of g(C),   g(C) = keep_{|C|}(G[W(C)])
    Ymax = max{ |C| : g(C) = F }.

**Enumeration.** ESU over connected sets, each set exactly once, with root = its minimum
vertex. Every node carries W = V \ N[C]. A child w is generated only if
|W \ N(w)| >= F. A bit-sliced counter computes the set of children with
|W ∩ N(w)| <= |W| - F in one sweep. A subtree is cut when |W| < F, because W only
shrinks. The enumeration is finite, and this is the whole stop rule: no loop over k is
needed.

**kd(W, k, t)**: is there K ⊆ W with all components <= k and |K| >= t? This is an exact
branch and bound:
- Iterated forced deletions: a vertex with deg_W >= k + (|W| - t) cannot be kept.
- Components of size <= k are kept whole. Components larger than k get an upper bound
  from a greedy packing of disjoint connected (k+1)-sets, one deletion per packed set.
- When there are several big components, they are solved one by one (keep is additive).
- Otherwise branch on a max-degree vertex w: either w is kept (then enumerate w's whole
  component by growing it, with a "kill-aware" bound: adding x removes N[x] from the
  rest), or w is deleted.

**Warm start**: randomized local search for max |K| with components <= k, for
k <= 8 (k <= 12 and a larger budget on graphs with density < 0.45). Each solution's
largest component is grown inside V \ N[K \ C], which keeps f and can raise Y.

**extend()**: whenever an anchor C reaches g(C) >= F, an optimal rest R is recovered by
self-reduction. C is then grown to its component in V \ N[R]. That is again a valid
optimal solution and can raise Ymax early. This is sound because R is inclusion-minimal
with kd true, hence valid with |R| = F, and C stays non-adjacent to R.

**Canonical balanced anchors (this variant).** Let y be the current Ymax.
- An anchor with |C| = y gets the **full** test g(C) >= F, then g >= F+1. If g(C) = F,
  it also gets the **one-vertex extension test**: for every w in N(C), check
  g(C+w) >= F at size y+1.
- An anchor with |C| != y is only tested if a **balanced** witness could exist whose
  canonical anchor is C. The canonical anchor is the max-size component with the largest
  minimum vertex. The second max-size component D lies in W(C) and contains a vertex
  smaller than root = min(C). Let maxLc be the largest component of G[W(C)] that meets
  V_{<root}. Then:
  - if maxLc < |C|, the anchor is skipped;
  - if |C| >= y and maxLc <= |C|, the whole subtree is cut.
  Thresholds are F+1 for |C| < y and F for |C| > y. Tests always use the exact kd, so a
  positive answer is always a real solution.
- Pass 1 runs over all roots, with (F, y) updated on the fly. If (F, y) changed, repair
  passes follow. A repair evaluates only size-y anchors (full + extension) and is
  depth-limited to y. The first repair only needs roots 0..(root of the last change),
  because later roots were already processed under the final state. After a change
  inside a repair, the next repair redoes all roots. Repairs stop when a pass makes no
  change.

**Witness**: the best kept set comes from the heuristic, or from an anchor plus a rest
rebuilt by self-reduction with kd. It is checked (maxcomp = Ymax, f = F) and printed as
`S:`.

## 3. Exactness argument

Final state (F, y). A clean final pass certifies, for every anchor C:
- (R1) |C| <= y-1: no balanced witness with f >= F+1 has C as canonical anchor;
- (R2) |C| = y: g(C) <= F, and if g(C) = F, every one-vertex extension has g < F;
- (R3) |C| >= y+1: no balanced witness with f >= F has C as canonical anchor.

Certificates are a negative kd at threshold <= the required one, or maxLc < |C|, or an
ancestor cut with maxLc <= that ancestor's size. maxLc only shrinks along a subtree,
because W shrinks and the root is fixed.

**F is optimal.** Suppose f(K*) >= F+1. Truncation gives a balanced K' (s1 = s2 = j)
with the same f. Its canonical anchor C' is reached by ESU, since
|W(C')| >= f > F. C' has a size-j partner D with min(D) < min(C'), so maxLc(C') >= j.
This contradicts R1 (j < y), R2 (j = y, because g(C') >= F+1) or R3 (j > y).

**Ymax is optimal.** Suppose an optimal K has maxcomp Y > y and s2 = j0.
- If j0 >= y+1, truncate C1 to size j0. This gives a balanced optimal solution at size
  j0 > y, which contradicts R3.
- If j0 <= y, truncate C1 to size y+1. This connected set C1' contains a connected
  y-subset C_y (drop a spanning-tree leaf w). Then g(C_y) = F, because the same rest
  works. The extension test of C_y with w is exactly "g(C1') >= F", which is positive.
  This contradicts R2.

Updates only ever come from real solutions, so F <= F* and y <= Y*.

**Mid-pass changes.** (F, y) only increase lexicographically. A negative test at an
older state implies the R1/R3 condition for the final state:
- thresholds used were F'+1 <= F (when F' < F) or the same;
- a full test implies a balanced one;
- maxLc certificates do not depend on the state;
- subtrees cut at an older state only contain sizes > y' with maxLc smaller than their
  size.

The only certificates that can be missing are R2 at the final y. The repair passes
provide them. Roots after the last change were visited entirely under the final state.

**Tests**:
- `check_small.py` through `tools/verified.sh`, so Z, X (= Xmin) and the `S:` witness
  are all checked. The final binary passes 900 tests with NMAX 22 (seed 505: 400; seed 606
  with `-H0 -E0`: 300; seed 707 with `-o 1`: 200). The build just before the `-t` checks
  were added to kd (otherwise the same logic) passed 1,600 (seeds 101 and 202 with
  500 each; seed 303 with `-H0 -E0`; seed 404 with `-o 9`). Earlier builds of the same
  logic passed another 2,800 (1,200 of them with the final pass and repair logic), including:
  - `-H0 -E0` (no warm start, no extension: many state changes and repairs);
  - random vertex orders (`-o 5`, `-o 7`, `-o 13`) and degree-descending order (`-o 1`).
- The example file gives `Z=4  X=1` / `S: 7`.
- Mutation checks: disabling the extension test (10/300 mismatches), disabling repair
  passes (2/300) and skipping the repair entirely (2/300) are all caught. So the suite
  does exercise these code paths.

## 4. Unsound things seen in the shared `solver.c` (twin instance)

- **`kdbal`** (since removed by the twin). It evaluated anchors with |C| <= Ymax only
  through a balanced witness with partner D ⊆ W ∩ V_{>root}. That is fine for detecting
  F. It is wrong for the tie rule: an anchor C' (size s') with an *unbalanced* witness of
  f = F+1 is rejected. The improvement is found later at a smaller canonical anchor and
  sets Ymax to that smaller size. C' is never re-evaluated at the new F, so Ymax can come
  out too small (and X too large). My scheme avoids this with the full test at size y,
  the extension test, and repair passes.
- **`tie = 0` subtree flag in `esu`** (still in the twin's version as I copied it). Once
  a subtree is marked "all descendants <= Ymax", descendants of size > Ymax are not
  evaluated. Ymax can *decrease* inside that subtree when an improvement is found at a
  smaller anchor (`Ymax = s`). Then later anchors of size in (new Ymax, old Ymax] that
  would be ties at the new F are skipped, so X can be too large. This is rare. A fix is
  to pass the subtree's maximum descendant size and compare it with the *current* Ymax.

## 5. Timings

See `bench_results.txt` (bench.sh, 300 s limit, run niced on a machine with load
average ~5 on 4 cores, so wall times are inflated).

| file | result | wall s |
|---|---|---|
| 10090 | Z=97 X=94 | 0.11 |
| 10050 | Z=90 X=85 | 0.34 |
| 10030 | Z=83 X=76 | 24.76 |
| 15090 | Z=147 X=142 | 0.18 |
| 15050 | Z=139 X=135 | 3.86 |
| 20090 | Z=196 X=195 | 0.19 |
| 20050 | Z=188 X=184 | 38.20 |
| 10020 | TIMEOUT | 300.02 |

`10020 TIMEOUT` is the 300 s wall limit. bench.sh was stopped after 10020: the four
remaining files (15030, 20030, 15020, 20020) cannot finish, so they were run with the CPU
limit instead (`-t`, `progress_runs.txt`) to show how far they get:

| file | CPU limit | best found (verified, not proven) | where it stopped |
|---|---|---|---|
| 10020 | 240 s | Z=76 X=70 (F=24, Y=6) | pass 1 at root 4 of 100 (roots 0-3 done); F=24, Y=6 found at root 1 |
| 15030 | 100 s | Z=131 X=125 (F=19, Y=6) | pass 1, root 0 of 150 |
| 20030 | 100 s | Z=179 X=174 (F=21, Y=5) | pass 1, root 0 of 200 |
| 15020 | 100 s | Z=123 X=118 (F=27, Y=5) | pass 1, root 0 of 150 |
| 20020 | 30 s | Z=168 X=163 (F=32, Y=5) | pass 1, root 0: 9 size-5 anchors took 148M kd nodes |

On 10020 the first roots cost about 60 s each (roots 0-3 in 240 s). My older exact
version v3 needed 537 s for the same roots and 1,190 s in total, and proved Z=76 X=70.
My unmeasured estimate for 10020 is therefore about 8-15 CPU minutes. On n >= 150 sparse instances even a single full evaluation (kd on
|W| ~ 70-90 with target ~32) takes seconds. So both the enumeration and kd's weak bound
on large W block those instances.

Head-to-head with my previous single-pass version (`ref_v6`, built from
`solver_b_v6_witness.c`), same machine and similar load:

| instance | ref_v6 | solver_b |
|---|---|---|
| G(70,20%) | 4.3 s | 4.6 s (no repair needed) |
| G(80,20%) | 22.9 s | 21.7 s |
| 10030 | 13.4 s | 15.8 s (repair over roots 0..3 only) |
| 15050 | 2.5 s | 3.0 s |
| 10040 | 0.65 s | 0.94 s |

So the balanced machinery is roughly neutral on random graphs. Its value is that it is a
second, independently argued exact scheme, and that its tie handling is provably right
under mid-run state changes.

## 6. What did not work or helped little

- Plain RDS keep_k (section 1): slower at every size I tried.
- "Dead level" subtree pruning (kd(W, s, F) and kd(W, s+1, F) false, plus a pair test):
  slower. ESU subtrees below the expensive levels hold only ~1-2 nodes, so a per-node
  bound rarely pays.
- The canonical restriction itself helps much less than hoped on random sparse graphs.
  G[W] is almost always one giant component meeting V_{<root}, so maxLc >= |C| nearly
  always. On G(80, 20%) only ~5% of evaluations are skipped and ~7% of nodes cut. What
  helps more is the stronger warm start plus `extend()`: then (F, Ymax) are usually
  right before pass 1 and no repair is needed.
- A stronger necessary condition (the partner D must have <= |W| - t boundary vertices
  inside W) would make the balanced filter much sharper, but it needs a small search per
  node. Not implemented.
- Clique-partition bounds: useless for k >= 3 on sparse graphs, as expected.
- Fundamental limit: the number of connected sets C with |N[C]| <= n - F. For G(100, 20%)
  that is ~3*10^8 sets up to size 7 and ~7.6*10^8 in total. For n >= 150 at 20-30% it grows by orders of
  magnitude, so the sparse large instances are out of reach for this method.
