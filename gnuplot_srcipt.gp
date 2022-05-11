reset

    set xlabel 'F(n)' set ylabel 'time (ns)' set title 'Fibonacci runtime' set
        terminal png font " Times_New_Roman,12 " set output "statistic.png"

    plot "out_str_num" using 1 : 2 with linespoints linewidth 2 title "str num",
    "out_sso" using 1 : 2 with linespoints linewidth 2 title "sso"