/*
 * heur.c -- fast heuristic for "ParcalaR" (vertex integrity, tie rule: largest Y).
 *
 *   Destroy S, Y = largest component of G - S, minimise Z = |S| + Y; among optimal
 *   solutions take the largest Y (smallest X = Z - Y).
 *   With K = V \ S kept:  f(K) = |K| - maxcomp(G[K]),  Z = n - max f.
 *   keep_k := max |K| with every component of G[K] <= k vertices,
 *   F = max_k (keep_k - k),  Ymax = max{ k : keep_k - k = F },  X = Z - Ymax.
 *
 * Method (see NOTES.md): one iterated local search per cap k (ARW-style ILS for MIS,
 * generalised to "components <= k": force-insert + greedy cut repair, free insertions,
 * (1,2)-swaps evaluated without touching the state), time-sliced over a dynamic range
 * of k with priority to the k whose +1 improvement would raise (F, Ymax).  Elite
 * solutions are shared between k: a k-solution is a (k+1)-solution, a (k+1)-solution
 * shrinks to a k-solution, every local optimum is probed for insertions at cap k+1,
 * and the anchor-enlargement operator grows/replaces a component C of K inside
 * G - N[K \ C].  Typical: optimum F in <= 0.2 s CPU for ~98% of runs at n = 100..200.
 *
 * The block between "BEGIN HEUR BLOCK" and "END HEUR BLOCK" is self-contained and
 * paste-safe (all identifiers prefixed hr_ / HR_, no main):
 *
 *   int heur_run(int n, const uint64_t adj[][4], double tlimit,
 *                uint64_t bestK[4], int *F, int *Y);
 *
 * returns F (>= 0; -1 if n is out of range).  bestK is a VALID kept set with
 * |bestK| - maxcomp(G[bestK]) = F and maxcomp = Y (both recomputed from bestK before
 * returning, so they are sound lower bounds: Z <= n - F; among solutions with that
 * value Ymax >= Y).  tlimit is CPU seconds.  heur_run_ex additionally takes a seed,
 * a verbosity flag and an optional array keep[0..n] that receives, for every k, the
 * size of the largest valid kept set found with all components <= k (a lower bound
 * on keep_k; monotone in k).  The call is re-entrant (all state is reset and freed),
 * single-threaded, uses ~23 KB per active k plus ~60 KB of static data (stack: < 0.5 MB worst case), and stops
 * early on small graphs (no (F,Y) progress for 2000*n + 20000 ILS iterations).
 *
 * Command line: heur FILE [-t SECONDS(default 0.5)] [-s SEED] [-v]
 *   prints  "Z=<z>  X=<x>"  and on a second line  "S: <destroyed vertices, 1-based>"
 */
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

/* ============================ BEGIN HEUR BLOCK ============================ */
/* requires <stdio.h> <stdlib.h> <string.h> <stdint.h> <time.h>, _POSIX_C_SOURCE >= 199309L */
#define HR_MAXN 256
#define HR_NW 4

typedef struct { uint64_t w[HR_NW]; } hr_bs;

#if defined(__POPCNT__)
#define HR_POPC(x) __builtin_popcountll(x)
#else
static inline int HR_POPC(uint64_t x) {
    x = x - ((x >> 1) & 0x5555555555555555ULL);
    x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
    x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
    return (int)((x * 0x0101010101010101ULL) >> 56);
}
#endif
#define HR_CTZ(x) __builtin_ctzll(x)

static inline int hr_cnt(const hr_bs *a) { return HR_POPC(a->w[0]) + HR_POPC(a->w[1]) + HR_POPC(a->w[2]) + HR_POPC(a->w[3]); }
static inline int hr_empty(const hr_bs *a) { return !(a->w[0] | a->w[1] | a->w[2] | a->w[3]); }
static inline int hr_first(const hr_bs *a) { for (int i = 0; i < HR_NW; i++) if (a->w[i]) return i * 64 + HR_CTZ(a->w[i]); return -1; }
static inline void hr_set(hr_bs *a, int v) { a->w[v >> 6] |= 1ULL << (v & 63); }
static inline void hr_clr(hr_bs *a, int v) { a->w[v >> 6] &= ~(1ULL << (v & 63)); }
static inline int hr_has(const hr_bs *a, int v) { return (int)((a->w[v >> 6] >> (v & 63)) & 1); }
static inline void hr_or_eq(hr_bs *a, const hr_bs *b) { for (int i = 0; i < HR_NW; i++) a->w[i] |= b->w[i]; }
static inline void hr_andnot_eq(hr_bs *a, const hr_bs *b) { for (int i = 0; i < HR_NW; i++) a->w[i] &= ~b->w[i]; }
static inline hr_bs hr_and(const hr_bs *a, const hr_bs *b) { hr_bs r; for (int i = 0; i < HR_NW; i++) r.w[i] = a->w[i] & b->w[i]; return r; }
static inline hr_bs hr_andnot(const hr_bs *a, const hr_bs *b) { hr_bs r; for (int i = 0; i < HR_NW; i++) r.w[i] = a->w[i] & ~b->w[i]; return r; }
static inline int hr_meets(const hr_bs *a, const hr_bs *b) { return ((a->w[0] & b->w[0]) | (a->w[1] & b->w[1]) | (a->w[2] & b->w[2]) | (a->w[3] & b->w[3])) != 0; }
static const hr_bs HR_ZERO = {{0, 0, 0, 0}};

#define HR_FOREACH(v, S, ...) do { hr_bs _s = (S); for (int _i = 0; _i < HR_NW; _i++) { uint64_t _x = _s.w[_i]; \
    while (_x) { int v = _i * 64 + HR_CTZ(_x); _x &= _x - 1; __VA_ARGS__ } } } while (0)

/* ---- graph & globals ---- */
static int hr_n;
static hr_bs hr_adj[HR_MAXN], hr_ALL;

/* ---- tunables ---- */
static int hr_force_eps = 16;       /* 1/eps: repair deletes a random kept neighbour instead of greedy */
static int hr_pair_budget = 4096;   /* max pair tests per (1,2)-swap evaluation */
static double hr_acc_mult = 2.0;    /* acceptance: p = 1/(1 + mult * d * dstar) */
static int hr_probe = 1;            /* upward probe of every local optimum to cap k+1 */
static int hr_fmode = 0;            /* forced-vertex choice: 0 = older of 2 random, m>=1 = min msum of m+1, 9 = uniform */
static int hr_q[4] = {64, 64, 4, 1}; /* ILS iterations per round for gap 0, gap 1 above Ybest, gap 1 below, gap 2 */
static long long hr_stall = -1;     /* stop after this many ILS iterations without (F,Y) progress (-1: 2000*n + 20000) */
static int hr_multi = 0;            /* per-mille prob. of forcing 2..4 vertices (0: ARW 1/(2|K|+2)) */
static int hr_onlyk = 0, hr_onlyT = 0; /* debug: single-k ILS (command line -K k:target) */

/* ---- rng (splitmix64) ---- */
static uint64_t hr_rs = 0x9E3779B97F4A7C15ULL;
static inline uint64_t hr_rnd(void) {
    uint64_t z = (hr_rs += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}
static inline int hr_rndint(int m) { return (int)(((hr_rnd() >> 32) * (uint64_t)m) >> 32); }
static inline double hr_rnd01(void) { return (double)(hr_rnd() >> 11) * (1.0 / 9007199254740992.0); }

static double hr_cpu(void) {
    struct timespec ts;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}
static double hr_t0;
static double hr_elapsed(void) { return hr_cpu() - hr_t0; }

/* ---- state of one ILS for a fixed cap k ---- */
typedef struct {
    int k;
    hr_bs inK;
    int sz;
    int cid[HR_MAXN];                  /* component slot of each kept vertex */
    hr_bs cmem[HR_MAXN], cnb[HR_MAXN];  /* members / outside neighbourhood of a slot */
    int csz[HR_MAXN];
    int fslot[HR_MAXN], nfslot;        /* free slots */
    unsigned cver[HR_MAXN];            /* slot version, bumped on every (re)allocation */
    int msum[HR_MAXN];                 /* v not kept: total size of kept comps adjacent to v */
    int last[HR_MAXN];                 /* iteration when v last changed membership */
} hr_St;

static long long hr_it = 0, hr_it_imp = 0;

static void hr_build(hr_St *s, const hr_bs *K) {
    s->inK = *K;
    s->sz = hr_cnt(K);
    s->nfslot = 0;
    for (int i = HR_MAXN - 1; i >= 0; i--) s->fslot[s->nfslot++] = i;
    for (int v = 0; v < hr_n; v++) s->msum[v] = 0;
    hr_bs rest = *K;
    while (!hr_empty(&rest)) {
        int s0 = hr_first(&rest);
        hr_bs p = HR_ZERO, fr = HR_ZERO, nb = HR_ZERO;
        hr_set(&p, s0); hr_set(&fr, s0);
        while (!hr_empty(&fr)) {
            int x = hr_first(&fr); hr_clr(&fr, x);
            hr_or_eq(&nb, &hr_adj[x]);
            hr_bs nx = hr_and(&hr_adj[x], &rest); hr_andnot_eq(&nx, &p);
            hr_or_eq(&p, &nx); hr_or_eq(&fr, &nx);
        }
        hr_andnot_eq(&rest, &p);
        hr_andnot_eq(&nb, K);
        int c = s->fslot[--s->nfslot], size = hr_cnt(&p);
        s->cver[c]++;
        s->cmem[c] = p; s->cnb[c] = nb; s->csz[c] = size;
        HR_FOREACH(x, p, s->cid[x] = c;);
        HR_FOREACH(w, nb, s->msum[w] += size;);
    }
}

/* insert v (not kept); may create a component larger than k */
static void hr_insert(hr_St *s, int v) {
    hr_bs nk = hr_and(&hr_adj[v], &s->inK);
    hr_bs mem = HR_ZERO, nb = hr_adj[v];
    hr_set(&mem, v);
    int tot = 1, big = -1, bigsz = -1;
    while (!hr_empty(&nk)) {
        int x = hr_first(&nk);
        int c = s->cid[x];
        hr_andnot_eq(&nk, &s->cmem[c]);
        int cs = s->csz[c];
        HR_FOREACH(w, s->cnb[c], s->msum[w] -= cs;);
        hr_or_eq(&mem, &s->cmem[c]);
        hr_or_eq(&nb, &s->cnb[c]);
        tot += cs;
        if (cs > bigsz) { if (big >= 0) s->fslot[s->nfslot++] = big; big = c; bigsz = cs; }
        else s->fslot[s->nfslot++] = c;
    }
    hr_set(&s->inK, v);
    s->sz++;
    hr_andnot_eq(&nb, &s->inK);
    int c;
    if (big >= 0) {
        c = big;
        hr_bs relab = hr_andnot(&mem, &s->cmem[big]);
        HR_FOREACH(x, relab, s->cid[x] = c;);
    } else { c = s->fslot[--s->nfslot]; s->cid[v] = c; }
    s->cver[c]++;
    s->cmem[c] = mem; s->cnb[c] = nb; s->csz[c] = tot;
    HR_FOREACH(w, nb, s->msum[w] += tot;);
}

/* remove kept vertex u */
static void hr_remove(hr_St *s, int u) {
    int c = s->cid[u];
    int cs = s->csz[c];
    HR_FOREACH(w, s->cnb[c], s->msum[w] -= cs;);
    hr_bs rest = s->cmem[c];
    hr_clr(&rest, u);
    s->fslot[s->nfslot++] = c;
    hr_clr(&s->inK, u);
    s->sz--;
    s->msum[u] = 0;
    while (!hr_empty(&rest)) {
        int s0 = hr_first(&rest);
        hr_bs p = HR_ZERO, fr = HR_ZERO, nb = HR_ZERO;
        hr_set(&p, s0); hr_set(&fr, s0);
        while (!hr_empty(&fr)) {
            int x = hr_first(&fr); hr_clr(&fr, x);
            hr_or_eq(&nb, &hr_adj[x]);
            hr_bs nx = hr_and(&hr_adj[x], &rest); hr_andnot_eq(&nx, &p);
            hr_or_eq(&p, &nx); hr_or_eq(&fr, &nx);
        }
        hr_andnot_eq(&rest, &p);
        hr_andnot_eq(&nb, &s->inK);
        int d = s->fslot[--s->nfslot], size = hr_cnt(&p);
        s->cver[d]++;
        s->cmem[d] = p; s->cnb[d] = nb; s->csz[d] = size;
        HR_FOREACH(x, p, s->cid[x] = d;);
        HR_FOREACH(w, nb, s->msum[w] += size;);
    }
}

/* ---- local search ---- */
static int hr_collect_free(hr_St *s, int *out) {
    int m = 0, lim = s->k - 1;
    for (int i = 0; i < HR_NW; i++) {
        uint64_t x = ~s->inK.w[i] & hr_ALL.w[i];
        while (x) { int v = i * 64 + HR_CTZ(x); x &= x - 1; if (s->msum[v] <= lim) out[m++] = v; }
    }
    return m;
}

/* insert free vertices until none is left: prefer smallest resulting component */
static void hr_fill_free(hr_St *s) {
    int buf[HR_MAXN];
    for (;;) {
        int m = hr_collect_free(s, buf);
        if (!m) return;
        int best = -1, bv = 1 << 30, ties = 0;
        for (int i = 0; i < m; i++) {
            int v = buf[i], val = s->msum[v];
            if (val < bv) { bv = val; best = v; ties = 1; }
            else if (val == bv && hr_rndint(++ties) == 0) best = v;
        }
        hr_insert(s, best);
        s->last[best] = (int)hr_it;
    }
}

/* closure of v: union of the kept components adjacent to v */
static inline hr_bs hr_closure(const hr_St *s, int v) {
    hr_bs nk = hr_and(&hr_adj[v], &s->inK), cl = HR_ZERO;
    while (!hr_empty(&nk)) {
        int x = hr_first(&nk);
        const hr_bs *cm = &s->cmem[s->cid[x]];
        hr_or_eq(&cl, cm); hr_andnot_eq(&nk, cm);
    }
    return cl;
}

/* (1,2)-swaps inside component c: for each u in c, evaluate hypothetically (without
   touching the state) which vertices become free when u is deleted and whether two of
   them are jointly insertable: (v,w) is jointly insertable iff they are non-adjacent
   and touch disjoint components, or 2 + |CL(v) u CL(w)| <= k (CL = union of adjacent
   kept components).  Precondition: no free vertex.  Returns 1 if a move was applied. */
static hr_bs hr_clbuf[HR_MAXN];
static hr_bs hr_Bt[HR_MAXN + 1];     /* hr_Bt[j] = {w not kept : msum[w] <= k-1+j}, j = 1..k */
static void hr_make_Bt(const hr_St *s) {
    int k = s->k;
    for (int j = 1; j <= k; j++) hr_Bt[j] = HR_ZERO;
    for (int i = 0; i < HR_NW; i++) {
        uint64_t x = ~s->inK.w[i] & hr_ALL.w[i];
        while (x) {
            int v = i * 64 + HR_CTZ(x); x &= x - 1;
            int j = s->msum[v] - (k - 1); if (j < 1) j = 1;
            if (j <= k) hr_Bt[j].w[i] |= 1ULL << (v & 63);
        }
    }
    for (int j = 2; j <= k; j++) hr_or_eq(&hr_Bt[j], &hr_Bt[j - 1]);
}
static int hr_try_comp(hr_St *s, int c) {
    int k = s->k, cs = s->csz[c], lk = k - 1;
    int L[HR_MAXN], nl = 0, F[HR_MAXN], mF[HR_MAXN], nf;
    hr_bs Lb = hr_and(&s->cnb[c], &hr_Bt[cs]);
    if (hr_cnt(&Lb) < 2) return 0;
    HR_FOREACH(w, Lb, L[nl++] = w;);
    int mem[HR_MAXN], mm = 0;
    hr_bs cm = s->cmem[c];
    HR_FOREACH(x, cm, mem[mm++] = x;);
    for (int a = mm - 1; a > 0; a--) { int j = hr_rndint(a + 1); int t = mem[a]; mem[a] = mem[j]; mem[j] = t; }
    hr_bs P[HR_MAXN], NB[HR_MAXN]; int psz[HR_MAXN];
    for (int a = 0; a < mm; a++) {
        int u = mem[a];
        hr_bs rest = cm; hr_clr(&rest, u);
        int r = 0;
        while (!hr_empty(&rest)) {
            int s0 = hr_first(&rest);
            hr_bs p = HR_ZERO, fr = HR_ZERO, nb = HR_ZERO; hr_set(&p, s0); hr_set(&fr, s0);
            while (!hr_empty(&fr)) {
                int x = hr_first(&fr); hr_clr(&fr, x);
                hr_or_eq(&nb, &hr_adj[x]);
                hr_bs nx = hr_and(&hr_adj[x], &rest); hr_andnot_eq(&nx, &p);
                hr_or_eq(&p, &nx); hr_or_eq(&fr, &nx);
            }
            hr_andnot_eq(&rest, &p);
            P[r] = p; NB[r] = nb; psz[r] = hr_cnt(&p); r++;
        }
        nf = 0;
        for (int i = 0; i < nl; i++) {
            int w = L[i], val = s->msum[w] - cs;
            for (int q = 0; q < r && val <= lk; q++) if (hr_has(&NB[q], w)) val += psz[q];
            if (val <= lk) { mF[nf] = val; F[nf++] = w; }
        }
        if (nf < 2) continue;
        hr_bs inter = hr_ALL;
        int nsmall = 0;
        for (int i = 0; i < nf; i++) {
            hr_bs cl = hr_closure(s, F[i]);
            hr_andnot_eq(&cl, &cm);
            for (int q = 0; q < r; q++) if (hr_has(&NB[q], F[i])) hr_or_eq(&cl, &P[q]);
            hr_clbuf[i] = cl;
            inter = hr_and(&inter, &cl);
            if (mF[i] <= k - 2) nsmall++;
        }
        if (!hr_empty(&inter)) {
            /* every pair shares a component: only pairs with 2 + |union| <= k can work,
               which needs msum <= k-2 for both; keep just those candidates */
            if (nsmall < 2) continue;
            int t = 0;
            for (int i = 0; i < nf; i++) if (mF[i] <= k - 2) { F[t] = F[i]; mF[t] = mF[i]; hr_clbuf[t] = hr_clbuf[i]; t++; }
            nf = t;
        }
        int budget = hr_pair_budget;
        for (int i = 0; i < nf && budget > 0; i++) {
            int v = F[i], mv = mF[i];
            for (int j = i + 1; j < nf; j++) {
                int w = F[j], ok;
                if (--budget < 0) break;
                const hr_bs *x = &hr_clbuf[i], *y = &hr_clbuf[j];
                if (!hr_meets(x, y) && !hr_has(&hr_adj[v], w)) ok = 1;
                else if (2 + (mv > mF[j] ? mv : mF[j]) > k) ok = 0;
                else { hr_bs un = *x; hr_or_eq(&un, y); ok = (2 + hr_cnt(&un) <= k); }
                if (ok) {
                    hr_remove(s, u); hr_insert(s, v); hr_insert(s, w);
                    s->last[u] = s->last[v] = s->last[w] = (int)hr_it;
                    return 1;
                }
            }
        }
    }
    return 0;
}

static void hr_local_search(hr_St *s) {
    int reps[HR_MAXN];
    for (;;) {
        hr_fill_free(s);
        int m = 0;
        hr_bs rest = s->inK;
        while (!hr_empty(&rest)) { int r = hr_first(&rest); reps[m++] = r; hr_andnot_eq(&rest, &s->cmem[s->cid[r]]); }
        for (int i = m - 1; i > 0; i--) { int j = hr_rndint(i + 1); int t = reps[i]; reps[i] = reps[j]; reps[j] = t; }
        int imp = 0;
        hr_make_Bt(s);
        for (int i = 0; i < m && !imp; i++) imp = hr_try_comp(s, s->cid[reps[i]]);
        if (!imp) return;
    }
}

/* force v into the solution: first delete kept vertices so that v's future component
   has size <= k (greedy: delete the vertex that detaches most from v, random ties,
   occasionally a random kept neighbour), then insert v */
static void hr_force_insert(hr_St *s, int v) {
    int cslot[HR_MAXN], cgain[HR_MAXN], cx[HR_MAXN], ncache = 0;
    unsigned cv[HR_MAXN];
    for (;;) {
        hr_bs nk = hr_and(&hr_adj[v], &s->inK);
        int ids[HR_MAXN], nid = 0, total = 1;
        while (!hr_empty(&nk)) {
            int x = hr_first(&nk); int c = s->cid[x];
            hr_andnot_eq(&nk, &s->cmem[c]);
            ids[nid++] = c; total += s->csz[c];
        }
        if (total <= s->k) break;
        int bx = -1;
        if (hr_rndint(hr_force_eps) == 0) {
            hr_bs nv = hr_and(&hr_adj[v], &s->inK);
            int r = hr_rndint(hr_cnt(&nv));
            HR_FOREACH(x, nv, if (r-- == 0) bx = x;);
        } else {
            int bgain = -1, ties = 0;
            for (int i = 0; i < nid; i++) {
                int c = ids[i], cs = s->csz[c], g = -1, gx = -1;
                for (int q = 0; q < ncache; q++) if (cslot[q] == c && cv[q] == s->cver[c]) { g = cgain[q]; gx = cx[q]; break; }
                if (g < 0) {
                    /* best vertex of component c: deleting it detaches the most from v */
                    hr_bs seeds0 = hr_and(&hr_adj[v], &s->cmem[c]);
                    if (hr_cnt(&seeds0) == 1) { g = cs; gx = hr_first(&seeds0); }
                    else {
                        int t2 = 0;
                        HR_FOREACH(x, s->cmem[c], {
                            hr_bs W = s->cmem[c]; hr_clr(&W, x);
                            hr_bs p = hr_and(&seeds0, &W), fr = p;
                            while (!hr_empty(&fr)) {
                                int y = hr_first(&fr); hr_clr(&fr, y);
                                hr_bs nx = hr_and(&hr_adj[y], &W); hr_andnot_eq(&nx, &p);
                                hr_or_eq(&p, &nx); hr_or_eq(&fr, &nx);
                            }
                            int gain = cs - hr_cnt(&p);
                            if (gain > g) { g = gain; gx = x; t2 = 1; }
                            else if (gain == g && hr_rndint(++t2) == 0) gx = x;
                        });
                    }
                    if (ncache < HR_MAXN) { cslot[ncache] = c; cv[ncache] = s->cver[c]; cgain[ncache] = g; cx[ncache] = gx; ncache++; }
                }
                if (g > bgain) { bgain = g; bx = gx; ties = 1; }
                else if (g == bgain && hr_rndint(++ties) == 0) bx = gx;
            }
        }
        hr_remove(s, bx);
        s->last[bx] = (int)hr_it;
    }
    hr_insert(s, v);
    s->last[v] = (int)hr_it;
}

/* ---- elite store (all k) + per-k runs ---- */
static hr_bs hr_bestK[HR_MAXN + 2];
static int hr_bestsz[HR_MAXN + 2];
static double hr_tbest[HR_MAXN + 2];

typedef struct {
    hr_St cur;
    hr_bs saved; int savedsz;
    long long iters;
} hr_Run;

static hr_Run *hr_runs[HR_MAXN + 2];
static int hr_kact = 0;              /* ILS runs for k = 1..hr_kact are active */
static int hr_Fbest = -1, hr_Ybest = 0;
static double hr_t_Fbest = 0, hr_t_F = 0;
static int hr_in_enlarge = 0;
static int hr_tgtF = -1, hr_tgtY = 0;   /* experiments: stop once (F,Y) >= target */

/* largest component of G[K] */
static int hr_maxcomp(const hr_bs *K) {
    hr_bs rest = *K;
    int best = 0;
    while (!hr_empty(&rest)) {
        int s0 = hr_first(&rest);
        hr_bs p = HR_ZERO, fr = HR_ZERO; hr_set(&p, s0); hr_set(&fr, s0);
        while (!hr_empty(&fr)) { int x = hr_first(&fr); hr_clr(&fr, x); hr_bs nx = hr_and(&hr_adj[x], &rest); hr_andnot_eq(&nx, &p); hr_or_eq(&p, &nx); hr_or_eq(&fr, &nx); }
        int c = hr_cnt(&p);
        if (c > best) best = c;
        hr_andnot_eq(&rest, &p);
    }
    return best;
}

/* store K as a solution for cap k; returns 1 if it improves best[k] */
static int hr_store(int k, const hr_bs *K, int sz) {
    if (k < 1 || k > hr_n || sz <= hr_bestsz[k]) return 0;
    hr_bestK[k] = *K; hr_bestsz[k] = sz; hr_tbest[k] = hr_elapsed();
    int f = sz - k;
    if (f > hr_Fbest) hr_t_F = hr_tbest[k];
    if (f > hr_Fbest || (f == hr_Fbest && k > hr_Ybest)) { hr_Fbest = f; hr_Ybest = k; hr_t_Fbest = hr_tbest[k]; hr_it_imp = hr_it; }
    hr_Run *r = hr_runs[k];
    if (r) { hr_build(&r->cur, K); r->saved = *K; r->savedsz = sz; }
    return 1;
}

static void hr_offer(int k, const hr_bs *K, int sz);

/* anchor enlargement: for each component C of K, R = K \ C, D = component of G - N[R]
   containing C; every connected D' (C subset D' subset D, BFS prefix) with
   |D'| >= maxcomp(R) gives a solution for cap |D'| of size |R| + |D'|. */
static void hr_enlarge(const hr_bs *K) {
    hr_bs rest = *K;
    int nc = 0; hr_bs comps[HR_MAXN]; int csz[HR_MAXN];
    while (!hr_empty(&rest)) {
        int s0 = hr_first(&rest);
        hr_bs p = HR_ZERO, fr = HR_ZERO; hr_set(&p, s0); hr_set(&fr, s0);
        while (!hr_empty(&fr)) { int x = hr_first(&fr); hr_clr(&fr, x); hr_bs nx = hr_and(&hr_adj[x], &rest); hr_andnot_eq(&nx, &p); hr_or_eq(&p, &nx); hr_or_eq(&fr, &nx); }
        hr_andnot_eq(&rest, &p);
        comps[nc] = p; csz[nc] = hr_cnt(&p); nc++;
    }
    int ksz = hr_cnt(K);
    for (int i = 0; i < nc; i++) {
        hr_bs R = hr_andnot(K, &comps[i]);
        int mR = 0;
        for (int j = 0; j < nc; j++) if (j != i && csz[j] > mR) mR = csz[j];
        hr_bs NR = R;
        HR_FOREACH(x, R, hr_or_eq(&NR, &hr_adj[x]););
        hr_bs W = hr_andnot(&hr_ALL, &NR);   /* contains comps[i] */
        int ord[HR_MAXN], no = 0;
        hr_bs p = comps[i];
        HR_FOREACH(x, p, ord[no++] = x;);
        for (int h = 0; h < no; h++) {
            hr_bs nx = hr_and(&hr_adj[ord[h]], &W); hr_andnot_eq(&nx, &p);
            HR_FOREACH(y, nx, { ord[no++] = y; hr_set(&p, y); });
        }
        int Rs = ksz - csz[i];
        if (no > csz[i]) {
            hr_bs D = comps[i];
            for (int j = csz[i] + 1; j <= no; j++) {
                hr_set(&D, ord[j - 1]);
                if (j >= mR && Rs + j > hr_bestsz[j]) { hr_bs KK = R; hr_or_eq(&KK, &D); hr_offer(j, &KK, Rs + j); }
            }
        }
        /* other components of G - N[R] (C itself may be replaced by a larger one) */
        hr_bs W2 = hr_andnot(&W, &p);
        while (!hr_empty(&W2)) {
            int s0 = hr_first(&W2);
            int o2[HR_MAXN], n2 = 0;
            hr_bs q = HR_ZERO; hr_set(&q, s0); o2[n2++] = s0;
            for (int h = 0; h < n2; h++) {
                hr_bs nx = hr_and(&hr_adj[o2[h]], &W2); hr_andnot_eq(&nx, &q);
                HR_FOREACH(y, nx, { o2[n2++] = y; hr_set(&q, y); });
            }
            hr_andnot_eq(&W2, &q);
            if (n2 <= csz[i] || n2 < mR) continue;
            hr_bs D = HR_ZERO;
            for (int j = 1; j <= n2; j++) {
                hr_set(&D, o2[j - 1]);
                if (j > csz[i] && j >= mR && Rs + j > hr_bestsz[j]) { hr_bs KK = R; hr_or_eq(&KK, &D); hr_offer(j, &KK, Rs + j); }
            }
        }
    }
}

/* shrink K (cap k+1) to cap k: cut every component to a BFS prefix of size k */
static void hr_shrink_offer(int k, const hr_bs *K) {
    if (k < 1) return;
    hr_bs rest = *K, out = *K;
    while (!hr_empty(&rest)) {
        int s0 = hr_first(&rest);
        int ord[HR_MAXN], no = 0;
        hr_bs p = HR_ZERO; hr_set(&p, s0); ord[no++] = s0;
        for (int h = 0; h < no; h++) {
            hr_bs nx = hr_and(&hr_adj[ord[h]], &rest); hr_andnot_eq(&nx, &p);
            HR_FOREACH(y, nx, { ord[no++] = y; hr_set(&p, y); });
        }
        hr_andnot_eq(&rest, &p);
        int t = no;
        while (t > k) hr_clr(&out, ord[--t]);   /* BFS-order prefix is connected */
    }
    int sz = hr_cnt(&out);
    if (sz > hr_bestsz[k]) hr_offer(k, &out, sz);
}

static void hr_offer(int k, const hr_bs *K, int sz) {
    if (!hr_store(k, K, sz)) return;
    for (int j = k + 1; j <= hr_kact; j++) if (hr_bestsz[j] < sz) hr_store(j, K, sz);   /* still valid */
    if (k > 1) hr_shrink_offer(k - 1, K);
    if (!hr_in_enlarge) { hr_in_enlarge = 1; hr_enlarge(K); hr_in_enlarge = 0; }
}

/* one ILS iteration: perturb (force 1..4 vertices in), exact completion, local search,
   ARW-like acceptance */
static void hr_ils_iter(hr_Run *r) {
    hr_St *s = &r->cur;
    hr_it++;
    r->iters++;
    if (s->sz >= hr_n) return;
    int nf = 1;
    if (hr_multi > 0 ? hr_rndint(1000) < hr_multi : hr_rndint(2 * (s->sz + 1)) == 0) { nf = 2; while (nf < 4 && hr_rndint(2) == 0) nf++; }
    int first = -1;
    for (int f = 0; f < nf; f++) {
        int v = -1;
        if (f == 0) {
            /* sample outside vertices; choose by age (mode 0) or smallest msum (mode>=1) */
            int ns = hr_fmode == 0 ? 2 : (hr_fmode == 9 ? 1 : hr_fmode + 1);
            for (int t = 0; t < ns; t++) {
                int r0 = hr_rndint(hr_n), w = -1;
                for (int q = 0; q < hr_n; q++) { int x = r0 + q; if (x >= hr_n) x -= hr_n; if (!hr_has(&s->inK, x)) { w = x; break; } }
                if (w < 0) break;
                if (v < 0) v = w;
                else if (hr_fmode == 0) { if (s->last[w] < s->last[v]) v = w; }
                else if (s->msum[w] < s->msum[v] || (s->msum[w] == s->msum[v] && s->last[w] < s->last[v])) v = w;
            }
        } else {
            /* within distance 2 of the first forced vertex */
            hr_bs N2 = hr_adj[first];
            HR_FOREACH(x, hr_adj[first], hr_or_eq(&N2, &hr_adj[x]););
            hr_andnot_eq(&N2, &s->inK);
            int m = hr_cnt(&N2);
            if (!m) break;
            int rr = hr_rndint(m);
            HR_FOREACH(x, N2, if (rr-- == 0) v = x;);
        }
        if (v < 0) break;
        if (f == 0) first = v;
        hr_force_insert(s, v);
    }
    hr_local_search(s);
    int sz = s->sz;
    if (sz > hr_bestsz[s->k]) hr_offer(s->k, &s->inK, sz);
    if (hr_probe && s->k < hr_n && sz >= hr_bestsz[s->k + 1]) {
        /* msum does not depend on k: is anything insertable at cap k+1? */
        int k = s->k, any = 0;
        for (int i = 0; i < HR_NW && !any; i++) {
            uint64_t x = ~s->inK.w[i] & hr_ALL.w[i];
            while (x) { int v = i * 64 + HR_CTZ(x); x &= x - 1; if (s->msum[v] <= k) { any = 1; break; } }
        }
        if (any) {
            static hr_St tmp;
            memcpy(&tmp, s, sizeof(hr_St));
            tmp.k = k + 1;
            hr_local_search(&tmp);
            if (tmp.sz > hr_bestsz[k + 1]) hr_offer(k + 1, &tmp.inK, tmp.sz);
        }
    }
    if (sz >= r->savedsz) { r->saved = s->inK; r->savedsz = sz; }
    else {
        int d = r->savedsz - sz, dstar = hr_bestsz[s->k] - sz;
        double pacc = 1.0 / (1.0 + hr_acc_mult * d * dstar);
        if (hr_rnd01() < pacc) { r->saved = s->inK; r->savedsz = sz; }
        else hr_build(s, &r->saved);
    }
}

static void hr_activate(int k) {
    if (k < 1 || k > hr_n || hr_runs[k]) return;
    hr_Run *r = (hr_Run *)calloc(1, sizeof(hr_Run));
    if (!r) return;
    r->cur.k = k;
    hr_runs[k] = r;
    if (k > hr_kact) hr_kact = k;
    hr_bs K = HR_ZERO;
    if (hr_bestsz[k] >= 0 && (k == 1 || hr_bestsz[k] >= hr_bestsz[k - 1])) K = hr_bestK[k];
    else if (k > 1 && hr_bestsz[k - 1] >= 0) K = hr_bestK[k - 1];
    hr_build(&r->cur, &K);
    hr_local_search(&r->cur);
    r->saved = r->cur.inK; r->savedsz = r->cur.sz;
    if (r->cur.sz > hr_bestsz[k]) hr_offer(k, &r->cur.inK, r->cur.sz);
}

int heur_run_ex(int n, const uint64_t adj[][4], double tlimit, uint64_t seed, int verbose,
                uint64_t bestK[4], int *F, int *Y, int *keep) {
    if (n < 0 || n > HR_MAXN) return -1;
    hr_t0 = hr_cpu();
    hr_n = n;
    hr_rs = seed * 0x2545F4914F6CDD1DULL + 12345;
    hr_ALL = HR_ZERO;
    for (int v = 0; v < n; v++) {
        hr_set(&hr_ALL, v);
        for (int i = 0; i < HR_NW; i++) hr_adj[v].w[i] = adj[v][i];
    }
    for (int v = 0; v < n; v++) {           /* defensive: symmetrise, drop loops / out-of-range bits */
        hr_clr(&hr_adj[v], v);
        hr_adj[v] = hr_and(&hr_adj[v], &hr_ALL);
        HR_FOREACH(w, hr_adj[v], hr_set(&hr_adj[w], v););
    }
    for (int k = 0; k <= HR_MAXN + 1; k++) { hr_bestsz[k] = -1; hr_runs[k] = NULL; }
    hr_kact = 0; hr_Fbest = -1; hr_Ybest = 0; hr_t_Fbest = 0; hr_t_F = 0; hr_in_enlarge = 0; hr_it = 0; hr_it_imp = 0;

    int complete = 1;
    for (int v = 0; v < n && complete; v++) if (hr_cnt(&hr_adj[v]) != n - 1) complete = 0;
    if (complete) {                         /* F = 0, keep everything (Y = n) */
        hr_bestK[n > 0 ? n : 0] = hr_ALL;
        if (n > 0) { hr_bestsz[n] = n; hr_Fbest = 0; hr_Ybest = n; }
    } else {
        if (hr_onlyk > 0) {               /* debug: single-k ILS until keep_k >= target */
            hr_activate(hr_onlyk);
            while (hr_elapsed() < tlimit && hr_bestsz[hr_onlyk] < hr_onlyT) hr_ils_iter(hr_runs[hr_onlyk]);
            fprintf(stderr, "ONLYK k=%d keep=%d iters=%lld t=%.4f\n", hr_onlyk, hr_bestsz[hr_onlyk], hr_runs[hr_onlyk]->iters, hr_elapsed());
            tlimit = -1;
        }
        int kinit = n <= 40 ? n : 3;
        for (int k = 1; k <= kinit && tlimit > 0; k++) hr_activate(k);
        long long round = 0, stall = hr_stall >= 0 ? hr_stall : 2000LL * n + 20000;
        while (hr_elapsed() < tlimit && hr_it - hr_it_imp < stall) {
            if (hr_tgtF >= 0 && (hr_Fbest > hr_tgtF || (hr_Fbest == hr_tgtF && hr_Ybest >= hr_tgtY))) break;
            round++;
            while (hr_kact < n && hr_bestsz[hr_kact] - hr_kact >= hr_Fbest - 2) hr_activate(hr_kact + 1);
            for (int k = 1; k <= hr_kact; k++) {
                hr_Run *r = hr_runs[k];
                if (!r) continue;
                int gap = hr_Fbest - (hr_bestsz[k] - k);
                int quota;
                if (gap <= 0) quota = hr_q[0];
                else if (gap == 1) quota = k > hr_Ybest ? hr_q[1] : hr_q[2];
                else if (gap == 2) quota = hr_q[3];
                else if (gap == 3) quota = (round % 4 == 0) ? 1 : 0;
                else quota = (round % 16 == 0) ? 1 : 0;
                for (int q = 0; q < quota; q++) hr_ils_iter(r);
                if (hr_elapsed() >= tlimit) break;
                if (hr_tgtF >= 0 && (hr_Fbest > hr_tgtF || (hr_Fbest == hr_tgtF && hr_Ybest >= hr_tgtY))) break;
            }
        }
    }
    /* final answer, recomputed from the stored sets (sound) */
    int bF = -1, bY = 0, bk = -1;
    for (int k = 1; k <= n; k++) {
        if (hr_bestsz[k] < 0) continue;
        int sz = hr_cnt(&hr_bestK[k]), mc = hr_maxcomp(&hr_bestK[k]);
        if (sz != hr_bestsz[k] || mc > k) { fprintf(stderr, "heur: BUG invalid solution for k=%d\n", k); continue; }
        int f = sz - mc;
        if (f > bF || (f == bF && mc > bY)) { bF = f; bY = mc; bk = k; }
    }
    if (bk < 0) { bF = 0; bY = n > 0 ? 1 : 0; }   /* unreachable for n >= 1: any single vertex */
    if (bestK) {
        if (bk >= 0) for (int i = 0; i < HR_NW; i++) bestK[i] = hr_bestK[bk].w[i];
        else { for (int i = 0; i < HR_NW; i++) bestK[i] = 0; if (n > 0) bestK[0] = 1; }
    }
    if (F) *F = bF;
    if (Y) *Y = bY;
    if (keep) {
        int run = 0;
        for (int k = 0; k <= n; k++) {
            if (k >= 1 && hr_bestsz[k] > run) run = hr_bestsz[k];
            keep[k] = run;
        }
        if (n > 0) keep[n] = n;
    }
    if (verbose) {
        int kshow = hr_kact > bY ? hr_kact : bY;
        for (int k = 1; k <= kshow; k++) {
            if (hr_bestsz[k] < 0) continue;
            fprintf(stderr, "k=%3d keep=%3d f=%3d t=%.3fs iters=%lld\n", k, hr_bestsz[k], hr_bestsz[k] - k, hr_tbest[k], hr_runs[k] ? hr_runs[k]->iters : 0LL);
        }
        fprintf(stderr, "F=%d Ymax=%d time_to_best=%.3fs time_to_F=%.3fs total=%.3fs\n", bF, bY, hr_t_Fbest, hr_t_F, hr_elapsed());
    }
    for (int k = 0; k <= HR_MAXN + 1; k++) { free(hr_runs[k]); hr_runs[k] = NULL; }
    return bF;
}

int heur_run(int n, const uint64_t adj[][4], double tlimit, uint64_t bestK[4], int *F, int *Y) {
    return heur_run_ex(n, adj, tlimit, 1, 0, bestK, F, Y, NULL);
}
/* ============================= END HEUR BLOCK ============================= */

/* ---------------------------------------------------------------- command line */
static uint64_t g_adj[HR_MAXN][4];

static int read_graph(const char *path, int *pn) {
    FILE *f = fopen(path, "r");
    if (!f) { perror(path); return 0; }
    int n, m;
    if (fscanf(f, "%d %d", &n, &m) != 2) { fprintf(stderr, "bad header\n"); fclose(f); return 0; }
    if (n < 0 || n > HR_MAXN) { fprintf(stderr, "n out of range (max %d)\n", HR_MAXN); fclose(f); return 0; }
    long cnt = 0, tot = (long)n * n; int c;
    while (cnt < tot && (c = fgetc(f)) != EOF) {
        if (c == '0' || c == '1') {
            int i = (int)(cnt / n), j = (int)(cnt % n);
            if (c == '1' && i != j) { g_adj[i][j >> 6] |= 1ULL << (j & 63); g_adj[j][i >> 6] |= 1ULL << (i & 63); }
            cnt++;
        }
    }
    fclose(f);
    *pn = n;
    return 1;
}

int main(int argc, char **argv) {
    const char *path = NULL;
    uint64_t seed = 1;
    double tl = -1;
    int verbose = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-t") && i + 1 < argc) tl = atof(argv[++i]);
        else if (!strcmp(argv[i], "-s") && i + 1 < argc) seed = strtoull(argv[++i], NULL, 10);
        else if (!strcmp(argv[i], "-v")) verbose = 1;
        else if (!strcmp(argv[i], "-E") && i + 1 < argc) hr_force_eps = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-A") && i + 1 < argc) hr_acc_mult = atof(argv[++i]);
        else if (!strcmp(argv[i], "-P") && i + 1 < argc) hr_probe = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-M") && i + 1 < argc) hr_fmode = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-N") && i + 1 < argc) hr_multi = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-L") && i + 1 < argc) hr_stall = atoll(argv[++i]);
        else if (!strcmp(argv[i], "-Q") && i + 1 < argc) sscanf(argv[++i], "%d,%d,%d,%d", &hr_q[0], &hr_q[1], &hr_q[2], &hr_q[3]);
        else if (!strcmp(argv[i], "-K") && i + 1 < argc) { if (sscanf(argv[++i], "%d:%d", &hr_onlyk, &hr_onlyT) != 2) hr_onlyk = 0; }
        else if (!strcmp(argv[i], "-T") && i + 1 < argc) { if (sscanf(argv[++i], "%d:%d", &hr_tgtF, &hr_tgtY) != 2) hr_tgtY = 0; }
        else path = argv[i];
    }
    if (!path) { fprintf(stderr, "usage: %s FILE [-t SECONDS] [-s SEED] [-v]\n", argv[0]); return 1; }
    int n;
    if (!read_graph(path, &n)) return 1;
    if (tl < 0) tl = 0.5;
    if (n == 0) { printf("Z=0  X=0\nS:\n"); return 0; }
    uint64_t K[4];
    int F, Y;
    heur_run_ex(n, (const uint64_t (*)[4])g_adj, tl, seed, verbose, K, &F, &Y, NULL);
    int Z = n - F;
    printf("Z=%d  X=%d\nS:", Z, Z - Y);
    for (int v = 0; v < n; v++) if (!((K[v >> 6] >> (v & 63)) & 1)) printf(" %d", v + 1);
    printf("\n");
    return 0;
}
