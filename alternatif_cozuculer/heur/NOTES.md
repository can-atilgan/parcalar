# heur — fast heuristic incumbent for ParçalaR (vertex integrity, tie rule max Y)

`heur.c` (C99, no libraries, `gcc -O2 -Wall -Wextra -std=c99 -pedantic` clean, no `-march`).

```
gcc -O2 -std=c99 -o heur heur.c
./heur FILE [-t SECONDS] [-s SEED] [-v]       # default -t 0.5 (CPU seconds)
Z=<z>  X=<x>
S: <destroyed vertices, 1-based>               # validated with tools/verify.py
```
`-v` prints, on stderr, the best keep_k found for every k, its time, the iterations
spent on each k, and `F=.. Ymax=.. time_to_best=.. time_to_F=..`.
Experiment-only flags: `-T F:Y` (stop when (F,Y) reached), `-K k:target` (single-k ILS),
`-Q a,b,c,d` (schedule), `-E -A -M -N -P -L` (see the tunables block in the source).

## Reusable function (paste-safe block)

Everything between `BEGIN HEUR BLOCK` and `END HEUR BLOCK` is self-contained: all
identifiers are prefixed `hr_`/`HR_`, there is no `main`, and the call is re-entrant
(state is reset and freed on each call). Tested by compiling the block in a harness
that already defines `n`, `adj`, `bs`, `MAXN`, `NW`, `POPC`, `FOREACH`, `deg`, `verbose`:
there were no clashes, and all 300 random calls were consistent.

```c
int heur_run(int n, const uint64_t adj[][4], double tlimit, uint64_t bestK[4], int *F, int *Y);
int heur_run_ex(int n, const uint64_t adj[][4], double tlimit, uint64_t seed, int verbose,
                uint64_t bestK[4], int *F, int *Y, int *keep /* [n+1] or NULL */);
```
* returns F (`-1` if n > 256). `bestK` is a valid kept set. F and Y are **recomputed from
  bestK** (F = |bestK| − maxcomp, Y = maxcomp) before returning, so they are sound lower
  bounds: Z ≤ n − F, and if F is optimal then Ymax ≥ Y.
* `keep[k]` (optional) = largest valid kept set found with all components ≤ k (lower
  bound on keep_k, monotone, keep[n] = n). An exact solver can use these bounds per k
  or per anchor size.
* `tlimit` is CPU time (`CLOCK_PROCESS_CPUTIME_ID`). On small graphs the call stops early
  after 2000·n + 20000 ILS iterations without progress (n = 7: 0.02 s; n = 50: ≈0.45 s).
* Complete graph: F = 0, K = V, Y = n. The input adjacency is symmetrised defensively.
* Memory: about 23 KB per active k plus about 60 KB static. Worst-case stack is under 0.5 MB
  (also tested with `ulimit -s 1024`).

## Algorithm

Reformulation: kept set K, f(K) = |K| − maxcomp(G[K]); keep_k = max |K| with all
components ≤ k; F = max_k (keep_k − k), Ymax = max{k : keep_k − k = F}. If
keep_k − k = F then the solution's largest component is exactly k, so the per-k view
gives the tie rule directly.

**Per-k state** (`hr_St`): kept bitset, component slots (members, outside neighbourhood
`cnb`, size, version), and for every non-kept vertex `msum[v]` = the total size of kept
components adjacent to v. v is insertable at cap k iff `1 + msum[v] ≤ k`. Insert merges
the adjacent components; remove splits a component by bitset BFS. Both update `msum`
only over the affected components' neighbourhoods. `msum` does not depend on k.

**Per-k ILS** (ARW's MIS ILS generalised to "components ≤ k"):
1. *Perturbation:* force a non-kept vertex v in (the older of 2 random candidates;
   with probability 1/(2|K|+2), 2–4 vertices within distance 2). *Repair:* while v's
   future component exceeds k, delete the kept vertex that detaches the most from v. Gains
   are computed per adjacent component (if c touches v at a single vertex, that vertex
   detaches all of c) and cached by slot version; 1/16 of the time a random kept
   neighbour is deleted instead.
2. *Local search:* insert free vertices (smallest resulting component first), then
   (1,2)-swaps. For each component c, a bitset prefilter (threshold sets
   B_t = {w : msum[w] ≤ t}) checks whether removing a vertex of c could free ≥ 2 vertices.
   For each u ∈ c the swap is evaluated **hypothetically** (pieces of c−u, adjusted
   msum, adjusted closures), so failed trials cost nothing. A pair (v,w) is jointly
   insertable iff they are non-adjacent and touch disjoint components, or
   2 + |CL(v) ∪ CL(w)| ≤ k (CL = union of adjacent kept components). If all candidate
   closures share a component, only candidates with msum ≤ k−2 are paired (the main
   dense-graph speed-up, 4x).
3. *Acceptance:* worse solutions are accepted with p = 1/(1 + 2·d·d*) (d = loss vs.
   current, d* = loss vs. best), otherwise the run reverts to the saved solution.

**Sharing between k** (`hr_offer`):
* a k-solution is valid for every k' > k (copied up to active runs);
* a (k+1)-solution shrinks to a k-solution (BFS-prefix of each component);
* **upward probe:** every local optimum at cap k with |K| ≥ best[k+1] is checked for
  vertices with msum ≤ k (insertable at cap k+1). If any exist, a local search at k+1
  runs on a copy. This drives Ymax.
* **anchor enlargement:** for every component C of a new elite K, with R = K∖C: the
  component of G − N[R] containing C, and every other component of G − N[R], gives
  solutions R ∪ D' for each connected BFS-prefix D' with |D'| ≥ maxcomp(R), which
  yields f = |R| at cap |D'|.

**Schedule over k:** ILS runs exist for an active range 1..kact. The range grows while
best[kact] − kact ≥ F − 2. Each round gives a run `gap = F − (best_k − k)`, and the
following iteration counts: gap 0 → 64 (a +1 raises F), gap 1 with k > Ybest → 64
(a +1 raises Ymax), gap 1 with k < Ybest → 4, gap 2 → 1, gap 3 → 1 every 4 rounds,
otherwise 1 every 16 rounds. Graphs with n ≤ 40 activate all k at once. The final
answer is the best (f, maxcomp) over all stored sets, each re-verified.

## Parameters and what was tested (CPU-time based, single thread)

| knob | default | tested | finding |
|---|---|---|---|
| schedule (gap0, gap1>Y, gap1<Y, gap2) | 64,64,4,1 | 64,32,32,8 / 64,64,16,4 / 64,64,8,2 / 64,96,8,2 / 64,128,8,2 / 32,64,8,4 / 64,64,16,2 / 64,64,32,4 / 64,32,4,1 / 64,48,4,1 | (F,Y)@0.5s went 0.71 → 0.91 on the hard training set when low-value k got less time; the variants around the default are within noise |
| acceptance multiplier `-A` | 2 | 0, 0.25, 0.5, 8, 1000 | 0.25–2 equivalent; never accepting worse (1000) is ~13x slower; always accepting (0) is 3x slower |
| repair randomness `-E` | 16 | 1, 2, 4, 1e5 | within noise |
| forced-vertex choice `-M` | older of 2 | uniform, min-msum of 2/3/4 | within noise |
| multi-force prob `-N` | ARW 1/(2|K|+2) | 5%, 20% | 20% worse |
| tabu on re-forcing | off | 3 iterations | far slower (the benchmark did not finish within its time limit); removed |
| exact DFS completion after perturbation | off | 50…2000 nodes | worse on sparse graphs and on p=50/60 (removed) |
| upward probe + enlarge-any-component | on | off | (F,Y)@0.2s 0.61 → 0.73 on the training set; 15090 Y hit@0.1s 0.3 → 1.0 |

Iteration rates (single k, n = 200): p=0.2: about 107k/s at k=3 and 60k/s at k=7; p=0.9:
about 120k/s at k=2 and 60k/s at k=6.

## Evaluation

References (`results/refs.txt`, built by `refs.py`):
* **exact** (90 instances): values from a snapshot of `solvers/rds/solver` (another agent's
  exact solver; unverified), run with 120–900 s timeouts. They cover bench p ≥ 50 (all n),
  p=40 for n ≤ 160, p=30 for n ≤ 130, and generated n=50–90 instances (`inst/`, made with
  `tools/gen.py one … 7`). The task's known values 10050 (10/5), 15050 (11/4) and
  10040 (13/5) are among them, and all match.
* **best-known** (22 instances: p=20 all n, p=30 n ≥ 140, p=40 n ≥ 170): the best (F,Ymax)
  over 4 seeds × 6–8 s runs. These are not proven optimal. Every one of those runs agreed,
  except 19040 (2 of 4 runs found Y=7).
* No run ever exceeded a reference.
* Brute force (`tools/check_small.py`, default settings): final code 300 tests at n ≤ 20,
  300 at n ≤ 22, and 400 at n ≤ 20 with the previous build (same algorithm), plus 900
  tests with earlier development versions, all with different seeds: **ALL OK**. `tools/verify.py` accepts the output on
  every bench, inst and small file.

### Time-to-target, main instances (20 seeds each, 1 s cap; `results/eval_main_t1_s20.txt`)

| inst | ref F/Ymax | src | F hit @0.1/0.2/0.5 s | (F,Y) hit @0.1/0.2/0.5 s | median t_F | median t_FY | max t_FY | (F,Y) in 1 s |
|---|---|---|---|---|---|---|---|---|
| 10020 | 24/6 | bk | 1.00/1.00/1.00 | 0.90/1.00/1.00 | 0.004 | 0.012 | 0.150 | 20/20 |
| 10030 | 17/7 | exact | 1.00/1.00/1.00 | 1.00/1.00/1.00 | 0.001 | 0.005 | 0.022 | 20/20 |
| 10050 | 10/5 | exact | 1.00/1.00/1.00 | 1.00/1.00/1.00 | 0.001 | 0.006 | 0.027 | 20/20 |
| 10090 | 3/3 | exact | 1.00/1.00/1.00 | 1.00/1.00/1.00 | 0.007 | 0.007 | 0.010 | 20/20 |
| 15020 | 27/8 | bk | 1.00/1.00/1.00 | 0.80/0.95/1.00 | 0.001 | 0.043 | 0.215 | 20/20 |
| 15030 | 20/4 | bk | 0.45/0.60/0.90 | 0.45/0.60/0.90 | 0.126 | 0.126 | 0.934 | 20/20 |
| 15050 | 11/4 | exact | 0.90/1.00/1.00 | 0.60/0.75/0.90 | 0.016 | 0.062 | 0.808 | 20/20 |
| 15090 | 3/5 | exact | 1.00/1.00/1.00 | 0.85/0.95/1.00 | 0.000 | 0.026 | 0.371 | 20/20 |
| 20020 | 32/7 | bk | 0.75/1.00/1.00 | 0.10/0.25/0.65 | 0.037 | 0.328 | 0.864 | 17/20 |
| 20030 | 22/6 | bk | 0.65/0.85/0.95 | 0.35/0.60/0.90 | 0.070 | 0.137 | 0.775 | 20/20 |
| 20050 | 12/4 | exact | 0.80/1.00/1.00 | 0.45/0.75/1.00 | 0.050 | 0.125 | 0.453 | 20/20 |
| 20090 | 4/1 | exact | 0.95/1.00/1.00 | 0.95/1.00/1.00 | 0.011 | 0.011 | 0.199 | 20/20 |

Overall (240 runs): F within 0.1/0.2/0.3/0.5/1 s = 0.875/0.954/0.983/0.988/1.000;
(F,Y) = 0.704/0.821/0.900/0.946/0.988. An earlier run of the same code with a 2 s cap gave F 1.000 and (F,Y) 0.996 (239/240).

### Full set (112 instances × 8 seeds, 0.5 s; `results/eval_full_t05_s8.txt`)

F within 0.02/0.05/0.1/0.2/0.3/0.5 s: 0.810/0.891/0.940/0.983/0.992/0.996.
(F,Ymax) within the same budgets: 0.588/0.715/0.806/0.897/0.940/0.961.

| group | #inst | F@.1 | FY@.1 | F@.2 | FY@.2 | F@.5 | FY@.5 |
|---|---|---|---|---|---|---|---|
| n50–90 (exact) | 24 | 1.00 | 1.00 | 1.00 | 1.00 | 1.00 | 1.00 |
| p=20% | 11 | 0.93 | 0.68 | 1.00 | 0.85 | 1.00 | 0.98 |
| p=30% | 11 | 0.85 | 0.74 | 0.96 | 0.86 | 0.99 | 0.97 |
| p=40% | 11 | 0.93 | 0.66 | 0.99 | 0.77 | 1.00 | 0.83 |
| p=50% | 11 | 0.83 | 0.57 | 0.94 | 0.75 | 0.99 | 0.90 |
| p=60% | 11 | 0.85 | 0.65 | 0.94 | 0.81 | 0.98 | 0.94 |
| p=70% | 11 | 0.99 | 0.81 | 1.00 | 0.95 | 1.00 | 0.99 |
| p=80% | 11 | 1.00 | 0.93 | 1.00 | 0.97 | 1.00 | 1.00 |
| p=90% | 11 | 1.00 | 0.99 | 1.00 | 1.00 | 1.00 | 1.00 |

### Failure cases and slow cases

* **Ymax on some p=0.4 graphs.** 19040 (best-known 15/7): Y=7 was found only by 2 of 4
  runs of 8 s, after about 6 s. At 0.5 s every run returns 15/6, so F is right and Y is
  one short. It needs keep_7 = 22, a rare structure the k=7 run takes over 100k
  iterations to hit. 18040 (15/6): 3/8 runs reach Y=6 at 0.5 s.
* **F at a single-k peak.** In 15030 the optimum F=20 exists only at k=4, while k=3..8
  all sit at F−1. The k=4 run alone needs a median of ~2k iterations (25 ms), but it shares
  time with 5–6 other k, giving median 0.13 s and max 0.93 s. Similar: 14050, 16060, 19050,
  20030, 20060 (p=0.5–0.6). About 1–2% of runs are still at F−1 after 0.3 s, and these
  then usually report a larger Y.
* **Ymax at n=200, p=0.2–0.3:** 20020's Y=7 has median 0.33 s; 3/20 runs miss it within 1 s.
* No validity failures. The returned set always satisfies the reported F and Y.

## Recommended budget

As the incumbent for the exact B&B, use **0.3 s** at n = 150–200: F is optimal in 99.2% of
runs, and (F,Ymax) in 94%. Use 0.5 s if a tight Ymax matters (99.6% / 96%). 0.1 s is
enough for n ≤ 90 and for p ≥ 0.8 (F 100%, (F,Y) ≥ 93%). The quality/time trade-off
is the "within" table above. The Ymax gains from a longer budget come mostly on
p = 0.4–0.5 graphs.

## Files

* `heur.c`, `heur`: the heuristic and the CLI.
* `exp.py` (multi-seed best-known), `ttt.py` (time-to-target for given targets),
  `onek.py` (single-k ILS benchmark), `evalall.py` (all referenced instances),
  `refs.py` (merges the references into `results/refs.txt`).
* `inst/`: generated n=50–90 instances. `results/`: exact solver outputs
  (`rds_*.txt`), best-known runs (`ref_*.txt`), evaluation tables, `summary_by_density.txt`.
