# ParçalaR: exact vertex integrity solver

A research study on solving one NP-hard network problem exactly and proving that each answer is optimal.

## The problem

You are given a network of `n` computers (an undirected graph). Wipe a set `S` of computers. Let `X = |S|`, and let `Y` be the size of the largest connected part that remains. The goal is to **minimise `Z = X + Y`**. If several solutions reach the same `Z`, the one with the smallest `X` is chosen.

This quantity is the **vertex integrity** of the graph. Barefoot, Entringer and Swart introduced it in 1987 to measure how vulnerable a network is [1]. Computing it is NP-hard on general graphs [2].

## What this study does

- **An exact solver**, [`final/parcalar.c`](final/parcalar.c): a single C file using branch and bound over connected sets, with bitset bounds. Each answer comes with a proof of optimality. OpenMP parallelism is optional.
- **Four alternative exact solvers and a heuristic**, in [`alternatif_cozuculer/`](alternatif_cozuculer/). They were used for comparison and cross-checking, and each has its own notes.
- **Independent checkers**, in [`tools/`](tools/):
  - a feasibility verifier (`verify.py`),
  - a brute-force oracle for small graphs (`brute.c`),
  - a CP-SAT model (`cpsat.py`),
  - a random graph generator (`gen.py`).
- **Benchmarks** on 88 random graphs (`n` = 100–200, edge density 20–90%), in [`bench/`](bench/).
- **An interactive visualisation** of the search, serial and parallel, in Turkish and English, in [`viz/`](viz/). [Open it online](https://claude.ai/artifact/6hF2Nt3NyyTehcUAT8uChy), or open `viz/dist/index.html` locally.

## Results

| Run | Machine | Proven optimal | Table |
|---|---|---|---|
| Original | low-power machine, 1 core, 120 s limit (some long runs on 4 cores) | 67 / 88 | [`results/SONUCLAR.md`](results/SONUCLAR.md) |
| Current | AMD Ryzen 7 7800X3D, 16 threads, 600 s limit | 74 / 74 run (14 sparsest not run yet) | [`results/AMD_Ryzen_7_7800X3D/16-cekirdek/SONUCLAR.md`](results/AMD_Ryzen_7_7800X3D/16-cekirdek/SONUCLAR.md) |

The current run used GCC 16.2.0 (WinLibs, MinGW-w64) with this command:

```
gcc -O2 -march=native -Wa,-muse-unaligned-vector-move -fopenmp -static final/parcalar.c -o parcalar
```

- `-march=native` is about 1.6× faster than plain `-O2` on a single core. Most of the gain comes from POPCNT/BMI.
- `-Wa,-muse-unaligned-vector-move` works around a MinGW AVX stack-alignment crash (GCC bug 54412).

Sparse graphs (20–30% density) are the hard cases.

## Build and run

```
gcc -O2 -march=native parcalar.c -o parcalar              # single thread
gcc -O2 -march=native -fopenmp parcalar.c -o parcalar     # all cores
./parcalar 10020.txt            # or: ./parcalar 100 20   -> reads 10020.txt
./parcalar 10020.txt -t 600     # time limit: prints the best valid answer, flagged as unproven
python3 tools/verify.py bench/10020.txt output.txt         # independent feasibility check
```

The output is `Z=<z>  X=<x>`, followed by `S: ...` (the wiped computers, numbered from 1). The input format is `n m` on the first line, then the `n × n` 0/1 adjacency matrix. See [`final/README.md`](final/README.md) for details (in Turkish) and [`small/example.txt`](small/example.txt) for the 7-node example (answer `Z=4 X=1`, `S: 7`).

## Repository layout

| Path | Contents |
|---|---|
| `final/` | the exact solver |
| `alternatif_cozuculer/` | alternative solvers (`bnb`, `rds`, …) and a heuristic, each with NOTES |
| `bench/` | 88 random test graphs `{n}{density}.txt`, generated for this study |
| `results/` | result tables, raw timings and every program output |
| `tools/` | verifier, brute force, CP-SAT model, generator; `tools/windows/` has the Windows benchmark scripts |
| `viz/` | interactive page: `page.html` + `sim.js`, built with `node viz/build.js` into `viz/dist/index.html` |

Most notes and file names are in Turkish.

## References

1. **Original definition.** C. A. Barefoot, R. C. Entringer, H. C. Swart. *Vulnerability in graphs: a comparative survey.* J. Combin. Math. Combin. Comput. 1 (1987) 13–22.
2. **NP-completeness** (also on planar graphs). L. H. Clark, R. C. Entringer, M. R. Fellows. *Computational complexity of integrity.* J. Combin. Math. Combin. Comput. 2 (1987) 179–191.
3. **Basic bounds.** W. Goddard, H. C. Swart. *Integrity in graphs: bounds and basics.* J. Combin. Math. Combin. Comput. 7 (1990) 139–151.
4. **Survey.** K. S. Bagga, L. W. Beineke, W. D. Goddard, M. J. Lipman, R. E. Pippert. *A survey of integrity.* Discrete Applied Mathematics 37/38 (1992) 13–28.
5. **Parameterised complexity** (O(p^(p+1)·n) algorithm, p³ kernel). P. G. Drange, M. S. Dregi, P. van 't Hof. *On the computational complexity of vertex integrity and component order connectivity.* Algorithmica (2016). [arXiv:1403.6331](https://arxiv.org/abs/1403.6331)
6. **Exact exponential algorithm** (O(1.9602^n), theoretical). S. Das, S. Das, S. S. Islam, R. M. Mitra, A. Mohapatra, A. Paul. *Solving vertex integrity faster than 2^n.* arXiv (2026). [arXiv:2609.33779](https://arxiv.org/abs/2609.33779)

The solver's reformulation `Z = n − max(|K| − largest component)` matches the one used in [6], which credits [3]. The search itself (start-vertex enumeration of the largest component, region bounds, passes repeated until nothing changes) is this study's own design. It was not taken from these papers.
