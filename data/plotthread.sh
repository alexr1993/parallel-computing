file=$1

gnuplot <<- END
    set term png
    set output '$1.png'

    set title '$1'
    set xlabel "Number of Threads"
    set ylabel "Wall Time"


    plot '$file' using 1:2:3:4 with errorbars t 'range', \
        '$file' using 1:2 with lines t 'median'
END

#         '$file' using 1:3 with linespoints t 'user',\
#         '$file' using 1:4 with linespoints t 'sys'
#END
