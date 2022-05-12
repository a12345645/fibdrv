reset

set xlabel 'F(n)'
set ylabel 'time (ns)'
set title 'Fibonacci runtime'
set terminal png font " Times_New_Roman,12 "
set output "statistic.png"

plot \
"out_nn" using 1:2 with linespoints linewidth 2 title "bn",\
"out_cc" using 1:2 with linespoints linewidth 2 title "bn fast doubling"