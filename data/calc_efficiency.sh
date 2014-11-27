for n in `seq 1 16`;
do
    for d in `seq 100 100 600`;
    do
        # Fetch sequential speed
        avg=$(head threadavg/${d}dim.dat -n1 | cut -d ' ' -f2)
        low=$(head threadavg/${d}dim.dat -n1 | cut -d ' ' -f3)
        high=$(head threadavg/${d}dim.dat -n1 | cut -d ' ' -f4)
        echo "avg: $avg, low: $low, high: $high"
        ./efficiency.sh $n $d $avg $low $high
    done
done

