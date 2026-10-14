# ParçalaR — exact solver, variant `bnb/b` (C1/C2 enumeration + branch and bound)

Files: `solver.c` (single C99 file), `solver` (built with `gcc -O2 -march=native`),
`lab/` (experiments, scripts, logs), `inst/` (small scratch instances).

```
./solver FILE [-v | -vv] [-t SECONDS]
Z=<z>  X=<x>
S: <destroyed machines, 1-based input indices, ascending>
```

By default the program runs until the answer is proven.  With `-t SECONDS` (CPU time) it
prints the best solution found so far when the limit is hit, writes
"NOT proven optimal" to stderr and exits with code 3 (the `S:` witness is still valid,
only optimality is unproven).

`-v` prints a phase profile to stderr (alpha, heuristic, per pass: number of C1 candidates by
size, C2 / rest / MIS nodes, hits, seconds); `-vv` adds one line per finished anchor.
Builds warning-free with `gcc -O2 -Wall -Wextra -std=c99 -pedantic`; without `-march=native`;
and with `-DNO_BUILTINS` (portable popcount/ctz fallbacks, tested).

## 1. Reformulation

* K = kept set (complement of the destroyed set S), f(K) = |K| − maxcomp(G[K]).
  Z = n − F with F = max f.  Tie rule: X = Z − Ymax, Ymax = largest maxcomp(K) among
  all K with f(K) = F (destroy as few machines as possible).
* Write K = C1 ∪ R with C1 a largest component, k = |C1|.  Then R ⊆ region(C1) :=
  V ∖ N[C1], every component of R has ≤ k vertices and f(K) = |R|.
  Conversely, for any R let L(R) := maxcomp(G − N[R]); if maxcomp(R) ≤ L(R) then
  K = R ∪ (largest component of G − N[R]) has f = |R| and maxcomp = L(R).
  This is how every solution found during the search is evaluated and how the witness
  `S` is produced (S = V ∖ K, recomputed and re-checked at the end).
* Truncation lemma: replacing C1 by a connected s2-subset keeps f.  Hence every K with
  f ≥ T has a twin with the same f in **C2-form**: two components C1, C2 of the same
  size k (C2 ⊆ region(C1), anchors ordered: anchor(C2) > anchor(C1), anchor = min vertex).
* Monotonicity: region(C) shrinks when C grows, and every connected set has connected
  subsets of every smaller size.

## 2. Algorithm

1. **MIS** (bitset branch and bound with greedy clique-partition bound): α(G) gives
   F ≥ α − 1 (all components of size 1).  Y from the independent set: max_v L(I ∖ {v}).
2. **Local search** per k = 2..12 (stops 4 sizes after the last useful one): random
   insert + "cut the overgrown component back to a random connected piece" moves with
   greedy refill; each visited K is evaluated with the L(R) rule above.  Iterations scale
   with sparsity (2 000 per k for dense, up to 22 000 for p ≤ 0.3).  Only a starting point.
3. **Main pass (exact)**: for every anchor c = 0..n−1 (vertices relabelled by
   decreasing degree) enumerate every connected set C1 with min vertex c and
   |region(C1)| ≥ F (each connected set exactly once: frontier branching include/exclude,
   excluded vertices never re-enter).  A frontier vertex x is dropped when
   |N(x) ∩ region| > |region| − F (it would leave < F vertices).  For each C1 of size k ≥ 2,
   with the current (F, Y):
   * k ≤ Y:   is there a C2-form R in region(C1) with |R| ≥ F + 1?  (improvement)
   * k = Y+1: is there any R in region(C1), comps ≤ k, |R| ≥ F?      (tie, no C2 needed)
   * k ≥ Y+2: is there a C2-form R with |R| ≥ F?                     (tie or improvement)

   C2-form check (`slice_c2`): peel region, packing bound, then enumerate connected
   k-sets C2 (anchor > anchor(C1)) inside the region while
   |region ∖ N[C2-so-far]| ≥ need − k (frontier filter again with the unary
   "low-loss" bit-plane counter), then `rest(region ∖ N[C2], k, need − k)`.

   `rest(U, s, need)` = "exists R ⊆ U, comps ≤ s, |R| ≥ need": bounds
   * peeling: v ∈ R ⇒ |R| ≤ min(s, deg_U(v)+1) + |U ∖ N[v]|  ⇒ drop v if
     deg_U(v) > |U| − need + s − 1 (iterated, incremental degree update);
   * packing: disjoint connected (s+1)-sets each lose a vertex;
   * clique cover: each clique keeps ≤ s vertices;
   * s = 1 → exact MIS decision;
   then component-at-a-time branching: take the max-degree vertex v; either v ∉ R, or
   enumerate the whole component C ∋ v (frontier include/exclude, |C| ≤ s, frontier
   filter "the rest must still fit") and recurse on U ∖ N[C].
   Peeled regions are passed from a C1 node to its children (peeling is monotone under
   taking subsets), which roughly halves the peel cost.

   On success the found R updates (F, Y) via (|R|, L(R)) and the current anchor is redone
   with the new values (enumeration is restarted for that anchor only).
4. **Tie pass**: anchors processed before the last change were checked with older
   (F, Y); for them the no-C2 tie at size Y+1 is re-run (C1 connected (Y+1)-sets, region
   peeled for (Y+1, F)).  A hit raises Y and the tie pass is repeated over all anchors.
   (If it ever raised F the main pass would be repeated — cannot happen, see 3.)

## 3. Exactness

*F.*  Let K* be optimal with f(K*) = F* > F (current value).  Truncate to C2-form:
components C1, C2 of size k, f = F*.  If C1, C2 are ordered by anchor, C1 has
|region(C1)| ≥ F* ≥ F + 1, so C1 is enumerated (every connected set with region ≥ F is
enumerated; the frontier filter only removes vertices whose addition makes region < F).
Its check uses need F+1 (k ≤ Y), need F with no C2 requirement (k = Y+1, weaker) or need
F with C2 (k ≥ Y+2, weaker); in all cases R* = K* ∖ C1 satisfies it, so the check succeeds
and F increases.  F only grows; after an update the current anchor is
redone, so when the pass ends every anchor ≥ lim (lim = anchor of the last update) was
checked with the final (F, Y) and every anchor < lim with some older (F', Y'), F' ≤ F.  For
any anchor, the three checks with (F', Y') exclude every C2-form with f ≥ F' + 1 at every
size k (k ≤ Y': need F'+1; k = Y'+1: any R with |R| ≥ F'; k ≥ Y'+2: C2-form with
|R| ≥ F'), hence every C2-form with f ≥ F + 1.  By the truncation lemma no K with f > F
exists: F is optimal.

*Ymax.*  Let K* be optimal with maxcomp Ymax > Y, components s1 = Ymax ≥ s2 ≥ ...
(a) s2 ≤ Y + 1: take a connected (Y+1)-subset C1' of C1*; R* = K* ∖ C1* lies in
region(C1') and has comps ≤ s2 ≤ Y+1, |R*| = F, so the "k = Y+1, no C2" check of C1'
succeeds; the found R' has L(R') ≥ Y+1, so Y increases.
(b) s2 ≥ Y + 2: truncating C1* to s2 gives a C2-form tie at size s2 ≥ Y+2, caught by the
"k ≥ Y+2" check; the found R' has L(R') ≥ s2 > Y + 1.
Each success strictly raises Y and never past Ymax (L(R') of a found R' is the maxcomp of a
real optimal K).  Anchors checked with an older Y' < Y: their k ≥ Y'+2 checks cover all sizes
≥ Y+2 (with need ≤ F); the size-(Y+1) no-C2 check is redone by the tie pass.  If F changed
since (F' < F) every C2-form with f ≥ F was already excluded there at every size.  So when
nothing succeeds any more, Y = Ymax.

*Stop rule for k.*  There is no explicit k-loop: C1 is extended only while
|V ∖ N[C1]| ≥ F (needed by every check, the least need is F).  Because every connected set
of size k contains connected subsets of every smaller size and region shrinks with C1,
the enumeration of all relevant sizes terminates exactly when no connected set of the
next size has region ≥ F — i.e. the largest k that can still carry an optimal or improving
K.  Nothing is heuristic in the exact part; the local search only provides the start.

*Witness.*  Every update stores R; the final K = R ∪ (largest component of G − N[R]) has
f(K) = F and maxcomp(K) = L(R) = Y; the program re-checks this before printing S = V ∖ K.

## 4. Correctness tests

* `tools/check_small.py` through `lab/wrapcheck.sh` (which also runs `tools/verify.py` on the
  `S:` line and turns a witness failure into a mismatch):
  400 tests NMAX 16 seed 101, 400 NMAX 18 seed 102, 300 NMAX 20 seed 103,
  300 NMAX 22 seed 104, 200 NMAX 22 seed 105 → **1600 tests, ALL OK**;
  `-DNO_BUILTINS` build: 150 tests NMAX 20 seed 106 → ALL OK.
  (Earlier development versions passed another ~3000 tests.)
* `small/example.txt` → `Z=4  X=1`, `S: 7`.
* All benchmark answers below were checked with `tools/verify.py` (feasible, X+Y = Z).

## 5. Timings

BENCH_TABLE

## 6. What did not work (measured)

* **bb1** — vertex-at-a-time B&B for keep_k with attached-capacity + capped clique cover:
  n = 40, p = 0.2 already 20 s for k = 6.  Clique bounds are useless for k ≥ 3 on sparse graphs.
* **bb2** — R-first search on the hereditary family {R : maxcomp(R) ≤ L(R)}: fine for dense
  (10050 in 9 s) but 15050 > 300 s and 40-vertex sparse graphs 6 s (current: 0.04 s).
* **bb3** — separate per-k slices (C1 of size k, C2, rest) with a k-loop and an "alive"
  stop: correct but repeats the C1 prefix enumeration for every k; merging all k into one
  enumeration (current design) was 3–10× faster.
* **Russian doll** (cliquer-style, c_k[i] = keep_k of suffixes, vertex-at-a-time): 6020
  k = 6 takes 10 s, k = 7 32 s vs 0.7 s for the whole current solve.
* **Deletion search for small slack** (|U| − need ≤ σ: branch on which vertex of a connected
  (s+1)-set is deleted): σ = 2 neutral, σ = 4 +50 %, σ = 6 +250 % time on 10030.
* **Subtree tests for C1** (all exact and correct, all slower):
  - kmax/peel test (C1 can only grow through "low-loss" vertices, R must survive peeling
    with comps ≤ kmax): cuts nodes but costs more than it saves (+5–10 %).
  - pair test (every C2-form completion needs a connected (a+1)-subset C2' ⊆ region with
    |region ∖ N[C2']| ≥ need(kmax) − kmax): cuts 77 % of tested nodes on 15030 but
    costs ~64 enumeration nodes per test → 2× slower.
  - "tie cap" (same test at |A| = Y with size Y+2, capping the subtree at Y+1): caps 96 %
    on 10030 but the work there is in sizes ≤ Y+1; on 15030 it caps only 3.6 %.
  - size-feasibility test (for each size k > |A|: keep_k(region(A)) ≥ need(k)?; infeasible
    sizes are removed for the whole subtree).  On 15030 every |A| = 4 node has sizes
    5..8 infeasible — but proving it with `rest` costs 700–9000 nodes per test, more than
    the enumeration it saves (10030: 17 s → 30 s with tests at |A| = 2, 129 s at |A| = 7).
  - `rest(region(A), kmax, need)`: almost never fails (kmax too large).
* **Vertex order**: descending vs ascending degree changes only the C2 work; the number of
  C1 candidates is an invariant (= number of connected sets with region ≥ F) — 36 468 016
  on 10030 for both orders.
* More local-search iterations do not reduce the exact work once (F, Y) is optimal.

## 7. Where the time goes / limits

The cost is dominated by the number of connected sets C1 with |V ∖ N[C1]| ≥ F (each C1
check itself is cheap, ~4 B&B nodes on average).  For dense graphs this number is tiny
(Y ≈ 2–5, F ≈ 3–12).  For p ≤ 0.3 it explodes: 10030 has 3.6·10^7 such sets (17 s);
15030 needs 30–75 s per anchor (about 50·10^6 sets per anchor, 150 anchors ⇒ ~2 hours);
10020 needs 25–45 s per anchor (100 anchors ⇒ ~1 hour).  The sizes k ≥ Y+2 (pure tie
checks for the X rule) are about half of the work on 15030 (improvement-only run: 25 s per
anchor instead of 50 s).  So the instances with p ≤ 0.3 and n ≥ 100 (except 10030/11030-like
ones) are out of reach of this design within 300 s; the answer printed on timeout is not
available (the program prints only at the end).
