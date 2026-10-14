# ParçalaR exact solver — approach B ("RDS") — notes

Directory: `/home/user/parcalar/solvers/rds` (the sub-directory `b/` belongs to a twin instance
that split off a variant; it is not part of this deliverable).

* `solver.c` — final solver: C99, one file, `gcc -O2 -Wall -Wextra -std=c99 [-pedantic]`
  warning-free, also builds with clang and without `-march=native`; popcount/ctz use
  `__builtin_*` under `#if defined(__GNUC__)` with portable fallbacks (`-DNO_BUILTINS` tests them).
* `solver` — built with `gcc -O2 -march=native solver.c`.
* `proto0.c`, `proto1.c` — the Russian-Doll-Search prototypes that were measured and abandoned.
* Usage: `./solver FILE [-v]`. stdout:
  ```
  Z=<z>  X=<x>
  S: <destroyed computers, 1-based, ascending>
  ```
  The `S:` witness line (checked by `tools/verify.py`) was added to this file by the twin
  instance after `tools/verify.py` appeared; the solver self-checks the witness
  (exit code 2 + stderr warning if it fails — never observed). `-v` prints a profile to stderr
  (heuristic value, every improvement / tie / extension event, per anchor size: sets visited,
  anchors evaluated, kd nodes; times). `-H0` = no heuristic warm start (testing only).

## 1. What was assigned and what happened

Assigned: generalise Östergård's Russian-doll scheme to `keep_k` (R_k[i] = keep_k of a suffix)
and run a k-loop with a rigorous stop. I implemented two RDS variants (§5): they grow ~×8–10 per
+10 vertices on 20 % graphs, and the k-loop needs a very long tail before any rigorous stop
applies (§3). I replaced the k-loop by an **anchor reformulation** that covers all k in one
enumeration, has a built-in stop rule, and is 1–3 orders of magnitude faster.

## 2. Final algorithm

Kept set K, components s1 ≥ s2 ≥ …, f(K) = |K| − s1, F = max f, Z = n − F,
X = Z − Ymax, Ymax = max{ s1(K) : f(K) = F } (tie rule: fewest destroyed machines).
For a connected set C: W(C) = V \ N[C],
g(C) = max{ |R| : R ⊆ W(C), every component of G[R] has ≤ |C| vertices }.

**Theorem 1.** F = max_C g(C), Ymax = max{ |C| : g(C) = F } (C over nonempty connected sets).
*Proof.* (≥) R ⊆ V \ N[C] means no edge between C and R, so C is a component of K = C ∪ R and
all other components (components of G[R]) have ≤ |C| vertices: s1(K) = |C|, f(K) = |R| = g(C).
(≤) For an optimal K with s1 = Ymax and a largest component C, R = K \ C ⊆ V \ N[C] has
components ≤ |C|, so g(C) ≥ F. ∎

**Enumeration.** ESU (Wernicke 2006) lists every connected set exactly once (root = minimum
vertex). A node only stores W = V \ N[C] and Vext; the exclusive neighbours of a new vertex w
are N(w) ∩ W ∩ {> root}. Children with |W \ N(w)| ≥ threshold are found for all w at once with
a bit-sliced "≤ d neighbours in W" counter.

**Pruning (Lemma 2).** C ⊆ C′ ⇒ W(C′) ⊆ W(C) ⇒ g(C′) ≤ |W(C′)| ≤ |W(C)|. A subtree is cut when
|W| < F_cur (F_cur ≤ F always), so every connected C with |W(C)| ≥ F is visited.
*Tie-mode switch:* descendants only add vertices x ∈ Vext ∪ (W ∩ {> root}) with
|W \ N(x)| ≥ F; if s + #such x ≤ Ymax_cur, no descendant can be a tie larger than Ymax_cur, so
the subtree only needs improvements and uses threshold F_cur + 1.

**Evaluation of anchor C (|C| = s).** need = F_cur + 1 if s ≤ Ymax_cur, else F_cur.
`kd(W, s, need)` decides exactly whether g(C) ≥ need; on an improvement g(C) is pushed up by
further decisions; on a tie/improvement Ymax_cur := s.
*Extension:* after a success, one optimal rest R is recovered by self-reduction (drop v while
kd stays true; the minimal set is a valid rest with |R| = F) and C is grown to its component in
G[V \ N[R]] — again an anchor with rest R, so Ymax_cur can jump (also applied to the warm-start
solution). This also gives the `S:` witness.

**Correctness / stop rule.** F_cur and Ymax_cur only change on certified witnesses (F_cur ≤ F;
Ymax_cur is reset on every improvement). The largest component C* of an optimal K with
s1 = Ymax has g(C*) = F and |W(C*)| ≥ F, hence it is visited (Lemma 2, and the tie-mode switch
only applies when no descendant can exceed Ymax_cur); at its visit either F_cur < F (kd with
need ≤ F succeeds → F_cur = F) or F_cur = F and then Ymax_cur ≥ |C*| afterwards. So at the end
F_cur = F, Ymax_cur = Ymax. The enumeration is finite — **no k-loop and no separate stop rule**:
every component bound k is the anchor size k, and anchors that are too large are cut by Lemma 2.
Special cases: n = 0; complete graph (incl. n = 1) → F = 0, Ymax = n, `Z=n X=0`.

**kd(W, k, t)** — exact B&B "is keep_k(G[W]) ≥ t":
1. |W| < t → no; |W| ≤ k → yes.
2. Forced deletions (iterated): at most d = |W| − t vertices may be dropped, a kept u has a
   component of size ≥ 1 + deg_W(u) − d, so deg_W(u) ≥ k + d forces u out (> d forced → no).
3. Components: those with ≤ k vertices are kept whole; a big component B is bounded by
   |B| − #(greedy disjoint connected (k+1)-subsets). ≥ 2 big components are maximised
   independently (keep is additive) — this fixed an exponential blow-up on disjoint cliques.
4. Branch on a max-degree w: w ∉ R, or w ∈ R and its component is grown by include/exclude on
   frontier vertices. Kill-aware bound while growing C0 (rest U = W \ N[C0], room = k − |C0|):
   total ≤ |C0| + max_{0≤m≤room}(m + |U| − κ_(m)), κ_(m) = m-th smallest |U ∩ N[x]| over
   frontier ∪ U (cut kd nodes ~3× on sparse graphs).

**Warm start:** short iterated local search for k ≤ 8 (force-insert + repair + greedy fill),
only to start with a good F (the exact phase finds improvements itself; `-H0` tested).
**Implementation:** bitsets compiled for 1/2/3/4 words (`#include __FILE__` with a word-count
macro), smallest width chosen at run time; vertices relabelled by degree.

## 3. Why the k-loop is a trap (relevant for every design)

* Two disjoint non-adjacent connected s-sets exist far beyond the optimum Y: the expected number
  of such pairs C(n,s)²(1−p)^{s²} is ≫ 1 up to s ≈ 20 (n=100, p=.2), 25 (180, .2), 14 (100, .3),
  12 (150, .4), 10 (200, .5), 6 (200, .7). The truncation lemma only forces s2 = k when level
  k−1 is not a tie, so a rigorous "no two non-adjacent connected k-sets" stop needs k ≈ 20–25 on
  sparse graphs, and every level near the top has zero slack.
* keep_k − k is flat over many k (5020: 14,16,18,18,17,16,16,16,15,15,14,14,14,13 for k=1..14;
  10070: 5,6,6,5,5,5,4,4,3,3,3,3,2,2).
* Large-Y optima are real: exact Ymax = 7 on 10030, 6 on 15040, 6 on 10020, 7 on 7020/8020
  (heuristics / the instructor's table report Y ≈ 3–6, i.e. a right Z can come with a wrong X).
  On 10020 anchors up to size 15 still had |V \ N[C]| ≥ F and had to be refuted.

## 4. Correctness checks (final binary)

* `small/example.txt` → `Z=4  X=1`, `S: 7`.
* `tools/check_small.py`, all `ALL OK` (1800 tests): witness-verifying wrapper
  (`tools/verify.py` on every answer) 500 tests NMAX 22 seed 101, 500 NMAX 20 seed 102,
  300 NMAX 18 seed 104, 300 NMAX 22 seed 103 with `-H0` (exact search from scratch, no warm
  start); plain 200 NMAX 22 seed 105. Earlier versions passed another ~3000 tests.
* Structured n ≈ 200 graphs (path, cycle, star, random tree, 20 disjoint K10, K_n minus a
  perfect matching, random bipartite, 99 % dense): all exact in < 20 s. Grid 14×14 and 5 %/10 %
  random graphs (outside the 20–90 % test range) time out at 60 s.

## 5. Russian Doll Search measurements (abandoned)

`proto0`: fixed order, vertex-at-a-time, bound s + R_k[min P] and min_t (t + R_k[p_t]).
`proto1`: same tables, component-first search (seed = max-degree vertex), bound
c + room + R_k[min(W \ N(C))]. Degeneracy order was best of 4 orders. Seconds per level k:

| instance | k=1 | 2 | 3 | 4 | 5 | 6 |
|---|---|---|---|---|---|---|
| 7020 proto0 | 0.00 | 0.08 | 1.5 | 13.5 | 44.4 | – |
| 7020 proto1 | 0.01 | 0.17 | 1.07 | 3.48 | 6.98 | 13.4 |
| 15050 proto1 | 0.01 | 0.36 | 1.25 | 5.39 | 20.0 | 28.7 |
| 20050 proto1 | 0.08 | 1.70 | 8.33 | 34.4 | >60 | |
| 20070 proto1 | 0.00 | 0.04 | 0.27 | 1.48 | 1.52 | 1.52 |

Anchor solver, exact over all k: 7020 ≈ 4 s, 15050 ≈ 2.4 s, 20050 ≈ 25 s, 20070 ≈ 0.3 s.
Why RDS is weak here: R_k of a suffix ignores what the partial solution does to the suffix; on
sparse graphs a k-bounded partial set hardly restricts the suffix, so every partial set ahead of
the R curve survives (valid 20-sets outnumber independent 20-sets by ~e^17 in G(180,.2)), and
clique-cover bounds are useless for k ≥ 3.

## 6. Results (wall seconds, 4 shared cores, noisy; timeout 300 s)

Required set (`tools/bench.sh ./solver 300`; TIMEOUT rows from the same algorithm one build
earlier, the final build is not faster on them):

| file | result | s | | file | result | s |
|---|---|---|---|---|---|---|
| 10090 | Z=97 X=94 | 0.04 | | 15090 | Z=147 X=142 | 0.05 |
| 10050 | Z=90 X=85 | 0.13 | | 15050 | Z=139 X=135 | 2.7 |
| 10030 | Z=83 X=76 | 14.8 | | 15030 | TIMEOUT | 300 |
| 10020 | TIMEOUT (exact without limit: Z=76 X=70 in 747 s) | 300 | | 15020 | TIMEOUT | 300 |
| 20090 | Z=196 X=195 | 0.06 | | 20030 | TIMEOUT | 300 |
| 20050 | Z=188 X=184 | 25.1 | | 20020 | TIMEOUT | 300 |

Additional files: 10040 Z=87 X=82 0.71 s · 10060 Z=92 X=88 0.03 · 10070 Z=94 X=91 0.02 ·
10080 Z=96 X=91 0.02 · 11030 Z=92 X=87 47.3 · 12030 Z=102 X=99 223 · 15040 Z=136 X=130 62.7 ·
15060 Z=142 X=137 0.55 · 15070 Z=144 X=138 0.09 · 15080 Z=145 X=143 0.06 · 20040 TIMEOUT ·
20060 Z=190 X=188 1.38 · 20070 Z=193 X=190 0.23 · 20080 Z=195 X=190 0.08.
Witnesses of the fast ones were checked with `tools/verify.py` (all feasible and consistent).

Summary: ≥ 50 % density exact for n ≤ 200 (worst 20050, 25 s), 40 % up to n = 150 (63 s),
30 % up to n ≈ 120 (223 s), 20 % only n = 100 (747 s). Cost = number of connected sets
C with |V \ N[C]| ≥ F (10020: 7.7·10^8 sets, sizes 7–9 dominate; 20050: 6.8·10^7, sizes 4–5);
the per-set work is ~0.3–1.5 µs.

## 7. What did not help

* Dead-level subtree pruning (prove keep_{s+1}(W) < F at a node and "no good pair" of extension
  vertices): kd on the bigger parent set costs more than visiting the children (20–60 % slower).
* Partner (dominance) test: if kd(W,s,F) fails at a node, every witness below has a rest
  component D with |D| ≥ s+1, |V\N[D]| ≥ F and W-boundary ≤ |W| − F (so D is a component of
  G[W \ B], |B| ≤ |W| − F). Implemented for |W| − F ≤ 1: exact, but cut < 2 % of the sets and was
  2–3× slower. Without a cheap bound on the largest connected C with |V\N[C]| ≥ F
  (a co-biclique problem) partner search costs as much as the subtree.
* Balanced canonical evaluation for improvement-only anchors (rest must contain a |C|-component
  with all vertices > root): valid, slower than plain kd on sparse graphs.
* Longer warm starts: the exact phase finds improvements itself; shortest is fastest overall.

Ideas: a cheap rigorous bound on that maximum co-biclique size would make partner / dead-level
pruning effective (~10× on 20050 by estimate); an anytime mode (`F_cur` is always certified, so
printing the best witness at a time limit is trivial, but it would not be exact).
