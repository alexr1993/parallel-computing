#!/bin/bash

d=${1:-"20"}
p=${2:-"0.0001"}
n=${3:-"4"}
times=${4:-"20"}

echo "If no suspected race conditions/transient errors are found, only the \
first and last results are printed"

echo \
"Testing $times times on a $d x $d matrix wth precision $p on $n $threads..."

# Run the program a number of times and compare the number of iterations and
# the final precision against the last run. If there has been a change then
# inform the user
for i in `seq 1 $times`;
do
    output=` ./re -n$n p$p d$d`
    n_iters=`echo $output | sed "s/.*(//" | sed "s/[^0-9]//g"`
    last_change=`echo $output | sed "s/.*change: //" | sed "s/ Rel.*//"`

    info="Iterations: $n_iters, Precision: $last_change"

    if [[ "$i" -eq 1 ]];
    then
        prev_n_iters=$n_iters
        prev_last_change=$last_change
        echo "(First result) $info"

    else
        if [[ ("$prev_n_iters" -ne "$n_iters") \
              || ("$prev_last_change" !=  $"$last_change") ]];
        then
            echo "Potential problem: $info"
        fi
    fi

    if [[ "$i" -eq "$times" ]];
    then
        echo "(Last result) $info"
    fi

    prev_n_iters=$n_iters
    prev_last_change=$last_change
done

