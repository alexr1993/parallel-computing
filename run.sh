n_start=1
n_end=2
dim=50
prec=0.001

echo "`date -u` (dimensions $dim, prec $prec)" >> out

for n in `seq $n_start $n_end`;
do
    (time ./re -n$n -p$prec -d$dim) 2>&1 > stdout \
        | tr "\\n" "\t" \
        | sed "s/^\t/$n\t/" \
        | sed "s/\t$/\n/" \
        | sed "s/m//g" \
        | xargs echo >>out
done
