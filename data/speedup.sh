# for n in `seq 1 16`; do for d in `seq 100 100 600`; do
#./speedup.sh $n $d; done; done

nthreads=$1
dim=$2
seqtime_avg=$3
seqtime_low=$4
seqtime_high=$5
# every nthreads x dim permuation should have 5 results
nresults=5

# fetch time outputs for all runs on $nthreads with $dim
times=`grep -r "^$nthreads " . \
    | grep "[0-9]/${dim}dim" \
    | sed "s/.*\:[0-9]*.//g" \
    | sed "s/ .*//g" \
    | sort -n\
    | xargs echo`


times_arr=($times)
out_file="matavg/${nthreads}threads.dat"

if [[ ${#times_arr[@]} -ne $nresults ]]; then
    echo "More than 5 results found: $times"
    echo "Exiting"
    exit
fi

echo "(n: $nthreads, d: $dim) $times"

# Speedup = time in sequential / time in parallel
avg=$(bc<<< "scale = 10; $seqtime_avg / ${times_arr[2]}")
low=$(bc <<< "scale = 10; $seqtime_low / ${times_arr[0]}")
high=$(bc <<< "scale = 10; $seqtime_high / ${times_arr[4]}")

echo "(low: $low avg: $avg high: $high)"
echo "Ran on $nthreads threads, storing in $out_file"
#echo "$dim $avg $low $high" >>$out_file
