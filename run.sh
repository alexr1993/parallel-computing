n_start=1
n_end=16
default_d=15
default_p=0.0001

d=${1:-$default_d}
p=${2:-$default_p}

#output_tsv="data/${d}dim${p}prec"
echo "`date -u` (dimensions $d, prec $p)" >> out

# output is in format real  user    sys
for n in `seq $n_start $n_end`;
do
    (time ./re -n$n -p$p -d$d) 2>&1 >>stdout \
        | tr "\\n" "\t" \
        | sed "s/^\t/$n\t/" \
        | sed "s/\t$/\n/" \
        | sed "s/\s\w*\s/\t/g" \
        | sed "s/[0-9]m//g" \
        | sed "s/s//g" \
        | xargs echo >>out
done
