#Parameters

# Variables
scene = "SponzaDragons"
lights = "4"
tic = 2
tic2 = 5
mtic = 2
mtic2 = 5
sidemargin = 0.07
betweenmargin = 0.7
colwidth = 0.17

set terminal wxt size 900, 600
set datafile separator ','

# Legend
set key autotitle columnhead
set key nobox
set key horiz below
set key outside
set key at screen 0.5,0.095
set key reverse
set key maxrows 1
set key maxcols 1
set key Left

# Multiplot
#set multiplot layout 1,2 title "Average execution times".scene." - ".lights." Lights" font ",16"
set multiplot layout 1,2 title "Average execution time\n{/*0.9".scene." - ".lights." Lights}" font ",16"
set bmargin at screen 0.3

#set xlabel "Graphics card"
unset xlabel
set xtics rotate by 45 right
set grid ytics noxtics

set boxwidth 1
set style histogram gap 1
set style data histogram
set style fill solid 1 border -1

#set style boxplot nooutliers 
set style boxplot pointtype 2
set style boxplot fraction 1

#Plot 1: Dispatch()
set title "Red part" font ", 16"

set lmargin at screen sidemargin
set rmargin betweenmargin

unset y2label
unset y2tic
set ylabel "Time (Milliseconds)"
set ytic tic mirror
set mytics mtic
set yrange [0:*]

plot \
scene."_".lights."_3600_rt.csv" using 3:xtic(1) lc 3 title "Using Ray Generation shader", \
scene."_".lights."_3600_ip.csv" using 3:xtic(1) lc 6 title "Using Pixel shader", \
scene."_".lights."_3600_ic.csv" using 3:xtic(1) lc 7 title "Using Compute shader", \
\
scene."_".lights."_3600_rt_AMD Radeon RX 6900 XT_460.89.csv" using (-0.25):1:(colwidth) with boxplot lc 3 notitle fs transparent solid 0.5, \
scene."_".lights."_3600_ip_AMD Radeon RX 6900 XT_460.89.csv" using (0):1:(colwidth) with boxplot lc 6 notitle fs transparent solid 0.5, \
scene."_".lights."_3600_ic_AMD Radeon RX 6900 XT_460.89.csv" using (0.25):1:(colwidth) with boxplot lc 7 notitle fs transparent solid 0.5, \
\
scene."_".lights."_3600_rt_AMD Radeon RX 6700 XT_460.89.csv" using (0.75):1:(colwidth) with boxplot lc 3 notitle fs transparent solid 0.5, \
scene."_".lights."_3600_ip_AMD Radeon RX 6700 XT_460.89.csv" using (1):1:(colwidth) with boxplot lc 6 notitle fs transparent solid 0.5, \
scene."_".lights."_3600_ic_AMD Radeon RX 6700 XT_460.89.csv" using (1.25):1:(colwidth) with boxplot lc 7 notitle fs transparent solid 0.5, \
\
scene."_".lights."_3600_rt_NVIDIA GeForce RTX 2070_465.89.csv" using (1.75):1:(colwidth) with boxplot lc 3 notitle fs transparent solid 0.5, \
scene."_".lights."_3600_ip_NVIDIA GeForce RTX 2070_465.89.csv" using (2):1:(colwidth) with boxplot lc 6 notitle fs transparent solid 0.5, \
scene."_".lights."_3600_ic_NVIDIA GeForce RTX 2070_465.89.csv" using (2.25):1:(colwidth) with boxplot lc 7 notitle fs transparent solid 0.5, \
\
scene."_".lights."_3600_rt_NVIDIA GeForce RTX 3080_465.89.csv" using (2.75):1:(colwidth) with boxplot lc 3 notitle fs transparent solid 0.5, \
scene."_".lights."_3600_ip_NVIDIA GeForce RTX 3080_465.89.csv" using (3):1:(colwidth) with boxplot lc 6 notitle fs transparent solid 0.5, \
scene."_".lights."_3600_ic_NVIDIA GeForce RTX 3080_465.89.csv" using (3.25):1:(colwidth) with boxplot lc 7 notitle fs transparent solid 0.5, \


#Plot 2: Blue part

set title "Blue part" font ",16"
set lmargin betweenmargin
set rmargin at screen (1-sidemargin)
unset key

unset ylabel
unset ytics 
set grid ytics y2tics noxtics
set y2label "Time (Milliseconds)"
set y2tic tic2 mirror
set my2tics mtic2
set y2range [0:*]


plot \
scene."_".lights."_3600_rt.csv" using 4:xtic(1) lc 3 notitle, \
scene."_".lights."_3600_ip.csv" using 4:xtic(1) lc 6 notitle, \
scene."_".lights."_3600_ic.csv" using 4:xtic(1) lc 7 notitle, \
\
scene."_".lights."_3600_rt_AMD Radeon RX 6900 XT_460.89.csv" using (-0.25):2:(colwidth) with boxplot lc 3 notitle fs transparent solid 0.5, \
scene."_".lights."_3600_ip_AMD Radeon RX 6900 XT_460.89.csv" using (0):2:(colwidth) with boxplot lc 6 notitle fs transparent solid 0.5, \
scene."_".lights."_3600_ic_AMD Radeon RX 6900 XT_460.89.csv" using (0.25):2:(colwidth) with boxplot lc 7 notitle fs transparent solid 0.5, \
\
scene."_".lights."_3600_rt_AMD Radeon RX 6700 XT_460.89.csv" using (0.75):2:(colwidth) with boxplot lc 3 notitle fs transparent solid 0.5, \
scene."_".lights."_3600_ip_AMD Radeon RX 6700 XT_460.89.csv" using (1):2:(colwidth) with boxplot lc 6 notitle fs transparent solid 0.5, \
scene."_".lights."_3600_ic_AMD Radeon RX 6700 XT_460.89.csv" using (1.25):2:(colwidth) with boxplot lc 7 notitle fs transparent solid 0.5, \
\
scene."_".lights."_3600_rt_NVIDIA GeForce RTX 2070_465.89.csv" using (1.75):2:(colwidth) with boxplot lc 3 notitle fs transparent solid 0.5, \
scene."_".lights."_3600_ip_NVIDIA GeForce RTX 2070_465.89.csv" using (2):2:(colwidth) with boxplot lc 6 notitle fs transparent solid 0.5, \
scene."_".lights."_3600_ic_NVIDIA GeForce RTX 2070_465.89.csv" using (2.25):2:(colwidth) with boxplot lc 7 notitle fs transparent solid 0.5, \
\
scene."_".lights."_3600_rt_NVIDIA GeForce RTX 3080_465.89.csv" using (2.75):2:(colwidth) with boxplot lc 3 notitle fs transparent solid 0.5, \
scene."_".lights."_3600_ip_NVIDIA GeForce RTX 3080_465.89.csv" using (3):2:(colwidth) with boxplot lc 6 notitle fs transparent solid 0.5, \
scene."_".lights."_3600_ic_NVIDIA GeForce RTX 3080_465.89.csv" using (3.25):2:(colwidth) with boxplot lc 7 notitle fs transparent solid 0.5, \
