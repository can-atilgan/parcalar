/*
 * ParcalaR exact solver  (vertex integrity:  Z = min_S |S| + maxcomp(G - S);
 * tie rule: among optimal S the smallest X = |S|, i.e. the largest Y).
 *
 * Kullanim / Usage:
 *   ./parcalar                 -> N ve M sorulur, dosya adi "<N><M>.txt" (or. 100 20 -> 10020.txt)
 *   ./parcalar 100 20          -> 10020.txt
 *   ./parcalar DOSYA.txt       -> verilen dosya
 *   secenekler: -v (profil, stderr)  -q (S satirini yazma)  -t SN (zaman siniri: bulunan en iyi
 *               cozum yazilir, optimallik ISPATLANMADI uyarisi verilir; varsayilan: sinirsiz, kesin)
 * Output: line 1  "Z=<z>  X=<x>"
 *         line 2  "S: i j k ..."  destroyed machines (1-based, an optimal witness)
 *   stderr: self-check of the witness and the elapsed time.
 * Derleme / build:  gcc -O2 parcalar.c -o parcalar            (tek cekirdek)
 *                   gcc -O2 -fopenmp parcalar.c -o parcalar   (tum cekirdekler; sonuc ayni)
 *
 * Reformulation (proofs in NOTES.md):
 *   K = kept set, f(K) = |K| - maxcomp(G[K]),  F = max f,  Z = n - F,
 *   Ymax = largest maxcomp(K) over optimal K,  X = Z - Ymax.
 *   K = (C1, R): C1 a largest component, R = K \ C1 inside region(C1) = V \ N[C1],
 *   all comps of R <= |C1|, f(K) = |R|.  Ymax = max over optimal R of L(R), the largest
 *   component of G - N[R].
 *   Truncation lemma => every K with f(K) >= F+1 yields one with s1 = s2 ("C2-form").
 *
 * Algorithm:
 *   1. alpha(G) by bitset MIS branch and bound  -> F >= alpha-1, the k = 1 slice is closed.
 *   2. local-search heuristic for a starting (F, Y)  (Y extended with L(R)).
 *   3. main pass: enumerate every connected set C1 of size k >= 2 (canonical: anchor = min
 *      vertex, other vertices larger) whose region has >= F vertices; for each one decide
 *        k <= Y  : exists C2-form K with f >= F+1 ?          (improvement)
 *        k == Y+1: exists R, comps <= k, |R| >= F ?          (tie raising Y, no C2 needed)
 *        k >  Y+1: exists C2-form K with f >= F ?            (tie raising Y)
 *      C2 = another connected k-set in the region with anchor(C2) > anchor(C1); the rest R''
 *      is found by a component-at-a-time branch and bound.  Bounds: degree peeling (monotone,
 *      passed down the enumeration), boundary peeling, packing, clique cover.  A success
 *      updates (F, Y) := (|R|, L(R)) and the current anchor is redone.  Anchors are processed
 *      in parallel (OpenMP, optional); a pass is repeated until one complete pass makes no
 *      change to (F, Y): that pass decided every anchor under a single fixed (F, Y), which is
 *      the proof of optimality (and of the Ymax tie rule).
 *   The enumeration ends by itself when no connected set has a region of >= F vertices, which
 *   is the rigorous stop rule for the k-loop (no separate bound needed).
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#define MAXN 256
#define MW 4

typedef struct { uint64_t w[MW]; } bs;

#if defined(__GNUC__) && !defined(NO_BUILTINS)
#define POPC(x) __builtin_popcountll(x)
#define CTZ(x) __builtin_ctzll(x)
#else
static int popc_fb(uint64_t x) {
    x = x - ((x >> 1) & 0x5555555555555555ULL);
    x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
    x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
    return (int)((x * 0x0101010101010101ULL) >> 56);
}
static int ctz_fb(uint64_t x) { int c = 0; while (!(x & 1)) { x >>= 1; c++; } return c; }
#define POPC(x) popc_fb(x)
#define CTZ(x) ctz_fb(x)
#endif

static int n;
static bs adj[MAXN], nadj[MAXN], ALL, above[MAXN];
static int gdeg[MAXN];
static int orig[MAXN]; /* internal label -> input index (0-based) */
static int VERB = 0;
static int WITNESS = 1;  /* second stdout line 'S: ...' (destroyed machines, 1-based); -q suppresses */
static double TLIM = 0;          /* -t SEC: anytime mode (0 = exact, no limit) */
static time_t T_START;
static volatile int aborted = 0;
static long tick = 0;
#ifdef _OPENMP
#pragma omp threadprivate(tick)
#endif

static long LSIT = -1;      /* LS iterations per k (debug) */

/* ------------------------------------------------------------------ bitsets */
static void bclr(bs *a) { int i; for (i = 0; i < MW; i++) a->w[i] = 0; }
static void bset(bs *a, int v) { a->w[v >> 6] |= 1ULL << (v & 63); }
static void bunset(bs *a, int v) { a->w[v >> 6] &= ~(1ULL << (v & 63)); }
static int btest(const bs *a, int v) { return (int)((a->w[v >> 6] >> (v & 63)) & 1); }
static int NW = MW; /* words actually used */
static int bcount(const bs *a) { int i, c = 0; for (i = 0; i < NW; i++) c += POPC(a->w[i]); return c; }
static int bempty(const bs *a) { int i; for (i = 0; i < NW; i++) if (a->w[i]) return 0; return 1; }
static bs band(bs a, bs b) { int i; for (i = 0; i < MW; i++) a.w[i] &= b.w[i]; return a; }
static bs bor(bs a, bs b) { int i; for (i = 0; i < MW; i++) a.w[i] |= b.w[i]; return a; }
static bs bandn(bs a, bs b) { int i; for (i = 0; i < MW; i++) a.w[i] &= ~b.w[i]; return a; }
static int bfirst(const bs *a) { int i; for (i = 0; i < NW; i++) if (a->w[i]) return i * 64 + CTZ(a->w[i]); return -1; }
static int bandcount(const bs *a, const bs *b) { int i, c = 0; for (i = 0; i < NW; i++) c += POPC(a->w[i] & b->w[i]); return c; }
#define FOREACH(v, set) \
    for (int v##_i = 0; v##_i < NW; v##_i++) \
        for (uint64_t v##_x = (set).w[v##_i]; v##_x; v##_x &= v##_x - 1) \
            for (int v = v##_i * 64 + CTZ(v##_x), v##_o = 1; v##_o; v##_o = 0)

static double cpu(void) { return (double)clock() / CLOCKS_PER_SEC; }
#ifdef _OPENMP
#include <omp.h>
static double wall(void) { return omp_get_wtime(); }
#else
static double wall(void) { return (double)clock() / CLOCKS_PER_SEC; }
#endif
static double W_START;

/* connected component of v inside S */
static bs compof(int v, bs S) {
    bs c, fr;
    bclr(&c); bset(&c, v); fr = c;
    while (!bempty(&fr)) {
        bs nf; bclr(&nf);
        FOREACH(u, fr) { nf = bor(nf, adj[u]); }
        nf = band(nf, S); nf = bandn(nf, c);
        c = bor(c, nf); fr = nf;
    }
    return c;
}
static int maxcomp(bs S) {
    int mx = 0;
    while (!bempty(&S)) {
        int s;
        bs c = compof(bfirst(&S), S);
        s = bcount(&c);
        if (s > mx) mx = s;
        S = bandn(S, c);
        if (bcount(&S) <= mx) break;
    }
    return mx;
}
/* L(R) = largest component of G - N[R] */
static int Lof(bs R) {
    bs N = R;
    FOREACH(v, R) { N = bor(N, adj[v]); }
    return maxcomp(bandn(ALL, N));
}

/* ------------------------------------------------------------------ statistics */
static long st_mis, st_rest, st_comp, st_c1, st_c2, st_slice[MAXN], st_pass[MAXN], st_work[MAXN], st_hit;
#ifdef _OPENMP
#pragma omp threadprivate(st_mis, st_rest, st_comp, st_c1, st_c2, st_slice, st_pass, st_work, st_hit)
#endif


/* ------------------------------------------------------------------ MIS (k = 1 and s = 1 rests) */
static bs mis_cur, mis_sol;
static int mis_best;
#ifdef _OPENMP
#pragma omp threadprivate(mis_cur, mis_sol, mis_best)
#endif

static int mis_rec(bs P, int size, int target) {
    int order[MAXN], col[MAXN], cnt = 0, c = 0, i;
    bs U = P;
    st_mis++;
    while (!bempty(&U)) {
        bs Q = U;
        c++;
        while (!bempty(&Q)) {
            int v = bfirst(&Q);
            bunset(&U, v); bunset(&Q, v);
            Q = band(Q, adj[v]);
            order[cnt] = v; col[cnt] = c; cnt++;
        }
    }
    for (i = cnt - 1; i >= 0; i--) {
        int v = order[i], r = 0;
        bs NP;
        if (size + col[i] <= mis_best) return 0;
        bset(&mis_cur, v);
        NP = bandn(P, nadj[v]);
        if (bempty(&NP)) {
            if (size + 1 > mis_best) { mis_best = size + 1; mis_sol = mis_cur; if (mis_best >= target) r = 1; }
        } else r = mis_rec(NP, size + 1, target);
        bunset(&mis_cur, v);
        if (r) return 1;
        bunset(&P, v);
    }
    return 0;
}
/* 1 if alpha(G[U]) >= need ; solution in mis_sol */
static int mis_dec(bs U, int need) {
    bclr(&mis_cur); mis_best = need - 1;
    return mis_rec(U, 0, need);
}
static int mis_opt(bs U) {
    bclr(&mis_cur); bclr(&mis_sol); mis_best = 0;
    mis_rec(U, 0, MAXN + 1);
    return mis_best;
}

/* ------------------------------------------------------------------ bounds for "R in U, comps <= s, |R| >= need" */
/* peeling: v in R  =>  |R| <= min(s, deg_U(v)+1) + |U \ N[v]|   <=>  deg_U(v) <= |U| - need + s - 1 */
static bs peelh(bs U, int s, int need, int *hint) {
    int cnt = bcount(&U), D, mx = 0, any = 0;
    int dg[MAXN];
    bs rm;
    if (cnt < need) return U;
    D = cnt - need + s - 1;
    if (D >= cnt - 1 || D >= *hint) return U;
    bclr(&rm);
    FOREACH(u, U) {
        int d = bandcount(&adj[u], &U);
        dg[u] = d;
        if (d > D) { bset(&rm, u); any = 1; } else if (d > mx) mx = d;
    }
    if (!any) { *hint = mx; return U; }
    for (;;) {
        /* remove rm, update degrees, recheck with the new D */
        int removed = bcount(&rm);
        U = bandn(U, rm);
        cnt -= removed;
        if (cnt < need) return U;
        FOREACH(r, rm) {
            bs nb = band(adj[r], U);
            FOREACH(v, nb) { dg[v]--; }
        }
        D = cnt - need + s - 1;
        bclr(&rm); any = 0; mx = 0;
        FOREACH(u, U) {
            if (dg[u] > D) { bset(&rm, u); any = 1; } else if (dg[u] > mx) mx = dg[u];
        }
        if (!any) { *hint = mx; return U; }
    }
}
static bs peel(bs U, int s, int need) { int h = MAXN; return peelh(U, s, need, &h); }
/* greedy clique partition, each clique holds at most s kept vertices */
static int cliquebound(bs U, int s) {
    int ub = 0;
    while (!bempty(&U)) {
        bs Q = U;
        int t = 0;
        while (!bempty(&Q)) {
            int v = bfirst(&Q);
            bunset(&U, v); bunset(&Q, v); Q = band(Q, adj[v]);
            t++;
        }
        ub += t < s ? t : s;
    }
    return ub;
}
/* boundary peeling: v can be in R only if some connected C (v in C, C subset U, |C| <= s)
   has |N(C) cap U| <= |U| - need  (all of N(C) cap U lies outside R).  "Good" sets are
   union-closed and closed under taking supersets of the ambient set, so the iterated removal
   yields the maximal good subset, which contains every valid R.  Applied when the slack
   |U| - need is small (it is then very selective and the searches are tiny). */
static int BPMAX = 16;
static long st_bp, st_bprej;
#ifdef _OPENMP
#pragma omp threadprivate(st_bp, st_bprej)
#endif

static int bp_dfs(bs C, int cs, bs B, bs Ex, bs U, int s, int sigma) {
    int nb = bcount(&B), nex = bandcount(&B, &Ex), fr;
    st_bp++;
    if (nb <= sigma) return 1;
    if (cs >= s) return 0;
    /* every further vertex removes at most one boundary vertex; excluded ones stay */
    fr = nb - nex;
    if (nex + (fr > s - cs ? fr - (s - cs) : 0) > sigma) return 0;
    {
        bs cand = bandn(B, Ex);
        FOREACH(u, cand) {
            bs C2 = C, B2;
            bset(&C2, u);
            B2 = bor(B, band(adj[u], U)); B2 = bandn(B2, C2);
            if (bp_dfs(C2, cs + 1, B2, Ex, U, s, sigma)) return 1;
            bset(&Ex, u);
            if (bandcount(&B, &Ex) > sigma) return 0;
        }
    }
    return 0;
}
static int comp_ok(int v, bs U, int s, int sigma) {
    bs C, B, Ex;
    if (bandcount(&adj[v], &U) <= sigma) return 1;
    bclr(&C); bset(&C, v); bclr(&Ex);
    B = band(adj[v], U);
    return bp_dfs(C, 1, B, Ex, U, s, sigma);
}
static bs bpeel(bs U, int s, int need) {
    for (;;) {
        int cnt = bcount(&U), any = 0;
        if (cnt < need) return U;
        FOREACH(v, U) {
            if (btest(&U, v) && !comp_ok(v, U, s, cnt - need)) {
                bunset(&U, v); cnt--; any = 1;
                if (cnt < need) return U;
            }
        }
        if (!any) return U;
    }
}

/* greedy packing of disjoint connected t-sets: each needs a removed vertex */
static int packbound(bs U, int t) {
    int cnt = 0;
    while (bcount(&U) >= t) {
        int v = bfirst(&U), sz = 1;
        bs S, fr;
        bclr(&S); bset(&S, v);
        fr = band(adj[v], U);
        while (sz < t && !bempty(&fr)) {
            int u = bfirst(&fr);
            bset(&S, u); sz++;
            fr = bor(fr, adj[u]); fr = band(fr, U); fr = bandn(fr, S);
        }
        if (sz == t) cnt++;
        U = bandn(U, S);
    }
    return cnt;
}

/* vertices v of S with |N[v] cap Rg| <= t  (unary bit-plane counters) */
static bs lowloss(bs S, bs Rg, int t) {
    bs P[34];
    int j, done = 0;
    if (t > 32) return S;
    for (j = 0; j <= t + 1; j++) bclr(&P[j]);
    P[0] = S;
    FOREACH(r, Rg) {
        bs x = band(nadj[r], S);
        int top = done + 1 < t + 1 ? done + 1 : t + 1;
        for (j = top; j >= 1; j--) P[j] = bor(P[j], band(P[j - 1], x));
        done++;
    }
    return bandn(S, P[t + 1]);
}

/* ------------------------------------------------------------------ deletion search (small slack) */
/* exists X subset U \ Prot, |X| <= sigma, such that every comp of G[U \ X] has <= s vertices
   (and, if ranchor >= 0, some comp has exactly s vertices and min vertex > ranchor) ?
   Branching on a connected (s+1)-set T: some vertex of T must be deleted; the i-th branch
   deletes t_i and protects t_1..t_{i-1} (no duplicates).  Solution in SOL. */
static int SIGMAX = -1;
static long st_del;
static bs SOL;
#ifdef _OPENMP
#pragma omp threadprivate(st_del, SOL)
#endif

static int del_rec(bs U, bs Prot, int s, int sigma, int ranchor) {
    bs r = U, big;
    int found = 0, okc2 = ranchor < 0;
    st_del++;
    while (!bempty(&r)) {
        int v = bfirst(&r), sz;
        bs c = compof(v, r);
        sz = bcount(&c);
        if (sz > s) { big = c; found = 1; break; }
        if (sz == s && v > ranchor) okc2 = 1;
        r = bandn(r, c);
    }
    if (!found) { if (okc2) { SOL = U; return 1; } return 0; }
    if (sigma == 0) return 0;
    if (packbound(U, s + 1) > sigma) return 0;
    {   /* T: connected (s+1)-subset of big, grown from a protected vertex if possible */
        bs T, fr, pb = band(big, Prot);
        int v = bempty(&pb) ? bfirst(&big) : bfirst(&pb), sz = 1;
        bclr(&T); bset(&T, v);
        fr = band(adj[v], big);
        while (sz < s + 1) {
            /* prefer protected neighbours (fewer branches) */
            bs fp = band(fr, Prot);
            int u = bempty(&fp) ? bfirst(&fr) : bfirst(&fp);
            bset(&T, u); sz++;
            fr = bor(fr, adj[u]); fr = band(fr, big); fr = bandn(fr, T);
        }
        {
            bs cand = bandn(T, Prot);
            FOREACH(x, cand) {
                bs U2 = U;
                bunset(&U2, x);
                if (del_rec(U2, Prot, s, sigma - 1, ranchor)) return 1;
                bset(&Prot, x);
            }
        }
    }
    return 0;
}
static int del_search(bs U, int s, int need, int ranchor) {
    bs Prot;
    bclr(&Prot);
    return del_rec(U, Prot, s, bcount(&U) - need, ranchor);
}

/* ------------------------------------------------------------------ rest search */
/* exists R subset U, all comps of G[R] <= s, |R| >= need ?  (solution in SOL) */
static int rest(bs U, int s, int need);

/* building the component C (contains the seed); Fr = undecided frontier, NC = C u N(C) cap U */
static int comp_enum(bs C, int cs, bs Fr, bs U, bs NC, int s, int need) {
    bs Rg = bandn(U, NC);
    int rg = bcount(&Rg);
    st_comp++;
    if (cs < s && !bempty(&Fr)) {
        int cb, fr;
        if (cs + (s - cs) + rg < need) return 0;
        cb = cliquebound(Rg, s);
        if (cs + (s - cs) + cb < need) return 0;
        {   /* frontier filter: x joins C => other comps live in Rg \ N(x) */
            bs rm; int any = 0;
            bclr(&rm);
            int t = s + rg - need;
            FOREACH(x, Fr) { if (gdeg[x] > t && bandcount(&Rg, &adj[x]) > t) { bset(&rm, x); any = 1; } }
            if (any) { Fr = bandn(Fr, rm); U = bandn(U, rm); NC = bandn(NC, rm); }
        }
        fr = bcount(&Fr);
        while (fr > 0) {
            int u, add = s - cs < fr ? s - cs : fr;
            bs C2, au, Fr2, NC2;
            if (cs + add + cb < need) return 0;
            u = bfirst(&Fr);
            C2 = C; bset(&C2, u);
            au = band(adj[u], U);
            Fr2 = bor(Fr, au); Fr2 = bandn(Fr2, C2);
            NC2 = bor(NC, au);
            if (comp_enum(C2, cs + 1, Fr2, U, NC2, s, need)) return 1;
            bunset(&Fr, u); bunset(&U, u); bunset(&NC, u); fr--;
        }
    }
    if (cs + rg < need) return 0;
    if (rest(Rg, s, need - cs)) { SOL = bor(SOL, C); return 1; }
    return 0;
}
static int rest_pp(bs U, int s, int need, int prepeeled) {
    st_rest++;
    if (need <= 0) { bclr(&SOL); return 1; }
    if (!prepeeled) U = peel(U, s, need);
    if (bcount(&U) < need) return 0;
    if (bcount(&U) - need <= BPMAX) { U = bpeel(U, s, need); if (bcount(&U) < need) { st_bprej++; return 0; } }
    if (bcount(&U) - need <= SIGMAX) return del_search(U, s, need, -1);
    if (s == 1) { if (mis_dec(U, need)) { SOL = mis_sol; return 1; } return 0; }
    if (bcount(&U) - packbound(U, s + 1) < need) return 0;
    if (cliquebound(U, s) < need) return 0;
    for (;;) {
        int v = -1, bd = -1;
        bs C, Fr, NC;
        FOREACH(x, U) { int d = bandcount(&adj[x], &U); if (d > bd) { bd = d; v = x; } }
        bclr(&C); bset(&C, v);
        Fr = band(adj[v], U);
        NC = bor(C, Fr);
        if (comp_enum(C, 1, Fr, U, NC, s, need)) return 1;
        bunset(&U, v);
        U = peel(U, s, need);
        if (bcount(&U) < need) return 0;
        if (cliquebound(U, s) < need) return 0;
    }
}
static int rest(bs U, int s, int need) { return rest_pp(U, s, need, 0); }

/* ------------------------------------------------------------------ C2 enumeration */
static int c2_k, c2_need; /* size of C2, need for R'' */
#ifdef _OPENMP
#pragma omp threadprivate(c2_k, c2_need)
#endif

/* C: partial C2, Fr: undecided frontier (subset Allow), Rg: region for R'' (= P \ N[C], peeled) */
static int enum_c2(bs C, int cs, bs Fr, bs Allow, bs Rg, int hint) {
    int rg;
    st_c2++;
    Rg = peelh(Rg, c2_k, c2_need, &hint);
    rg = bcount(&Rg);
    if (rg < c2_need) return 0;
    if (cs == c2_k) {
        if (rest(Rg, c2_k, c2_need)) { SOL = bor(SOL, C); return 1; }
        return 0;
    }
    {
        bs rm; int any = 0;
        bclr(&rm);
        int t = rg - c2_need;
        if (t <= 32 && (t + 1) * rg < 2 * bcount(&Fr)) {
            bs keep = lowloss(Fr, Rg, t);
            rm = bandn(Fr, keep); any = !bempty(&rm);
        } else {
            FOREACH(x, Fr) { if (gdeg[x] > t && bandcount(&Rg, &adj[x]) > t) { bset(&rm, x); any = 1; } }
        }
        if (any) { Fr = bandn(Fr, rm); Allow = bandn(Allow, rm); }
    }
    while (!bempty(&Fr)) {
        int u = bfirst(&Fr);
        bs C2 = C, au, Fr2;
        bset(&C2, u);
        au = band(adj[u], Allow);
        Fr2 = bor(Fr, au); Fr2 = bandn(Fr2, C2);
        if (enum_c2(C2, cs + 1, Fr2, Allow, bandn(Rg, nadj[u]), hint)) return 1;
        bunset(&Fr, u); bunset(&Allow, u);
    }
    return 0;
}
/* C2-form check for C1 (size k, anchor c, region Rg): R = C2 u R'' in Rg, |R| >= need */
static int slice_c2(bs P, int k, int c, int need) {
    int hint = MAXN;
    bs cand;
    if (bcount(&P) < need) return 0;
    if (bcount(&P) - packbound(P, k + 1) < need) return 0;
    if (bcount(&P) - need <= BPMAX) { P = bpeel(P, k, need); if (bcount(&P) < need) { st_bprej++; return 0; } }
    if (bcount(&P) - need <= SIGMAX) return del_search(P, k, need, c);
    c2_k = k; c2_need = need - k;
    cand = band(P, above[c]);
    FOREACH(c2, cand) {
        bs B, Allow, Fr;
        bclr(&B); bset(&B, c2);
        Allow = band(P, above[c2]);
        Fr = band(adj[c2], Allow);
        if (enum_c2(B, 1, Fr, Allow, bandn(P, nadj[c2]), hint)) return 1;
    }
    return 0;
}

/* ------------------------------------------------------------------ main search */
static int F, Y;
static bs bestR;   /* witness: K = bestR + largest component of V \ N[bestR] has f = F, maxcomp = Y */
static int anchor;
static int phase; /* 1: main pass, 2: final tie pass */
static int nchanges;
#ifdef _OPENMP
#pragma omp threadprivate(F, Y, bestR, anchor, phase, nchanges)
#endif
/* shared best (all threads): a pass is repeated until it completes without any change */
static int gF, gY, gchanged;
static bs gbestR;


static void update_from(bs R) {
    int f = bcount(&R), y = Lof(R);
    if (f > F) { F = f; Y = y; bestR = R; }
    else if (f == F && y > Y) { Y = y; bestR = R; }
    nchanges++;
#ifdef _OPENMP
#pragma omp critical(parc_upd)
#endif
    {
        if (F > gF || (F == gF && Y > gY)) { gF = F; gY = Y; gbestR = bestR; gchanged = 1; }
    }
    if (VERB) fprintf(stderr, "    found f=%d L=%d -> F=%d Y=%d (anchor %d)\n", f, y, F, Y, anchor);
}

static int process_c1_(bs P, int k);
/* need for a C1 of size k, and peeling threshold theta = need - k */
static int needk(int k) { return k <= Y ? F + 1 : F; }
/* P = region of C1, already peeled with (k, needk(k)) */
static int process_c1(bs P, int k) {
    long w0 = st_c2 + st_rest + st_comp + st_del + st_bp + st_mis;
    int r = process_c1_(P, k);
    st_work[k] += st_c2 + st_rest + st_comp + st_del + st_bp + st_mis - w0;
    return r;
}
static int process_c1_(bs P, int k) {
    st_slice[k]++;
    if (bcount(&P) < needk(k)) return 0;
    st_pass[k]++;
    if (k <= Y) {
        if (slice_c2(P, k, anchor, F + 1)) { st_hit++; update_from(SOL); return 1; }
    } else if (k == Y + 1) {
        if (rest_pp(P, k, F, 1)) { st_hit++; update_from(SOL); return 1; }
    } else {
        if (slice_c2(P, k, anchor, F)) { st_hit++; update_from(SOL); return 1; }
    }
    return 0;
}
static int tieK;
#ifdef _OPENMP
#pragma omp threadprivate(tieK)
#endif

/* enumerate connected sets A (anchor = min vertex, others in Allow), Rg = V \ N[A] */
static int c1_dfs(bs A, int a, bs Fr, bs Allow, bs Rg, int hint, bs PR) {
    int rg;
    st_c1++;
    if (aborted) return 0;
    if (TLIM > 0 && (++tick & 0x3FFF) == 0 && difftime(time(NULL), T_START) >= TLIM) { aborted = 1; return 0; }
    if (phase == 2) Rg = peelh(Rg, tieK, F, &hint);
    rg = bcount(&Rg);
    if (rg < F) return 0;
    if (a >= 2) {
        if (phase == 1) {
            /* PR = parent's region peeled for this size; peeling is monotone under subsets */
            int h2 = MAXN;
            bs P = band(PR, Rg);
            if (bcount(&P) >= needk(a)) {
                P = peelh(P, a, needk(a), &h2);
                if (process_c1(P, a)) return 1;
            } else st_slice[a]++;
        }
        else if (a == tieK) {
            st_slice[a]++;
            if (rest(Rg, a, F)) { st_hit++; update_from(SOL); return 1; }
            return 0;
        }
    }
    {
        bs rm; int any = 0;
        bclr(&rm);
        int t = rg - F;
        if (t <= 32 && (t + 1) * rg < 2 * bcount(&Fr)) {
            bs keep = lowloss(Fr, Rg, t);
            rm = bandn(Fr, keep); any = !bempty(&rm);
        } else {
            FOREACH(x, Fr) { if (gdeg[x] > t && bandcount(&Rg, &adj[x]) > t) { bset(&rm, x); any = 1; } }
        }
        if (any) { Fr = bandn(Fr, rm); Allow = bandn(Allow, rm); }
    }
    if (phase == 1 && !bempty(&Fr)) {
        int h3 = MAXN;
        PR = peelh(Rg, a + 1, needk(a + 1), &h3);
    }
    while (!bempty(&Fr)) {
        int u = bfirst(&Fr);
        bs A2 = A, au, Fr2;
        bset(&A2, u);
        au = band(adj[u], Allow);
        Fr2 = bor(Fr, au); Fr2 = bandn(Fr2, A2);
        if (c1_dfs(A2, a + 1, Fr2, Allow, bandn(Rg, adj[u]), hint, PR)) return 1;
        bunset(&Fr, u); bunset(&Allow, u);
    }
    return 0;
}
static int run_anchor(int c) {
    bs A, Fr;
    anchor = c;
    bclr(&A); bset(&A, c);
    Fr = band(adj[c], above[c]);
    return c1_dfs(A, 1, Fr, above[c], bandn(ALL, nadj[c]), MAXN, ALL);
}

/* ------------------------------------------------------------------ local search heuristic */
static uint64_t rs = 0x9E3779B97F4A7C15ULL;
static uint64_t rnd(void) { rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return rs; }

static int ls_k;
static int tot[MAXN];
/* vertices u not in K that can be added keeping comps <= ls_k */
static bs addable_set(bs K) {
    bs r = K, out, res;
    int i;
    for (i = 0; i < n; i++) tot[i] = 1;
    while (!bempty(&r)) {
        bs c = compof(bfirst(&r), r), nb;
        int s = bcount(&c);
        r = bandn(r, c);
        bclr(&nb);
        FOREACH(v, c) { nb = bor(nb, adj[v]); }
        nb = bandn(nb, K);
        FOREACH(u, nb) { tot[u] += s; }
    }
    out = bandn(ALL, K);
    bclr(&res);
    FOREACH(u, out) { if (tot[u] <= ls_k) bset(&res, u); }
    return res;
}
static void greedy_fill(bs *K) {
    for (;;) {
        bs Ad = addable_set(*K);
        int bestv = -1, bestsc = 1 << 30;
        if (bempty(&Ad)) return;
        FOREACH(u, Ad) {
            int sc = bandcount(&adj[u], &Ad) * 64 + (int)(rnd() & 63);
            if (sc < bestsc) { bestsc = sc; bestv = u; }
        }
        bset(K, bestv);
    }
}
static void ls_eval(bs K) {
    bs r = K, c1;
    int mx = 0, f, y;
    bclr(&c1);
    while (!bempty(&r)) {
        bs c = compof(bfirst(&r), r);
        int s = bcount(&c);
        if (s > mx) { mx = s; c1 = c; }
        r = bandn(r, c);
    }
    f = bcount(&K) - mx;
    if (f < F) return;
    y = Lof(bandn(K, c1));
    if (f > F || y > Y) { F = f; Y = y; bestR = bandn(K, c1); }
}
static int pick_random(bs S) {
    int cnt = bcount(&S), r, i;
    if (!cnt) return -1;
    r = (int)(rnd() % (uint64_t)cnt);
    for (i = 0; i < MW; i++) {
        int pc = POPC(S.w[i]);
        if (r < pc) {
            uint64_t x = S.w[i];
            while (r--) x &= x - 1;
            return i * 64 + CTZ(x);
        }
        r -= pc;
    }
    return -1;
}
static void ls_run(int k, long iters) {
    bs K, best;
    int cur, bsz;
    long it;
    ls_k = k;
    bclr(&K);
    greedy_fill(&K);
    ls_eval(K);
    cur = bsz = bcount(&K); best = K;
    for (it = 0; it < iters; it++) {
        bs out = bandn(ALL, K), K2, M;
        int u, s2;
        u = pick_random(out);
        if (u < 0) break;
        K2 = K; bset(&K2, u);
        M = compof(u, K2);
        if (bcount(&M) > k) {
            int target = 1 + (int)(rnd() % (uint64_t)k), sa = 1;
            bs A, bnd;
            bclr(&A); bset(&A, u);
            while (sa < target) {
                bs fr; int w;
                bclr(&fr);
                FOREACH(x, A) { fr = bor(fr, adj[x]); }
                fr = band(fr, M); fr = bandn(fr, A);
                w = pick_random(fr);
                if (w < 0) break;
                bset(&A, w); sa++;
            }
            bclr(&bnd);
            FOREACH(x, A) { bnd = bor(bnd, adj[x]); }
            bnd = band(bnd, M); bnd = bandn(bnd, A);
            K2 = bandn(K2, bnd);
        }
        greedy_fill(&K2);
        s2 = bcount(&K2);
        ls_eval(K2);
        if (s2 > bsz) { bsz = s2; best = K2; }
        if (s2 >= cur || (s2 == cur - 1 && (rnd() & 7) == 0)) { K = K2; cur = s2; }
        if (cur < bsz - 2) { K = best; cur = bsz; }
    }
}

/* ------------------------------------------------------------------ input */
static unsigned char a[MAXN][MAXN];   /* original 0/1 matrix (input labels), kept for self_check */
static int read_graph(const char *fn) {
    FILE *f = fopen(fn, "r");
    int m, cnt = 0, c, i;
    if (!f) return -1;
    if (fscanf(f, "%d %d", &n, &m) != 2) { fclose(f); return -2; }
    if (n < 0 || n > MAXN) { fclose(f); return -3; }
    memset(a, 0, sizeof a);
    while (cnt < n * n && (c = fgetc(f)) != EOF) {
        if (c == '0' || c == '1') {
            int i2 = cnt / n, j = cnt % n;
            if (c == '1' && i2 != j) { a[i2][j] = 1; a[j][i2] = 1; }
            cnt++;
        }
    }
    fclose(f);
    {   /* relabel: decreasing degree */
        int deg[MAXN], ord[MAXN], pos[MAXN], j;
        for (i = 0; i < n; i++) { deg[i] = 0; for (j = 0; j < n; j++) deg[i] += a[i][j]; ord[i] = i; }
        for (i = 1; i < n; i++) {
            int x = ord[i];
            j = i - 1;
            while (j >= 0 && deg[ord[j]] < deg[x]) { ord[j + 1] = ord[j]; j--; }
            ord[j + 1] = x;
        }
        for (i = 0; i < n; i++) { pos[ord[i]] = i; orig[i] = ord[i]; }
        for (i = 0; i < n; i++) bclr(&adj[i]);
        for (i = 0; i < n; i++)
            for (j = 0; j < n; j++)
                if (a[i][j]) bset(&adj[pos[i]], pos[j]);
    }
    NW = (n + 63) / 64; if (NW < 1) NW = 1;
    bclr(&ALL);
    for (i = 0; i < n; i++) { bset(&ALL, i); nadj[i] = adj[i]; bset(&nadj[i], i); gdeg[i] = bcount(&adj[i]); }
    for (i = 0; i < n; i++) {
        int w;
        bclr(&above[i]);
        for (w = i + 1; w < n; w++) bset(&above[i], w);
    }
    return 0;
}

static void reset_stats(void) {
    int i;
    st_mis = st_rest = st_comp = st_c1 = st_c2 = st_hit = st_del = st_bp = st_bprej = 0;
    for (i = 0; i < MAXN; i++) st_slice[i] = st_pass[i] = st_work[i] = 0;
}
static void print_stats(const char *tag, double t) {
    int i;
    if (!VERB) return;
    fprintf(stderr, "%s: F=%d Y=%d  c1=%ld c2=%ld rest=%ld comp=%ld mis=%ld hits=%ld  %.2fs\n",
            tag, F, Y, st_c1, st_c2, st_rest, st_comp, st_mis, st_hit, t);
    fprintf(stderr, "   deletion-search nodes %ld  boundary-peel dfs %ld rejects %ld\n", st_del, st_bp, st_bprej);
    fprintf(stderr, "   per C1 size k: [check] candidates / passed peel / search nodes\n");
    for (i = 2; i < MAXN; i++) if (st_slice[i])
        fprintf(stderr, "     k=%-3d [%s] %11ld %10ld %11ld\n", i,
                phase == 2 ? "tie Y+1, no C2, f>=F " : i <= Y ? "C2-form f>=F+1      " : i == Y + 1 ? "tie Y+1, no C2, f>=F " : "C2-form f>=F (tie)  ",
                st_slice[i], st_pass[i], st_work[i]);
}

/* answer + witness: K = bestR + largest component of G - N[bestR];  S = V \ K (1-based input labels) */
static int S_idx[MAXN], S_cnt = 0;   /* destroyed machines (1-based) of the printed answer */
static void print_answer(void) {
    bs N = bestR, O, rr, C, K, S;
    int mx = 0, i, cnt = 0, fk, yk, *idx = S_idx;
    FOREACH(v, bestR) { N = bor(N, adj[v]); }
    O = bandn(ALL, N); rr = O; bclr(&C);
    while (!bempty(&rr)) {
        bs c = compof(bfirst(&rr), rr);
        int s = bcount(&c);
        if (s > mx) { mx = s; C = c; }
        rr = bandn(rr, c);
    }
    K = bor(bestR, C);
    yk = maxcomp(K); fk = bcount(&K) - yk;
    if (fk != F || yk != Y) fprintf(stderr, "INTERNAL WARNING: witness f=%d y=%d differs from F=%d Y=%d\n", fk, yk, F, Y);
    S = bandn(ALL, K);
    FOREACH(v, S) { idx[cnt++] = orig[v] + 1; }
    for (i = 1; i < cnt; i++) {   /* insertion sort */
        int x = idx[i], j = i - 1;
        while (j >= 0 && idx[j] > x) { idx[j + 1] = idx[j]; j--; }
        idx[j + 1] = x;
    }
    S_cnt = cnt;
    printf("Z=%d  X=%d\n", n - F, n - F - Y);
    if (WITNESS) {   /* second line: destroyed machines, 1-based input labels (verify with tools/verify.py) */
        printf("S:");
        for (i = 0; i < cnt; i++) printf(" %d", idx[i]);
        printf("\n");
    }
}

/* independent check of the printed witness: BFS on the original matrix, G - S */
static int self_check(void) {
    static unsigned char del[MAXN], seen[MAXN];
    static int q[MAXN];
    int i, Ycalc = 0, ok;
    memset(del, 0, sizeof del); memset(seen, 0, sizeof seen);
    for (i = 0; i < S_cnt; i++) del[S_idx[i] - 1] = 1;
    for (i = 0; i < n; i++) {
        int h = 0, t = 0, sz = 0;
        if (del[i] || seen[i]) continue;
        seen[i] = 1; q[t++] = i;
        while (h < t) {
            int u = q[h++], w;
            sz++;
            for (w = 0; w < n; w++) if (a[u][w] && !del[w] && !seen[w]) { seen[w] = 1; q[t++] = w; }
        }
        if (sz > Ycalc) Ycalc = sz;
    }
    ok = (S_cnt == n - F - Y) && (S_cnt + Ycalc == n - F);
    fprintf(stderr, "Kontrol: |S|=%d, kalan en buyuk bilesen Y=%d, X+Y=%d -> %s\n",
            S_cnt, Ycalc, S_cnt + Ycalc, ok ? "UYGUN (tutarli)" : "HATA");
    return ok;
}

static int is_number(const char *t) { if (!*t) return 0; for (; *t; t++) if (*t < '0' || *t > '9') return 0; return 1; }

int main(int argc, char **argv) {
    const char *fn = NULL;
    char fnbuf[256];
    const char *num[2]; int nnum = 0;
    int i, complete = 1, a;
    double t0, t1;
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-v")) VERB = 1;
        else if (!strcmp(argv[i], "-vv")) VERB = 2;
        else if (!strcmp(argv[i], "-w")) WITNESS = 1;
        else if (!strcmp(argv[i], "-q")) WITNESS = 0;
        else if (!strcmp(argv[i], "-t") && i + 1 < argc) TLIM = atof(argv[++i]);
        else if (!strcmp(argv[i], "-i") && i + 1 < argc) LSIT = atol(argv[++i]);
        else if (!strcmp(argv[i], "-S") && i + 1 < argc) SIGMAX = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-B") && i + 1 < argc) BPMAX = atoi(argv[++i]);
        else if (is_number(argv[i]) && nnum < 2) num[nnum++] = argv[i];
        else fn = argv[i];
    }
    if (!fn) {
        int N0, M0;
        if (nnum == 2) { N0 = atoi(num[0]); M0 = atoi(num[1]); }
        else {
            printf("N: "); fflush(stdout);
            if (scanf("%d", &N0) != 1) return 1;
            printf("M: "); fflush(stdout);
            if (scanf("%d", &M0) != 1) return 1;
        }
        sprintf(fnbuf, "%d%d.txt", N0, M0);
        fn = fnbuf;
    }
    T_START = time(NULL); W_START = wall();
    if (read_graph(fn)) { fprintf(stderr, "Dosya okunamadi: %s\n", fn); return 1; }
    t0 = cpu();
    if (n == 0) { printf("Z=0  X=0\n"); if (WITNESS) printf("S:\n"); return 0; }
    for (i = 0; i < n; i++) if (bcount(&adj[i]) != n - 1) complete = 0;
    if (complete) { printf("Z=%d  X=0\n", n); if (WITNESS) printf("S:\n"); return 0; }

    /* k = 1 slice: independent sets */
    a = mis_opt(ALL);
    F = a - 1; Y = 1;
    {   /* Y from the independent set: L(I \ {v}) */
        bs I = mis_sol;
        bestR = I; bunset(&bestR, bfirst(&I));
        FOREACH(v, mis_sol) { bs R = I; int y; bunset(&R, v); y = Lof(R); if (y > Y) { Y = y; bestR = R; } }
    }
    if (VERB) fprintf(stderr, "n=%d alpha=%d (mis nodes %ld) %.2fs\n", n, a, st_mis, cpu() - t0);

    /* heuristic */
    t1 = cpu();
    {
        /* sparse/medium graphs need a good (F,Y) start; dense ones are solved fast anyway */
        long m2 = 0, iters;
        double p, w;
        for (i = 0; i < n; i++) m2 += gdeg[i];
        p = n > 1 ? (double)m2 / ((double)n * (n - 1)) : 0.0;
        w = (0.6 - p) / 0.3; if (w < 0) w = 0; if (w > 1) w = 1;
        iters = 2000 + (long)(20000 * w);
        if (LSIT >= 0) iters = LSIT;
        int k, kmax = 12, lastgood = 1;
        for (k = 2; k <= kmax && k < n; k++) {
            int F0 = F;
            ls_run(k, iters);
            if (F > F0 || Y >= k) lastgood = k;
            if (k > lastgood + 3) break;
        }
    }
    if (VERB) fprintf(stderr, "heuristic: F=%d Y=%d  %.2fs\n", F, Y, cpu() - t1);

    /* exact phase.  Every connected set C1 (anchor = its min vertex) is decided by run_anchor
       under the current (F, Y).  Anchors are independent, so they are distributed over the
       threads (OpenMP, if compiled with -fopenmp).  A pass in which (F, Y) never changed has
       decided every anchor under one fixed (F, Y): that is exactly the sequential proof.  If a
       pass changes (F, Y) anywhere, the whole pass is repeated with the new values. */
    gF = F; gY = Y; gbestR = bestR;
    for (;;) {
        int c;
        t1 = cpu(); gchanged = 0;
#ifdef _OPENMP
#pragma omp parallel
#endif
        {
            F = gF; Y = gY; bestR = gbestR; phase = 1; reset_stats();
#ifdef _OPENMP
#pragma omp for schedule(dynamic, 1)
#endif
            for (c = 0; c < n; c++) {
                if (aborted) continue;
                while (run_anchor(c) && !aborted) { }   /* local (F,Y) changed: redo this anchor */
            }
        }
        if (VERB) fprintf(stderr, "pass done: F=%d Y=%d changed=%d %.2fs (cpu, all threads)\n", gF, gY, gchanged, cpu() - t1);
        print_stats("pass (master thread's share)", cpu() - t1);
        if (!gchanged || aborted) break;
    }
    F = gF; Y = gY; bestR = gbestR;
    if (VERB) fprintf(stderr, "total %.2fs\n", cpu() - t0);
    print_answer();
    fflush(stdout);
    if (!self_check()) return 2;
    if (aborted) fprintf(stderr, "ZAMAN SINIRI (-t %g sn) doldu: yazilan cozum gecerli ama optimalligi ISPATLANMADI\n", TLIM);
    else fprintf(stderr, "Optimallik ispatlandi.\n");
#ifdef _OPENMP
    fprintf(stderr, "Sure: %.2f sn (%d is parcacigi)\n", wall() - W_START, omp_get_max_threads());
#else
    fprintf(stderr, "Sure: %.2f sn\n", wall() - W_START);
#endif
    return aborted ? 3 : 0;
}
