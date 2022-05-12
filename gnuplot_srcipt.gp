reset

set xlabel 'F(n)'
set ylabel 'time (ns)'
set title 'Fibonacci runtime'
set terminal png font " Times_New_Roman,12 "
set output "statistic.png"

plot \
"out" using 1:2 with linespoints linewidth 2 title "str num",\
"out_bn" using 1:2 with linespoints linewidth 2 title "SSO",\
"out_nn" using 1:2 with linespoints linewidth 2 title "bn"