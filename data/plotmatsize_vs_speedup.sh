gnuplot <<- END
    set term png
    set output 'matsize_vs_speedup.png'

    set title 'Matrix Size vs Speedup'
    set xlabel "Matrix Size"
    set ylabel "Speedup"


    plot '1threads.dat' using 1:2 with lines t '1 Thread', \
         '2threads.dat' using 1:2 with lines t '2 Threads', \
         '4threads.dat' using 1:2 with lines t '4 Threads', \
         '8threads.dat' using 1:2 with lines t '8 Threads', \
         '16threads.dat' using 1:2 with lines t '16 Threads'

END
