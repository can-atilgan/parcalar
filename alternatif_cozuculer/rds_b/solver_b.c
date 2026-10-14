/*
 * ParcalaR exact solver, variant "rds/b"  (minimum vertex integrity Z = X + Y,
 * tie rule: among optimal solutions destroy the fewest computers).
 *
 * Kept set K (K = V \ S), components s1 >= s2 >= ...,  f(K) = |K| - s1,  F = max f,
 * Z = n - F,  X = Z - Ymax,  Ymax = max{ s1(K) : f(K) = F }.
 *
 * Anchor reformulation: for a largest component C of K the rest R = K \ C lies in
 * W(C) = V \ N[C] and has components of size <= |C|; f(K) = |R|.  Hence
 *     F = max over connected C of g(C),   g(C) = keep_{|C|}( G[W(C)] ).
 * Connected sets C are enumerated with ESU (root = min vertex); a subtree is cut when
 * |W(C)| < F (W only shrinks).  "g(C) >= t" is decided exactly by a branch and bound kd().
 *
 * Canonical balanced anchors (state y = current Ymax):
 *   - |C| = y : full test g(C) >= F; if g(C) = F also test every one-vertex extension
 *               C+w for g >= F (covers optimal solutions with maxcomp y+1 and s2 <= y);
 *   - |C| != y: only balanced witnesses are needed (truncation lemma: s1 = s2), whose
 *               canonical anchor is the max-size component with the LARGEST minimum
 *               vertex; the second one, D, lies in W(C) and meets V_{<root}.  If no
 *               component of G[W(C)] meeting V_{<root} has >= |C| vertices the anchor is
 *               skipped, and if none has > |C| vertices (and |C| >= y) the subtree is cut.
 *   When (F, y) changes during the pass, repair passes (size-y anchors only, roots up to
 *   the last change) re-certify the size-y level.  Proof: NOTES_b.md.
 *
 * Usage:  solver_b FILE [-v]
 * Output: "Z=<z>  X=<x>" and "S: <destroyed computers, 1-based>"
 */
#ifndef PARCALAR_CORE
#define PARCALAR_CORE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#define MAXN 256

#ifdef __GNUC__
#define POPC(x) __builtin_popcountll(x)
#define CTZ(x) __builtin_ctzll(x)
#else
static int POPC(uint64_t x){ x=x-((x>>1)&0x5555555555555555ULL); x=(x&0x3333333333333333ULL)+((x>>2)&0x3333333333333333ULL);
    x=(x+(x>>4))&0x0F0F0F0F0F0F0F0FULL; return (int)((x*0x0101010101010101ULL)>>56); }
static int CTZ(uint64_t x){ int c=0; if(!(x&0xFFFFFFFFULL)){x>>=32;c+=32;} if(!(x&0xFFFFULL)){x>>=16;c+=16;}
    if(!(x&0xFFULL)){x>>=8;c+=8;} if(!(x&0xFULL)){x>>=4;c+=4;} if(!(x&0x3ULL)){x>>=2;c+=2;} if(!(x&0x1ULL)){c+=1;} return c; }
#endif

/* iterate over the set bits v of bitset S (S is evaluated once, copied) */
#define FOREACH(v, S, body) do { for(int _i=0;_i<NW;_i++){ uint64_t _x=(S).w[_i]; \
    while(_x){ int v=_i*64+CTZ(_x); _x&=_x-1; body } } } while(0)

/* ---------------- common state ---------------- */
static int n;
static unsigned char A[MAXN][MAXN];      /* relabelled adjacency matrix */
static int deg0[MAXN];
static int verbose = 0;
static int Fbest = -1, Ymax = 0;          /* best f found so far, largest anchor size achieving it */
static long long st_sets[MAXN+2], st_eval[MAXN+2], st_kd[MAXN+2], kdnodes;
static int opt_heur = 1;
static int opt_extend = 1, opt_order = 0;
static int heur_reps = 1, heur_kmax = 8, heur_mult = 2, heur_only = 0, heur_set = 0;
/* witness bookkeeping (relabelled vertex indices) */
static int perm_g[MAXN];                   /* relabelled a  ->  original vertex perm_g[a] (0-based) */
static int best_src = 0;                   /* 1: bestKeep holds a kept set, 2: bestAnchor holds an anchor */
static unsigned char bestKeep[MAXN], bestAnchor[MAXN], finalKeep[MAXN];
static int witness_ok = 0;
static int changed = 0, cur_root = 0, last_change_root = -1;
static double time_limit = 0;              /* -t SEC: optional CPU limit (0 = none, exact) */
static int aborted = 0;
static unsigned long tick = 0;
static long long st_ext[MAXN+2], st_skip[MAXN+2], st_cut[MAXN+2];

static double now(void){ return (double)clock()/CLOCKS_PER_SEC; }

static uint64_t rng_s = 88172645463325252ULL;
static uint64_t rnd(void){ rng_s^=rng_s<<13; rng_s^=rng_s>>7; rng_s^=rng_s<<17; return rng_s; }

/* ---------------- word-size specialised core (n <= 64*NW) ---------------- */
#define NW 1
#define SUF(x) x##_1
#include __FILE__
#undef NW
#undef SUF
#define NW 2
#define SUF(x) x##_2
#include __FILE__
#undef NW
#undef SUF
#define NW 3
#define SUF(x) x##_3
#include __FILE__
#undef NW
#undef SUF
#define NW 4
#define SUF(x) x##_4
#include __FILE__
#undef NW
#undef SUF

/* ---------------- input ---------------- */
static int read_graph(const char *path){
    FILE *f=fopen(path,"r");
    if(!f) return -1;
    int m;
    if(fscanf(f,"%d %d",&n,&m)!=2){ fclose(f); return -1; }
    if(n<0 || n>MAXN){ fclose(f); return -2; }
    static unsigned char B[MAXN][MAXN];
    memset(B,0,sizeof B);
    long cnt=0, tot=(long)n*n; int c;
    while(cnt<tot && (c=fgetc(f))!=EOF){
        if(c=='0'||c=='1'){ int i=(int)(cnt/n), j=(int)(cnt%n); if(c=='1' && i!=j){ B[i][j]=B[j][i]=1; } cnt++; }
    }
    fclose(f);
    /* relabel vertices by degree ascending */
    int d[MAXN], perm[MAXN];
    for(int i=0;i<n;i++){ d[i]=0; for(int j=0;j<n;j++) d[i]+=B[i][j]; perm[i]=i; }
    int key[MAXN];
    if(opt_order>=2) rng_s += (uint64_t)opt_order*0x9E3779B97F4A7C15ULL;
    for(int i=0;i<n;i++) key[i] = opt_order==0 ? d[i] : opt_order==1 ? -d[i] : (int)(rnd()%1000003);
    for(int a=1;a<n;a++){ int v=perm[a],b=a; while(b>0 && key[perm[b-1]]>key[v]){ perm[b]=perm[b-1]; b--; } perm[b]=v; }
    for(int a=0;a<n;a++){ perm_g[a]=perm[a]; deg0[a]=d[perm[a]]; for(int b=0;b<n;b++) A[a][b]=B[perm[a]][perm[b]]; }
    return 0;
}

static int is_uint(const char *a){ if(!*a) return 0; for(;*a;a++) if(*a<'0'||*a>'9') return 0; return 1; }

int main(int argc, char **argv){
    const char *path=NULL;
    const char *num[2]; int nnum=0;
    static char fname[64];
    for(int i=1;i<argc;i++){
        if(!strcmp(argv[i],"-v")) verbose=1;
        else if(!strcmp(argv[i],"-H0")) opt_heur=0;     /* testing only: no heuristic warm start */
        else if(!strcmp(argv[i],"-hr") && i+1<argc){ heur_reps=atoi(argv[++i]); heur_set=1; }
        else if(!strcmp(argv[i],"-hk") && i+1<argc){ heur_kmax=atoi(argv[++i]); heur_set=1; }
        else if(!strcmp(argv[i],"-hm") && i+1<argc){ heur_mult=atoi(argv[++i]); heur_set=1; }
        else if(!strcmp(argv[i],"-ho")) heur_only=1;
        else if(!strcmp(argv[i],"-t") && i+1<argc) time_limit=atof(argv[++i]);
        else if(!strcmp(argv[i],"-E0")) opt_extend=0;   /* testing only: no anchor extension */
        else if(!strcmp(argv[i],"-o") && i+1<argc) opt_order=atoi(argv[++i]); /* 0 deg asc, 1 deg desc, >=2 random */
        else if(is_uint(argv[i]) && nnum<2) num[nnum++]=argv[i];
        else path=argv[i];
    }
    if(!path){
        /* assignment convention: N=100, M(%)=20  ->  file "10020.txt" */
        long N=-1, M=-1;
        if(nnum==2){ N=atol(num[0]); M=atol(num[1]); }
        else if(nnum==0){
            fprintf(stderr,"N = "); if(scanf("%ld",&N)!=1) N=-1;
            fprintf(stderr,"M = "); if(scanf("%ld",&M)!=1) M=-1;
        }
        if(N<0 || M<0){ fprintf(stderr,"usage: %s FILE [-v] [-t SEC]   or   %s N M [-v]   (reads N M.txt, e.g. 10020.txt)\n",argv[0],argv[0]); return 1; }
        snprintf(fname,sizeof fname,"%ld%ld.txt",N,M);
        path=fname;
    }
    if(read_graph(path)<0){ fprintf(stderr,"cannot read %s\n",path); return 1; }
    if(n==0){ printf("Z=0  X=0\nS: \n"); return 0; }
    long long edges=0; for(int i=0;i<n;i++) edges+=deg0[i]; edges/=2;
    if(edges==(long long)n*(n-1)/2){            /* complete graph (incl. n=1): F=0, keep everything */
        if(verbose) fprintf(stderr,"complete graph\n");
        printf("Z=%d  X=0\nS: \n",n); return 0;
    }
    if(!heur_set){                               /* sparse graphs: spend more on the warm start */
        double dens = n>1 ? 2.0*(double)edges/((double)n*(n-1)) : 1.0;
        if(dens<0.45){ heur_reps=2; heur_kmax=12; heur_mult=10; }
    }
    if(n<=64) solve_1(); else if(n<=128) solve_2(); else if(n<=192) solve_3(); else solve_4();
    int Z=n-Fbest, X=Z-Ymax;
    printf("Z=%d  X=%d\n",Z,X);
    /* destroyed computers S = V \ K, 1-based original indices, ascending */
    static unsigned char dead[MAXN];
    for(int a=0;a<n;a++) dead[perm_g[a]] = !finalKeep[a];
    printf("S: ");
    int first=1;
    for(int v=0;v<n;v++) if(dead[v]){ printf(first?"%d":" %d",v+1); first=0; }
    printf("\n");
    if(!witness_ok) fprintf(stderr,"warning: witness check failed\n");
    if(aborted){ fprintf(stderr,"warning: time limit reached - best solution found, optimality NOT proven\n"); return 3; }
    return witness_ok?0:2;
}

#else  /* ================= core, compiled once per word count NW ================= */

typedef struct { uint64_t w[NW]; } SUF(bs);
#define BS SUF(bs)

static BS SUF(adj)[MAXN], SUF(cadj)[MAXN], SUF(GT)[MAXN], SUF(LT)[MAXN], SUF(ALL), SUF(ZERO);

static inline int SUF(cnt)(const BS *a){ int c=0; for(int i=0;i<NW;i++) c+=POPC(a->w[i]); return c; }
static inline int SUF(empty)(const BS *a){ uint64_t x=0; for(int i=0;i<NW;i++) x|=a->w[i]; return !x; }
static inline int SUF(bmin)(const BS *a){ for(int i=0;i<NW;i++) if(a->w[i]) return i*64+CTZ(a->w[i]); return -1; }
static inline void SUF(set)(BS *a,int v){ a->w[v>>6]|=1ULL<<(v&63); }
static inline void SUF(clr)(BS *a,int v){ a->w[v>>6]&=~(1ULL<<(v&63)); }
static inline BS SUF(and)(BS a,const BS *b){ for(int i=0;i<NW;i++) a.w[i]&=b->w[i]; return a; }
static inline BS SUF(or)(BS a,const BS *b){ for(int i=0;i<NW;i++) a.w[i]|=b->w[i]; return a; }
static inline BS SUF(andnot)(BS a,const BS *b){ for(int i=0;i<NW;i++) a.w[i]&=~b->w[i]; return a; }
static inline int SUF(andcnt)(const BS *a,const BS *b){ int c=0; for(int i=0;i<NW;i++) c+=POPC(a->w[i]&b->w[i]); return c; }

/* connected component of v inside W */
static inline BS SUF(comp_in)(int v, const BS *W){
    BS C=SUF(ZERO); SUF(set)(&C,v); BS fr=C;
    while(!SUF(empty)(&fr)){
        BS nb=SUF(ZERO);
        FOREACH(u, fr, { for(int i=0;i<NW;i++) nb.w[i]|=SUF(adj)[u].w[i]; });
        for(int i=0;i<NW;i++){ nb.w[i]&=W->w[i]&~C.w[i]; C.w[i]|=nb.w[i]; }
        fr=nb;
    }
    return C;
}

/* greedy packing of disjoint connected (k+1)-sets in B: each needs one deleted vertex.
   Start vertices are chosen by (static) small degree dg[]. */
static int SUF(pack_lb)(BS B, int k, const int *dg){
    int cnt=0, left=SUF(cnt)(&B);
    while(left>k){
        int v=-1, bd=1<<30;
        FOREACH(u, B, { if(dg[u]<bd){bd=dg[u];v=u;} });
        BS S=SUF(ZERO); SUF(set)(&S,v); int ss=1; BS fr=S;
        while(ss<k+1 && !SUF(empty)(&fr)){
            BS nb=SUF(ZERO); FOREACH(u, fr, { for(int i=0;i<NW;i++) nb.w[i]|=SUF(adj)[u].w[i]; });
            nb=SUF(and)(nb,&B); nb=SUF(andnot)(nb,&S);
            BS add=SUF(ZERO);
            FOREACH(u, nb, { if(ss<k+1){ SUF(set)(&add,u); ss++; } });
            S=SUF(or)(S,&add); fr=add;
        }
        if(ss==k+1) cnt++;
        B=SUF(andnot)(B,&S); left-=ss;
    }
    return cnt;
}

/* ---- kd(W,k,t): is there K subset of W, all components <= k, |K| >= t ?  (exact) ---- */
static int SUF(kd)(BS W, int k, int t);

/* grow the component C of the branching vertex. Wb: undecided vertices (C excluded),
   NC = N[C], frontier Fr = Wb & NC, rest U = Wb \ NC.  If X (|X| = m <= room) is added
   to C, the rest shrinks to U \ N[X], so the final total is at most
   cs + m + |U| - max_{x in X} kappa(x),  kappa(x) = |U & N[x]|, and max kappa over m
   distinct candidates is >= the m-th smallest kappa.  */
static int SUF(grow)(BS Wb, BS C, int cs, BS NC, int k, int t){
    kdnodes++;
    BS Fr=SUF(and)(Wb,&NC);
    BS U=SUF(andnot)(Wb,&NC);
    int fc=SUF(cnt)(&Fr), uc=SUF(cnt)(&U);
    int room = fc? k-cs : 0;
    if(room>fc+uc) room=fc+uc;
    if(cs+room+uc < t) return 0;
    if(room==0) return SUF(kd)(U,k,t-cs);
    if(cs+uc < t || room>1){
        /* kill-aware bound: histogram of kappa over candidates Fr u U */
        int hist[MAXN+2]; for(int i=0;i<=uc+1;i++) hist[i]=0;
        FOREACH(u, Fr, { hist[SUF(andcnt)(&SUF(adj)[u],&U)]++; });
        FOREACH(u, U,  { hist[SUF(andcnt)(&SUF(adj)[u],&U)+1]++; });
        int best=cs+uc, m=0;
        for(int kv=0; kv<=uc+1 && m<room; kv++){
            int h=hist[kv];
            while(h>0 && m<room){ m++; h--; int b=cs+m+uc-kv; if(b>best) best=b; }
        }
        if(best<t) return 0;
    }
    int x=-1,bd=-1;
    FOREACH(u, Fr, { int d=SUF(andcnt)(&SUF(adj)[u],&Wb); if(d>bd){bd=d;x=u;} });
    BS Wb2=Wb; SUF(clr)(&Wb2,x);
    BS C2=C; SUF(set)(&C2,x);
    BS NC2=SUF(or)(NC,&SUF(cadj)[x]);
    if(SUF(grow)(Wb2,C2,cs+1,NC2,k,t)) return 1;
    return SUF(grow)(Wb2,C,cs,NC,k,t);
}

static int SUF(kd)(BS W, int k, int t){
    kdnodes++;
    /* optional time limit: answering "no" on abort only loses solutions, never invents one */
    if(aborted) return 0;
    if(time_limit>0 && ((++tick)&4095)==0 && now()>time_limit){ aborted=1; return 0; }
    if(t<=0) return 1;
    int c=SUF(cnt)(&W);
    if(c<t) return 0;
    if(c<=k) return 1;
    int dg[MAXN];
    /* forced deletions (iterated): if at most d=c-t vertices are deleted, a kept vertex u
       has a component of size >= 1+deg_W(u)-d, so deg_W(u) >= k+d forces deletion */
    for(;;){
        int d=c-t, nf=0; BS forced=SUF(ZERO);
        FOREACH(u, W, { int x=SUF(andcnt)(&SUF(adj)[u],&W); dg[u]=x; if(x>=k+d){ SUF(set)(&forced,u); nf++; } });
        if(nf==0) break;
        if(nf>d) return 0;
        W=SUF(andnot)(W,&forced); c-=nf;
        if(c<=k) return 1;
    }
    /* connected components: small ones are kept entirely */
    BS R=W, B=SUF(ZERO); int fr=0, ub=0, nb=0;
    BS bc[MAXN/2+1]; int bub[MAXN/2+1];
    while(!SUF(empty)(&R)){
        int v=SUF(bmin)(&R);
        BS C=SUF(comp_in)(v,&R);
        R=SUF(andnot)(R,&C);
        int sz=SUF(cnt)(&C);
        if(sz<=k) fr+=sz;
        else { B=SUF(or)(B,&C); bc[nb]=C; bub[nb]=sz-SUF(pack_lb)(C,k,dg); ub+=bub[nb]; nb++; }
    }
    if(fr>=t) return 1;
    t-=fr;
    if(ub<t) return 0;
    if(nb>=2){
        /* independent components: keep(W) = sum of keep(B_j); maximise them one by one */
        int got=0, rem=ub;
        for(int j=0;j<nb;j++){
            rem-=bub[j];
            int lo=t-got-rem; if(lo<1) lo=1;
            int e=-1;
            for(int val=bub[j]; val>=lo; val--) if(SUF(kd)(bc[j],k,val)){ e=val; break; }
            if(e<0){ if(t-got-rem>0) return 0; e=0; }
            got+=e;
            if(got>=t) return 1;
        }
        return got>=t;
    }
    /* branch on a max-degree vertex w: either w is in K (enumerate its component), or not */
    int w=-1,bd=-1;
    FOREACH(u, B, { if(dg[u]>bd){bd=dg[u];w=u;} });
    BS B2=B; SUF(clr)(&B2,w);
    BS C=SUF(ZERO); SUF(set)(&C,w);
    if(SUF(grow)(B2,C,1,SUF(cadj)[w],k,t)) return 1;
    return SUF(kd)(B2,k,t);
}

/* ---------------- heuristic warm start (local search per component bound k) ---------------- */
static int SUF(maxcomp)(BS K){
    int m=0; BS R=K;
    while(!SUF(empty)(&R)){ int v=SUF(bmin)(&R); BS C=SUF(comp_in)(v,&R); R=SUF(andnot)(R,&C); int s=SUF(cnt)(&C); if(s>m)m=s; }
    return m;
}
static inline int SUF(csize_with)(int v, const BS *K){ BS KK=*K; SUF(set)(&KK,v); BS C=SUF(comp_in)(v,&KK); return SUF(cnt)(&C); }

static void SUF(consider)(BS K){
    if(SUF(empty)(&K)) return;
    int s1=SUF(maxcomp)(K), f=SUF(cnt)(&K)-s1;
    if(f<Fbest) return;
    /* grow each largest component E inside V \ N[K \ E]: same f, possibly larger s1 */
    BS R=K;
    while(!SUF(empty)(&R)){
        int v=SUF(bmin)(&R); BS E=SUF(comp_in)(v,&R); R=SUF(andnot)(R,&E);
        if(SUF(cnt)(&E)!=s1) continue;
        BS Kr=SUF(andnot)(K,&E), NR=SUF(ZERO);
        FOREACH(u, Kr, { NR=SUF(or)(NR,&SUF(cadj)[u]); });
        BS U=SUF(andnot)(SUF(ALL),&NR);
        BS X=SUF(comp_in)(v,&U);
        int e=SUF(cnt)(&X);
        if(f>Fbest || e>Ymax){
            Fbest=f; Ymax=e; best_src=1;
            BS K2=SUF(or)(Kr,&X);
            for(int a=0;a<n;a++) bestKeep[a]=(unsigned char)((K2.w[a>>6]>>(a&63))&1);
        }
    }
}

static void SUF(ls_k)(int k, int iters){
    BS K=SUF(ZERO);
    int ord[MAXN], key[MAXN];
    for(int i=0;i<n;i++){ ord[i]=i; key[i]=deg0[i]*4+(int)(rnd()%8); }
    for(int a=1;a<n;a++){ int v=ord[a],b=a; while(b>0 && key[ord[b-1]]>key[v]){ ord[b]=ord[b-1]; b--; } ord[b]=v; }
    for(int i=0;i<n;i++){ int v=ord[i]; if(SUF(csize_with)(v,&K)<=k) SUF(set)(&K,v); }
    BS best=K; int bsz=SUF(cnt)(&K);
    for(int it=0; it<iters; it++){
        BS K2=K;
        BS out=SUF(andnot)(SUF(ALL),&K2); int oc=SUF(cnt)(&out); if(!oc) break;
        int r=(int)(rnd()%(uint64_t)oc), v=-1;
        FOREACH(u, out, { if(r--==0) v=u; });
        SUF(set)(&K2,v);                              /* force v in, repair its component */
        for(;;){
            BS C=SUF(comp_in)(v,&K2); if(SUF(cnt)(&C)<=k) break;
            BS nb=SUF(and)(SUF(adj)[v],&K2);
            int m=SUF(cnt)(&nb), rr=(int)(rnd()%(uint64_t)m), x=-1;
            FOREACH(u, nb, { if(rr--==0) x=u; });
            SUF(clr)(&K2,x);
        }
        out=SUF(andnot)(SUF(ALL),&K2);                /* greedy free insertions */
        int cand[MAXN], nc=0; FOREACH(u, out, { cand[nc++]=u; });
        for(int i=nc-1;i>0;i--){ int j=(int)(rnd()%(uint64_t)(i+1)); int t=cand[i];cand[i]=cand[j];cand[j]=t; }
        for(int i=0;i<nc;i++){ int u=cand[i]; if(SUF(csize_with)(u,&K2)<=k) SUF(set)(&K2,u); }
        int s2=SUF(cnt)(&K2), s1=SUF(cnt)(&K);
        if(s2>=s1 || (rnd()%100)<2){ K=K2; if(s2>bsz){ bsz=s2; best=K2; } SUF(consider)(K2); }
    }
    SUF(consider)(best);
}

/* ---------------- exact anchor enumeration ---------------- */

/* anchor C (size s, W = V\N[C], wc=|W|): decide g(C) >= need, update F / Ymax */
static void SUF(record_anchor)(const BS *C){
    best_src=2;
    for(int a=0;a<n;a++) bestAnchor[a]=(unsigned char)((C->w[a>>6]>>(a&63))&1);
}
/* g(C) >= Fbest holds for anchor C (|C| = s, W = V \ N[C]).  Recover one optimal rest R
   (self-reduction: drop vertices while kd stays true; the minimal set is itself a valid rest
   with |R| = Fbest) and grow C to its component in G[V \ N[R]]: that is again an anchor with
   rest R, so f = Fbest with a possibly larger largest component (raises Ymax early). */
static void SUF(extend)(const BS *C, const BS *W, int s){
    if(!opt_extend) return;
    BS R=*W;
    for(int a=n-1;a>=0;a--){
        if(!((R.w[a>>6]>>(a&63))&1)) continue;
        BS R2=R; SUF(clr)(&R2,a);
        if(SUF(kd)(R2,s,Fbest)) R=R2;
    }
    BS NR=SUF(ZERO);
    FOREACH(u, R, { NR=SUF(or)(NR,&SUF(cadj)[u]); });
    BS U=SUF(andnot)(SUF(ALL),&NR);
    BS E=*C, fr=*C;
    while(!SUF(empty)(&fr)){
        BS nb=SUF(ZERO);
        FOREACH(u, fr, { for(int i=0;i<NW;i++) nb.w[i]|=SUF(adj)[u].w[i]; });
        for(int i=0;i<NW;i++){ nb.w[i]&=U.w[i]&~E.w[i]; E.w[i]|=nb.w[i]; }
        fr=nb;
    }
    int e=SUF(cnt)(&E);
    if(e>Ymax){
        Ymax=e; SUF(record_anchor)(&E);
        if(verbose) fprintf(stderr,"  extension: F=%d anchor size %d\n",Fbest,Ymax);
    }
}

/* ---- evaluation modes (see NOTES.md, "canonical balanced anchors") ----
   y = Ymax.  Size-y anchors get the full test (threshold F) and, when g(C) = F, the
   one-vertex extension test for ties at y+1.  Every other anchor only matters through a
   BALANCED witness whose canonical anchor is C: a second component D of size |C| with
   min(D) < min(C) inside W.  Such a D lies in a component of G[W] meeting V_{<root};
   maxLc = the largest such component.  Tests always use the exact kd, so a positive answer
   is always a real solution. */
static void SUF(found)(int g, int s, const BS *C, const BS *W){
    Fbest=g; Ymax=s; SUF(record_anchor)(C); changed=1; last_change_root=cur_root;
    if(verbose) fprintf(stderr,"  update: F=%d Ymax=%d\n",Fbest,Ymax);
    int y0=Ymax; SUF(extend)(C,W,s); if(Ymax!=y0){ changed=1; last_change_root=cur_root; }
}

static void SUF(eval_full)(int s, const BS *W, int wc, const BS *C){
    if(wc<Fbest) return;
    long long k0=kdnodes;
    st_eval[s]++;
    if(SUF(kd)(*W,s,Fbest)){
        int g=Fbest; while(wc>=g+1 && SUF(kd)(*W,s,g+1)) g++;
        if(g>Fbest) SUF(found)(g,s,C,W);
        else { int y0=Ymax; SUF(extend)(C,W,s); if(Ymax!=y0){ changed=1; last_change_root=cur_root; } }
        if(Ymax==s){
            /* ties at s+1 whose witness is unbalanced contain a size-s optimal anchor */
            BS ext=SUF(andnot)(SUF(ALL),W); ext=SUF(andnot)(ext,C);
            st_ext[s]++;
            int done=0;
            FOREACH(w, ext, {
                if(!done){
                    BS W2=SUF(andnot)(*W,&SUF(adj)[w]);
                    int wc2=SUF(cnt)(&W2);
                    if(wc2>=Fbest && SUF(kd)(W2,s+1,Fbest)){
                        int g2=Fbest; while(wc2>=g2+1 && SUF(kd)(W2,s+1,g2+1)) g2++;
                        BS C2=*C; SUF(set)(&C2,w);
                        SUF(found)(g2,s+1,&C2,&W2);
                        done=1;
                    }
                }
            });
        }
    }
    st_kd[s]+=kdnodes-k0;
}

static void SUF(eval_bal)(int s, const BS *W, int wc, const BS *C, int need){
    if(wc<need) return;
    long long k0=kdnodes;
    st_eval[s]++;
    if(SUF(kd)(*W,s,need)){
        int g=need; while(wc>=g+1 && SUF(kd)(*W,s,g+1)) g++;
        if(g>Fbest || (g==Fbest && s>Ymax)) SUF(found)(g,s,C,W);
    }
    st_kd[s]+=kdnodes-k0;
}

/* largest component of G[W] that meets V_{<root} */
static int SUF(maxLc)(const BS *W, int root){
    BS L=SUF(and)(*W,&SUF(LT)[root]);
    int m=0;
    while(!SUF(empty)(&L)){
        int v=SUF(bmin)(&L);
        BS Cc=SUF(comp_in)(v,W);
        int c=SUF(cnt)(&Cc); if(c>m) m=c;
        L=SUF(andnot)(L,&Cc);
    }
    return m;
}

/* candidates w in cand with |W & N(w)| <= d  (bit-sliced saturating counter, d <= 7) */
static inline BS SUF(atmost_nb)(const BS *W, const BS *cand, int d){
    BS G[8]; for(int j=0;j<=d;j++) G[j]=SUF(ZERO);
    FOREACH(u, *W, {
        for(int j=d;j>=1;j--) for(int i=0;i<NW;i++) G[j].w[i]|=G[j-1].w[i]&SUF(adj)[u].w[i];
        for(int i=0;i<NW;i++) G[0].w[i]|=SUF(adj)[u].w[i];
    });
    return SUF(andnot)(*cand,&G[d]);
}

/* ESU: current connected set C (size s, min vertex root) is represented by
   W = V \ N[C] (wc = |W|) and the extension set Vext.  The exclusive neighbours of a new
   vertex w are N(w) & W & GT[root].  repair=1: only size-Ymax anchors are evaluated. */
static void SUF(esu)(const BS *W, int wc, const BS *Vext, int s, int root, const BS *C, int repair){
    if(aborted) return;
    if(time_limit>0 && ((++tick)&1023)==0 && now()>time_limit){ aborted=1; return; }
    st_sets[s]++;
    int mL=-1;
    if(s==Ymax) SUF(eval_full)(s,W,wc,C);
    else if(!repair){
        int need = s<Ymax ? Fbest+1 : Fbest;
        if(wc>=need){ mL=SUF(maxLc)(W,root); if(mL>=s) SUF(eval_bal)(s,W,wc,C,need); else st_skip[s]++; }
    }
    if(SUF(empty)(Vext)) return;
    if(s>=Ymax){
        if(repair) return;
        if(mL<0) mL=SUF(maxLc)(W,root);
        if(mL<=s){ st_cut[s]++; return; }
    }
    int d=wc-Fbest;                             /* child w needs |W & N(w)| <= d */
    if(d<0) return;
    int vc=SUF(cnt)(Vext);
    BS good;
    if(d<=7 && (d+1)*wc*2 < vc*3) good=SUF(atmost_nb)(W,Vext,d);
    else { good=SUF(ZERO); FOREACH(w, *Vext, { if(SUF(andcnt)(W,&SUF(adj)[w])<=d) SUF(set)(&good,w); }); }
    FOREACH(w, good, {
        BS W2=SUF(andnot)(*W,&SUF(adj)[w]);
        int wc2=SUF(cnt)(&W2);
        if(wc2>=Fbest){
            BS V2=SUF(and)(*Vext,&SUF(GT)[w]);
            for(int i=0;i<NW;i++) V2.w[i]|=SUF(adj)[w].w[i]&W->w[i]&SUF(GT)[root].w[i];
            BS C2=*C; SUF(set)(&C2,w);
            SUF(esu)(&W2,wc2,&V2,s+1,root,&C2,repair);
        }
    });
}

static void SUF(pass)(int repair, int upto){
    for(int v=0; v<=upto && !aborted; v++){
        cur_root=v;
        BS W=SUF(andnot)(SUF(ALL),&SUF(cadj)[v]);
        int wc=SUF(cnt)(&W);
        if(wc<Fbest) continue;
        BS Vext=SUF(and)(SUF(adj)[v],&SUF(GT)[v]);
        BS C=SUF(ZERO); SUF(set)(&C,v);
        SUF(esu)(&W,wc,&Vext,1,v,&C,repair);
    }
}

/* Final kept set K with f(K)=Fbest and maxcomp(K)=Ymax.  For an anchor C, the rest R is
   recovered by self-reduction: drop v from W whenever kd(W-v, |C|, F) stays true; the final
   W is inclusion-minimal with kd true, hence itself a valid rest (comps <= |C|, |W| >= F). */
static void SUF(build_witness)(void){
    BS K=SUF(ZERO);
    int ab=aborted; double tl=time_limit;
    aborted=0; time_limit=0;                    /* the rebuild must run to completion */
    if(best_src==2){
        BS C=SUF(ZERO), NC=SUF(ZERO);
        for(int a=0;a<n;a++) if(bestAnchor[a]){ SUF(set)(&C,a); NC=SUF(or)(NC,&SUF(cadj)[a]); }
        int k=SUF(cnt)(&C);
        BS W=SUF(andnot)(SUF(ALL),&NC);
        if(SUF(kd)(W,k,Fbest)){
            for(int a=n-1;a>=0;a--){
                if(!((W.w[a>>6]>>(a&63))&1)) continue;
                BS W2=W; SUF(clr)(&W2,a);
                if(SUF(kd)(W2,k,Fbest)) W=W2;
            }
        }
        K=SUF(or)(C,&W);
    } else {
        for(int a=0;a<n;a++) if(bestKeep[a]) SUF(set)(&K,a);
    }
    for(int a=0;a<n;a++) finalKeep[a]=(unsigned char)((K.w[a>>6]>>(a&63))&1);
    int s1=SUF(empty)(&K)?0:SUF(maxcomp)(K);
    witness_ok = (s1==Ymax) && (SUF(cnt)(&K)-s1==Fbest);
    aborted=ab; time_limit=tl;
}

static void SUF(solve)(void){
    double t0=now();
    for(int i=0;i<NW;i++){ SUF(ZERO).w[i]=0; SUF(ALL).w[i]=0; }
    for(int a=0;a<n;a++){
        SUF(adj)[a]=SUF(ZERO); SUF(GT)[a]=SUF(ZERO); SUF(LT)[a]=SUF(ZERO);
        for(int b=0;b<n;b++) if(A[a][b]) SUF(set)(&SUF(adj)[a],b);
        for(int b=a+1;b<n;b++) SUF(set)(&SUF(GT)[a],b);
        for(int b=0;b<a;b++) SUF(set)(&SUF(LT)[a],b);
        SUF(cadj)[a]=SUF(adj)[a]; SUF(set)(&SUF(cadj)[a],a);
        SUF(set)(&SUF(ALL),a);
    }
    /* warm start */
    Fbest=-1; Ymax=0;
    { BS K=SUF(ZERO); SUF(set)(&K,0); SUF(consider)(K); }
    if(opt_heur) for(int rep=0;rep<heur_reps;rep++) for(int k=1;k<=n && k<=heur_kmax;k++) SUF(ls_k)(k, 200+heur_mult*n);
    double th=now();
    if(verbose) fprintf(stderr,"n=%d  heuristic: F>=%d (Y=%d)  %.3fs  [NW=%d]\n",n,Fbest,Ymax,th-t0,NW);
    if(heur_only){ SUF(build_witness)(); return; }
    /* exact enumeration of anchors: one full pass, then repair passes (size-Ymax anchors
       only) until a pass leaves (F, Ymax) unchanged */
    int passes=0;
    /* Roots processed after the last state change were evaluated under the final state,
       so a repair after pass 1 only needs roots 0..last_change_root; after a change inside a
       repair pass every root is redone. */
    changed=0; last_change_root=-1; SUF(pass)(0,n-1); passes++;
    if(verbose && !aborted) fprintf(stderr,"pass 1 done: F=%d Ymax=%d changed=%d (last change at root %d)  %.3fs\n",Fbest,Ymax,changed,last_change_root,now()-t0);
    int upto=last_change_root;
    while(changed && !aborted){
        changed=0; SUF(pass)(1,upto); passes++;
        if(verbose && !aborted) fprintf(stderr,"repair pass %d (roots 0..%d) done: F=%d Ymax=%d changed=%d  %.3fs\n",passes,upto,Fbest,Ymax,changed,now()-t0);
        upto=n-1;
    }
    double te=now();
    if(verbose && aborted) fprintf(stderr,"time limit: stopped in pass %d at root %d of %d; best so far F=%d Ymax=%d (not proven)\n",passes,cur_root,n,Fbest,Ymax);
    SUF(build_witness)();
    double tw=now();
    if(verbose) fprintf(stderr,"witness: %s  (%.3fs)\n",witness_ok?"verified":"FAILED",tw-te);
    if(verbose){
        long long ts=0, te2=0, tk=0;
        fprintf(stderr,"anchor size: visited / evaluated / skipped(maxLc) / cut(maxLc) / ext-tested / kd nodes\n");
        for(int s=1;s<=n;s++) if(st_sets[s]){ ts+=st_sets[s]; te2+=st_eval[s]; tk+=st_kd[s];
            fprintf(stderr,"  |C|=%-3d %12lld %12lld %12lld %12lld %10lld %13lld\n",s,st_sets[s],st_eval[s],st_skip[s],st_cut[s],st_ext[s],st_kd[s]); }
        fprintf(stderr,"  total   %12lld %12lld %12lld\n",ts,te2,tk);
        fprintf(stderr,"exact: F=%d Ymax=%d  enumeration %.3fs, total %.3fs\n",Fbest,Ymax,te-th,te-t0);
    }
}

#undef BS
#endif
