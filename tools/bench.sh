#!/bin/bash
# Usage: bench.sh SOLVER [TIMEOUT_SEC=600] [FILES...]   (default: all data/bench/*.txt sorted by n, pct)
# Prints: file  result  seconds
S=$1; T=${2:-600}; shift 2
FILES=("$@"); [ ${#FILES[@]} -eq 0 ] && FILES=($(ls $(dirname $0)/../data/bench/*.txt | sort -V))
for f in "${FILES[@]}"; do
  st=$(date +%s.%N)
  out=$(timeout $T "$S" "$f" 2>/dev/null | grep -o 'Z=[0-9]* *X=[0-9]*' | tr -s ' ')
  en=$(date +%s.%N)
  printf "%s\t%s\t%.2f\n" "$(basename $f .txt)" "${out:-TIMEOUT}" "$(echo "$en - $st" | bc)"
done
