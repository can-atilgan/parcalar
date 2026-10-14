#!/bin/bash
# run_one.sh BIN FILE TLIM -> one TSV line: name Z X proven wall_s verify
B=$1; F=$2; T=$3; name=$(basename $F .txt)
tmp=$(mktemp)
st=$(date +%s.%N)
$B $F -t $T > $tmp.out 2> $tmp.err
en=$(date +%s.%N)
zx=$(grep -o 'Z=[0-9]*  X=[0-9]*' $tmp.out | tr -s ' ')
pr=$(grep -q 'ispatlandi' $tmp.err && echo proven || echo NOT_PROVEN)
vf=$(python3 $(dirname $0)/verify.py $F $tmp.out >/dev/null 2>&1 && echo feasible || echo VERIFY_FAIL)
printf "%s\t%s\t%s\t%.1f\t%s\n" "$name" "$zx" "$pr" "$(echo "$en - $st" | bc)" "$vf"
cp $tmp.out $(dirname $0)/../results/bench/eski-makine/parcalar/eski-tur/ciktilar/$name.out
rm -f $tmp $tmp.out $tmp.err
