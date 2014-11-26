# for n in `seq 1 16`; do for d in `seq 100 100 600`; do
#./median.sh $n $d; done; done

nthreads=$1
dim=$2

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
out_file="avg/${dim}dim.dat"

if [[ ${#times_arr[@]} -ne $nresults ]]; then
    echo "More than 5 results found: $times"
    echo "Exiting"
    exit
fi

echo "(n: $nthreads, d: $dim) $times"

avg=${times_arr[2]}
low=${times_arr[0]}
high=${times_arr[4]}

echo "(low: $low avg: $avg high: $high)"
echo "$nthreads $avg $low $high" >>$out_file
