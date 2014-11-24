n_start=1
n_end=16
dim=250
prec=5

echo "`date -u` (dimensions $dim: , prec $prec)" >> out

for n in `seq $n_start $n_end`;
do
    (time ./re -n$n -p$prec -d$dim) 2>&1 > /dev/null \
        | tr "\\n" "\t" \
        | sed "s/^\t//" \
        | sed "s/\t$//" \
        | sed "s/m//" \
        | xargs printf "$i\t%s\n" >> out
done
