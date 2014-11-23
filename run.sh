n_start=0
n_end=8
dim=150
prec=5

echo "`date -u` (dimensions $dim: , prec $prec)" >> out

for n in `seq $n_start $n_end`;
do
    (time ./re -n$n -p$prec -d$dim) 2>&1 > /dev/null \
        | grep real \
        | awk '{ print $2 }' \
        | sed "s/^[0-9]m//" \
        | xargs printf "$n\t%s\n" >> out
done
