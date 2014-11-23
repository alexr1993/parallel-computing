n_start=0
n_end=16

file="matrices/L50"
prec=5

echo "`date -u` (file $file, prec $prec)" >> out

for n in `seq $n_start $n_end`;
do
    (time ./re -n$n -p$prec -f$file) 2>&1 > /dev/null \
        | grep user \
        | awk '{ print $2 }' \
        | sed "s/^[0-9]m//" \
        | xargs printf "$n\t%s\n" >> out
done
