import csv

with open('fib_500.txt', newline='') as verifyfile, open('out', newline='') as outfile:

    answer = csv.reader(verifyfile, delimiter=' ')
    out = csv.reader(outfile, delimiter=' ')

    for i, j in zip(answer, out):
        print(i[0], end=' ')
        if(i[1] == j[1]) :
            print('OK', i[1])
        else:
            print('Error', i[1], j[1])