# ParcalaR – exact solver "bnb" (approach A, per-k branch and bound)

Files: `solver.c` (single C99 file), `solver` (built with `gcc -O2 -march=native`),
`lab/` (prototypes, experiments, benchmark logs), `inst/` (scratch instances).

```
gcc -O2 -march=native -Wall -o solver solver.c          # also builds warning-free with
gcc -O2 -Wall -Wextra -std=c99 -pedantic -o solver solver.c   # (portable popcount fallback:
                                                              #  -DNO_BUILTINS forces it)
./solver FILE [-v|-vv] [-q] [-t SEC]
```
Output: line 1 `Z=<z>  X=<x>`; line 2 `S: i j ...` = an optimal destroyed set (1-based input
labels, checked with `tools/verify.py`; `-q` omits the line).  `-v` prints a per-phase profile on
stderr.  `-t SEC` is an *opt-in* anytime mode: when the time is up it prints the best (F,Y) found and
says on stderr that it is NOT proven.  Without `-t` the program is exact and runs to the end.

## 1. Reformulation

`K` = kept set, `f(K) = |K| - maxcomp(G[K])`, `F = max f`, `Z = n - F`, `Ymax` = largest
`maxcomp(K)` over optimal `K`, `X = Z - Ymax` (fewest destroyed machines among optimal solutions).

Facts used (besides the ones given in the task):

* **(C1,R) form.**  Let `C1` be a largest component of `K`, `R = K \ C1`.  Then
  `R ⊆ region(C1) := V \ N[C1]`, all comps of `R` have size `<= |C1|`, and `f(K) = |R|`.
  Conversely any connected `C1` and any `R ⊆ region(C1)` with comps `<= |C1|` give `f = |R|`.
* **C2-form.**  By the truncation lemma every `K` with `f(K) >= F+1` truncates to one with
  `s1 = s2 = k` (two components of the same size `k`), so improvements only need to be searched among
  `K = C1 ∪ C2 ∪ R''` with `|C1| = |C2| = k`, `C2 ⊆ region(C1)`, `R'' ⊆ region(C1) \ N[C2]`.
  Canonical choice: `C1` = the k-component with the smallest minimum vertex ("anchor"), so
  `anchor(C2) > anchor(C1)`.
* **Ymax.**  `Ymax = max L(R)` over optimal `R`, where `L(R)` = largest component of `G - N[R]`.
  If `Ymax > Y` for the current `Y`, take an optimal `K` with `s1 = Ymax`.  Either `s2 <= Y+1`: then a
  connected `(Y+1)`-subset of `C1` together with `R` is an optimal `K` with `maxcomp = Y+1` (a "no-C2
  tie at size Y+1"); or `s2 > Y+1`: then truncating `C1` to `s2` gives an optimal C2-form with
  `k = s2 > Y+1`.  So `Ymax = Y` is certified by: no C2-form with `f >= F` and `k > Y+1`, and no
  `R` (comps `<= Y+1`, `|R| >= F`) in the region of any connected `(Y+1)`-set.
* **Degree peeling.**  If `R ⊆ U`, comps `<= s`, `|R| >= need`, then every `v ∈ R` has
  `|U \ N[v]| >= need - s =: θ`.  Sets in which every vertex has `>= θ` non-neighbours are closed
  under union, so iterated removal gives the maximal such subset `peel_θ(U)`, which contains every
  valid `R`; and `peel_θ(U') ⊆ peel_θ(U) ∩ U'` for `U' ⊆ U` (used to pass peeled regions down the
  enumeration: a child starts from the parent's peeled set).
* **Boundary peeling** (stronger, used when the slack `σ = |U| - need` is small): `v ∈ R` needs a
  connected `C ∋ v`, `C ⊆ U`, `|C| <= s`, with `|N(C) ∩ U| <= σ` (the whole boundary of its
  component is outside `R`).  Again union-closed and monotone; the existence of `C` is decided by a
  tiny DFS (each added vertex removes at most one boundary vertex, giving the pruning bound).
* **Packing:** disjoint connected `(s+1)`-sets each lose a vertex: `|R| <= |U| - pack`.
  **Clique cover:** a clique holds at most `min(|Q|, s)` vertices of `R`.
* **Region filter:** a vertex `x` can join `C1` only if `|region(C1) \ N(x)| >= F`
  (regions shrink when `C1` grows).

## 2. Algorithm

1. `alpha(G)` by a bitset MIS branch and bound (colour-class bound) → `F >= alpha - 1`;
   this closes the `k = 1` slice (independent sets).
2. Local search (iterated local search with forced insertion and greedy refill, per `k = 2..12`,
   iterations scaled up for sparse graphs) → starting `(F, Y)`; `Y` is extended to `L(R)`.
   It is only a start: the exact part does not trust it.
3. **Main pass** (this is the "per-k branch and bound", all k in one sweep): for every anchor `c`,
   enumerate every connected set `C1` with min vertex `c` (in/out branching on the frontier, a
   frontier vertex is excluded for the subtree once decided) whose region has `>= F` vertices.
   For a `C1` of size `k >= 2`:
   * `k <= Y`: is there a C2-form `K` with `f >= F+1`?  (improvement)
   * `k == Y+1`: is there `R` in the region, comps `<= k`, `|R| >= F`? (tie, raises Y)
   * `k > Y+1`: is there a C2-form `K` with `f >= F`? (tie with `s2 = k`, raises Y)
   The C2-form check: degree peel (θ = need-k) → packing → boundary peel → enumerate `C2`
   (connected k-set inside the peeled region, anchor `> c`, pruned by the peeled region of the
   rest) → `rest()`: component-at-a-time branch and bound for `R''` (pick the max-degree vertex `v`;
   either `v ∉ R`, or enumerate its component by frontier in/out decisions; bounds: degree peel,
   boundary peel, packing, clique cover, `|C| + min(s-|C|, |frontier|) + bound(rest region)`;
   `s = 1` falls back to MIS).  A success returns a witness `R`; `(F, Y) := (|R|, L(R))` and the
   current anchor is redone with the new values.
4. **Tie pass** (only if `(F, Y)` changed during the main pass at anchor `c*`): anchors `< c*` were
   checked for the size-`(Y_old+1)` tie only, so they are re-checked for the size-`(Y+1)` tie
   (enumeration limited to size `Y+1`, peeled regions passed down).  Any further `Y` change repeats
   it over all anchors; an `F` change re-runs the main pass (cannot happen, kept for safety).

### Exactness and the stop rule
* Every check above is an exact decision procedure (all bounds are valid upper bounds, all branchings
  complete), so a failed check proves non-existence.
* After the main pass (no improvement found): any `K` with `f(K) >= F+1` truncates to a C2-form with
  some `k >= 2` (k = 1 is closed by alpha), whose `C1` has region `>= F+1 >= F` and was therefore
  enumerated and checked → contradiction.  Hence `F` is optimal.
* `Ymax = Y` by the two-case argument of section 1 (tie at `Y+1` checked for all anchors with the
  final `Y`; C2-ties at `k > Y+1` checked in the main pass with need `F`).
* **Stop rule of the k-loop:** there is no explicit loop over `k`.  A `C1` of any size `k` is useful
  only if `|V \ N[C1]| >= F`; regions shrink monotonically when `C1` grows, and the enumeration
  prunes a branch as soon as the region is below `F` (or a frontier vertex would make it so).  When
  the enumeration finishes, every connected set that could be the `C1` of an improving or tying `K`
  has been examined, for every size.  No heuristic part remains (the local search only provides
  the start values; `-t` is the only non-exact mode and it says so).
* Witness: the program prints `S = V \ (R ∪ largest component of G-N[R])` for the stored optimal
  `R`, and self-checks `f = F`, `maxcomp = Y`.

### Verification done
* `tools/check_small.py` (vs `bin/brute`, expects Z and Xmin): 500 (NMAX 18, seed 501) + 500
  (NMAX 20, seed 502) + 300 (NMAX 22, seed 503) + 300 (NMAX 22, seed 504) = 1600 tests, ALL OK; plus
  many smaller runs during development (all OK).  `small/example.txt` → `Z=4  X=1`, `S: 7`.
* Cross-check against an independent exact prototype (`lab/bb2.c`, vertex-at-a-time B&B on the
  (C1,R) formulation, also brute-force verified) on 85 random graphs with n = 26..46 and
  p = 15..80%: 0 differences (`lab/xval.sh`).  On the bench files that bb2 can finish
  (10050..20070) the results also agree.
* `tools/verify.py` accepts the printed witness (feasibility + Z = X + Y) on all tested files.

## 3. Timings
See section 5 (filled from `lab/bench_*.txt`).

## 4. What did not work / lessons
* **Per-k vertex-at-a-time B&B for keep_k** (`lab/bb1.c`, k-plex style: attached capacity +
  clique cover bound): hopeless on sparse graphs – clique covers of sparse random graphs give
  `Σ min(|Q|,k) ≈ |P|`; n = 40, p = 0.2 already needed 20M nodes for k = 6.
* **(C1,R) vertex B&B with L(R) feasibility** (`lab/bb2.c`): exact, elegant (no k loop) but the
  bound is again ~|P|; complete enumeration on the top 5 levels.  Kept as independent checker.
* **Per-k passes** (`lab/bb3.c`): strong per-k peeling of partial C1's, but the C1 enumeration is
  repeated for every k; the shared single pass (`lab/bb4.c` and later) was 3-10x faster.
* **Separate tie phase** for all `k > Y`: replaced by the two-case Ymax argument (only `k = Y+1`
  needs the no-C2 check).
* **kmax/viable-set subtree test** (bound the C1 size of a subtree by the component of A in the
  viable vertices, then peel with that size): cuts 10-15% of the nodes but costs more than it saves.
* **Deletion search** when the slack is small (branch on a connected (k+1)-set, protect earlier
  choices): exponential in the slack; no gain over the C2 enumeration except for slack 0.
* **alpha-based bound** `|R| <= s * alpha(U)` for s = 2,3: almost never rejects.
* **Incremental bit-sliced frontier counters** (subtract the rows of removed region vertices
  instead of recounting): same node count, ~40% slower (copying 9 bit planes per child).
* Boundary peeling inside the C2 enumeration nodes: more expensive than it saves.
* Things that did help, in order of impact: the C1/C2 slice structure with the region filter;
  merging all k into one pass; the frontier filter (drops non-viable vertices for the whole
  subtree); passing peeled regions down (monotonicity); boundary peeling (2x on sparse); the
  two-case Ymax argument (no separate tie phase); a better local search start (avoids re-checks).

## 5. Benchmark results
