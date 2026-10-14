#!/bin/bash
# run_bench.sh SOLVER OUT_DIR [THREADS]
# Runs every bench/*.txt with SOLVER -t 600, one file at a time, and appends one TSV row per
# file to OUT_DIR/ham.tsv (file Z X status solver_s wall_s). Program output goes to
# OUT_DIR/ciktilar/<file>.out|.err. Files already in ham.tsv are skipped, so a run can resume.
# Optional env: SKIP=<bash regex on the file name>, e.g. SKIP='20$' skips the 20% graphs.
# Used for results/AMD_Ryzen_7_7800X3D (Git Bash on Windows, gcc from WinLibs):
#   gcc -O2 -march=native -Wa,-muse-unaligned-vector-move -fopenmp -static final/parcalar.c -o parcalar_omp.exe
#   tools/windows/run_bench.sh ./parcalar_omp.exe results/AMD_Ryzen_7_7800X3D/16-cekirdek 16
B=$1; D=$2; THREADS=${3:-}
R=$(cd "$(dirname "$0")/../.." && pwd)
[ -n "$THREADS" ] && export OMP_NUM_THREADS=$THREADS
OUT=$D/ciktilar; TSV=$D/ham.tsv
mkdir -p "$OUT"
[ -f "$TSV" ] || printf "dosya\tZ\tX\tdurum\tsure_sn\tduvar_sn\n" > "$TSV"
for F in $(ls "$R"/bench/*.txt | sort); do
  f=$(basename "$F" .txt)
  grep -q "^$f	" "$TSV" && continue
  [ -n "$SKIP" ] && [[ $f =~ $SKIP ]] && continue
  st=$(date +%s.%N)
  "$B" "$F" -t 600 > "$OUT/$f.out" 2> "$OUT/$f.err"
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
