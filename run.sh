n_all=(2 4 8)
default_d=15
default_p=0.0001

d=${1:-$default_d}
p=${2:-$default_p}

make

#output_tsv="data/${d}dim${p}prec"
echo "`date -u` (dimensions $d, prec $p)" >> out

# output is in format real  user    sys
for n in ${n_all[@]};
do
    (time mpirun -np $n ./re -p$p -d$d) 2>&1 >>stdout \
        | tr "\\n" "\t" \
        | sed "s/^\t/$n\t/" \
        | sed "s/\t$/\n/" \
        | sed "s/\s\w*\s/\t/g" \
        | sed "s/s//g" \
        | xargs echo >>out
done
#        | sed "s/[0-9]m//g" \
