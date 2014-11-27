for n in `seq 1 16`;
do
    for d in `seq 100 100 600`;
    do
        # Fetch sequential speed
        avg=$(head matavg/${n}threads.dat -n1 | cut -d ' ' -f2)
        low=$(head matavg/${n}threads.dat -n1 | cut -d ' ' -f3)
        high=$(head matavg/${n}threads.dat -n1 | cut -d ' ' -f4)
        echo "avg: $avg, low: $low, high: $high"
        ./speedup.sh $n $d $avg $low $high
    done
done

