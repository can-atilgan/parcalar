#!/bin/bash
# run_bench.sh SOLVER OUT_DIR [THREADS]
# Runs every data/bench/*.txt (or BENCH/*.txt) with SOLVER -t 600, one file at a time, and appends one TSV row per
# file to OUT_DIR/ham.tsv (file Z X status solver_s wall_s). Program output goes to
# OUT_DIR/ciktilar/<file>.out|.err. Files already in ham.tsv are skipped, so a run can resume.
# Optional env: SKIP=<bash regex on the file name>, e.g. SKIP='20$' skips the 20% graphs;
#               BENCH=<graph folder> (default: data/bench/); TLIM=<seconds> (default 600);
#               PAUSE=<seconds> to wait between files, e.g. to let the CPU cool down (default 0).
# Used for results/<data set>/AMD_Ryzen_7_7800X3D/parcalar/16-cekirdek (Git Bash on Windows, gcc from WinLibs):
#   gcc -O2 -march=native -Wa,-muse-unaligned-vector-move -fopenmp -static final/parcalar.c -o parcalar_omp.exe
#   tools/windows/run_bench.sh ./parcalar_omp.exe results/bench/AMD_Ryzen_7_7800X3D/parcalar/16-cekirdek 16
#   BENCH=data/MEBtest tools/windows/run_bench.sh ./parcalar_omp.exe results/MEBtest/AMD_Ryzen_7_7800X3D/parcalar/16-cekirdek 16
B=$1; D=$2; THREADS=${3:-}
R=$(cd "$(dirname "$0")/../.." && pwd)
BENCH=${BENCH:-$R/data/bench}
TLIM=${TLIM:-600}
PAUSE=${PAUSE:-0}
first=1
[ -n "$THREADS" ] && export OMP_NUM_THREADS=$THREADS
OUT=$D/ciktilar; TSV=$D/ham.tsv
mkdir -p "$OUT"
[ -f "$TSV" ] || printf "dosya\tZ\tX\tdurum\tsure_sn\tduvar_sn\n" > "$TSV"
for F in $(ls "$BENCH"/*.txt | sort); do
  f=$(basename "$F" .txt)
  grep -q "^$f	" "$TSV" && continue
  [ -n "$SKIP" ] && [[ $f =~ $SKIP ]] && continue
  [ "$first" = 1 ] || [ "$PAUSE" = 0 ] || sleep "$PAUSE"; first=0
  st=$(date +%s.%N)
  "$B" "$F" -t "$TLIM" > "$OUT/$f.out" 2> "$OUT/$f.err"
  en=$(date +%s.%N)
  z=$(grep -o 'Z=[0-9]*' "$OUT/$f.out" | cut -d= -f2)
  x=$(grep -o 'X=[0-9]*' "$OUT/$f.out" | cut -d= -f2)
  if grep -q 'ispatlandi' "$OUT/$f.err"; then pr=ispatli
  elif grep -q 'ISPATLANMADI' "$OUT/$f.err"; then pr=ISPATSIZ
  else pr=HATA; fi
  sv=$(grep -o 'Sure: [0-9.]*' "$OUT/$f.err" | cut -d' ' -f2)
  printf "%s\t%s\t%s\t%s\t%s\t%.2f\n" "$f" "$z" "$x" "$pr" "$sv" "$(awk "BEGIN{print $en-$st}")" | tee -a "$TSV"
done
echo ALL_DONE
