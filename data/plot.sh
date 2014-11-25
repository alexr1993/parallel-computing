file=$1

gnuplot <<- END
    set term png
    set output '$1.png'

    set title '$1'
    set xlabel "Number of Threads"
    set ylabel "Time"

    set xtic autofreq
    set ytic autofreq

    plot '$file' using 1:2 with linespoints t 'real',\
         '$file' using 1:3 with linespoints t 'user',\
         '$file' using 1:4 with linespoints t 'sys'
END
