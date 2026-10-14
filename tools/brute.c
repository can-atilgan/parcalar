/* Exhaustive oracle for ParcalaR (n <= 26). Enumerates every kept set K (non-empty),
   f(K) = |K| - maxcomp(G[K]); F = max f; Z = n - F.
   Prints: Z=<z> Xmin=<min X over optimal> Xmax=<max X over optimal (Y>=1)> */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: brute file\n"); return 1; }
    FILE *f = fopen(argv[1], "r");
    if (!f) { perror("open"); return 1; }
    int n, m;
    if (fscanf(f, "%d %d", &n, &m) != 2) return 1;
    if (n > 26) { fprintf(stderr, "n too large\n"); return 1; }
    uint32_t adj[32] = {0};
    int cnt = 0, c;
    while (cnt < n * n && (c = fgetc(f)) != EOF) {
        if (c == '0' || c == '1') {
            int i = cnt / n, j = cnt % n;
            if (c == '1' && i != j) { adj[i] |= 1u << j; adj[j] |= 1u << i; }
            cnt++;
        }
    }
    fclose(f);
    int bestF = -1, Ymin = 0, Ymax = 0;
    uint64_t lim = 1ull << n;
    for (uint64_t KK = 1; KK < lim; KK++) {
        uint32_t K = (uint32_t)KK, rem = K;
        int maxc = 0;
        while (rem) {
            uint32_t comp = rem & (~rem + 1), fr = comp;
            while (fr) {
                int v = __builtin_ctz(fr);
                fr &= fr - 1;
                uint32_t nb = adj[v] & rem & ~comp;
                comp |= nb; fr |= nb;
            }
            rem &= ~comp;
            int s = __builtin_popcount(comp);
            if (s > maxc) maxc = s;
        }
        int fv = __builtin_popcount(K) - maxc;
        if (fv > bestF) { bestF = fv; Ymin = Ymax = maxc; }
        else if (fv == bestF) { if (maxc < Ymin) Ymin = maxc; if (maxc > Ymax) Ymax = maxc; }
    }
    int Z = n - bestF;
    printf("Z=%d Xmin=%d Xmax=%d\n", Z, Z - Ymax, Z - Ymin);
    return 0;
}
