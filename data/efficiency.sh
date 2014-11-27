# This script is for translating time based results to efficiency ones


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
out_file="threadsvsefficiency/${dim}dim.dat"

if [[ ${#times_arr[@]} -ne $nresults ]]; then
    echo "More than 5 results found: $times"
    echo "Exiting"
    exit
fi

echo "(n: $nthreads, d: $dim) $times"

# Speedup = time in sequential / time in parallel
avg=$(bc<<< "scale = 10; $seqtime_avg / (${times_arr[2]} * $nthreads)")
low=$(bc <<< "scale = 10; $seqtime_low / (${times_arr[0]} * $nthreads)")
high=$(bc <<< "scale = 10; $seqtime_high / (${times_arr[4]} * $nthreads) ")

# Write the relevant info to outfile
echo "(low: $low avg: $avg high: $high)"
echo "Ran on $nthreads threads, storing in $out_file"
echo "$nthreads $avg $low $high" >>$out_file
