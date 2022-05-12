import csv 
import sys

def fib_fast_double_iter(n) :
    #stack construction
    tracker =[]
    while n> 1: 
        tracker.append(n) 
        n //= 2
    #initialization
    f1, f2 = 1, 1
    #bottom - up
    while tracker:
        n = tracker.pop() 
        f1, f2 = f1 *(2 * f2 - f1), f1 ** 2 + f2 ** 2 
        if n % 2 == 1: 
            f1, f2 = f2, f1 + f2 
    return f1

with open('out', newline ='') as outfile:
    csv.field_size_limit(sys.maxsize)

    out = csv.reader(outfile, delimiter = ' ')
    fib =[0, 1, 1]
    j = 2
    for i in out: 
        print(i[0], end = ' ')
# #while (j <= int(i[0]) - 1):
# #fib[j % 3] = fib[(j - 1) % 3] + fib[(j - 2) % 3]
# #j += 1
# #fib[j % 3] = fib[(j - 1) % 3] + fib[(j - 2) % 3]
        s = fib_fast_double_iter(int(i[0]))
        if (i[1] == str(s)): 
            print('OK', i[1]) 
        else: 
            print('Error', i[1], s) 
            break