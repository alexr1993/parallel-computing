gnuplot <<- END
    set term png
    set output 'matsize_vs_speedup.png'

    set title 'Matrix Size vs Speedup'
    set xlabel "Number of Threads"
    set ylabel "Speedup"


    plot '100dim.dat' using 1:2 with lines t '1 100 dims', \
         '200dim.dat' using 1:2 with lines t '200 dims', \
         '300dim.dat' using 1:2 with lines t '300 dims', \
         '400dim.dat' using 1:2 with lines t '400 dims', \
         '500dim.dat' using 1:2 with lines t '500 dims', \
         '600dim.dat' using 1:2 with lines t '600 dims'

END
