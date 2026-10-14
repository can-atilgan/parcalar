#!/bin/bash
# run_heur.sh HEUR OUT_DIR [SECONDS=10] [SEED=1] [JOBS=8]
# Runs the ILS heuristic (alternatif_cozuculer/heur) on every BENCH/*.txt (default data/bench),
# JOBS files at a time (the heuristic is single-threaded and its limit is CPU time).
# Writes OUT_DIR/ham.tsv (file Z X F Y time_to_best_s cpu_limit_s) and OUT_DIR/ciktilar/<file>.out|.err.
# Used for results/MEBtest/AMD_Ryzen_7_7800X3D/sezgisel_ILS/tek-cekirdek-10sn:
#   gcc -O2 -march=native -Wa,-muse-unaligned-vector-move -static alternatif_cozuculer/heur/heur.c -o heur.exe
#   BENCH=data/MEBtest tools/windows/run_heur.sh ./heur.exe results/MEBtest/AMD_Ryzen_7_7800X3D/sezgisel_ILS/tek-cekirdek-10sn
H=$1; D=$2; T=${3:-10}; SEED=${4:-1}; JOBS=${5:-8}
R=$(cd "$(dirname "$0")/../.." && pwd)
BENCH=${BENCH:-$R/data/bench}
OUT=$D/ciktilar; mkdir -p "$OUT"
export H T SEED OUT
ls "$BENCH"/*.txt | sort | xargs -P "$JOBS" -I{} bash -c '
  f=$(basename "{}" .txt)
  "$H" "{}" -t "$T" -s "$SEED" -v > "$OUT/$f.out" 2> "$OUT/$f.err"
  z=$(grep -o "Z=[0-9]*" "$OUT/$f.out" | cut -d= -f2); x=$(grep -o "X=[0-9]*" "$OUT/$f.out" | cut -d= -f2)
  s=$(grep -o "F=[0-9]* Ymax=[0-9]* time_to_best=[0-9.]*" "$OUT/$f.err" | tail -1)
  F=$(echo "$s" | sed -n "s/F=\([0-9]*\).*/\1/p"); Y=$(echo "$s" | sed -n "s/.*Ymax=\([0-9]*\).*/\1/p"); tb=$(echo "$s" | sed -n "s/.*time_to_best=\([0-9.]*\).*/\1/p")
  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\n" "$f" "$z" "$x" "$F" "$Y" "$tb" "$T"
' > "$D/ham.unsorted"
{ printf "dosya\tZ\tX\tF\tY\ten_iyiye_sn\tsinir_sn\n"; sort "$D/ham.unsorted"; } > "$D/ham.tsv"
rm -f "$D/ham.unsorted"
echo "ALL_DONE $(($(wc -l < "$D/ham.tsv") - 1)) files"
