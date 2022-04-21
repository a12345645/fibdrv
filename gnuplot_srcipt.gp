reset
set terminal png font " Times_New_Roman,12 "
set output "statistic.png"

plot \
"out" using 2:4 with linespoints linewidth 2 title "BigN"