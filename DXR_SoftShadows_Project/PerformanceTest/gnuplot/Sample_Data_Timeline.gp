#Parameters

set terminal wxt size 1200, 600
file = "test1.csv"
set datafile separator ','

set title "Time Trends" font ", 16"

set xlabel "Nr of lights"
set ylabel "Time (Milliseconds)"
set xtic 1
set ytic 0.1
set grid

file = "test1.csv"


# To Visualize a single data set
plot "RTX3080.csv" using 1:2 with linespoint title "RTX 3080", "RTX3080i.csv" using 1:2 with linespoint title "RTX 2080"
