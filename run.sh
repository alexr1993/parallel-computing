p_start=1
p_end=4

for p in `seq $p_start $p_end`;
do
    (time ./re -n3 -p3 -fmatrices/L500) 2>&1 > /dev/null \
        | grep user \
        | awk '{ print $2 }' \
        | sed "s/^[0-9]m//" \
        | xargs printf "$p\t%s\n" >> out
done
