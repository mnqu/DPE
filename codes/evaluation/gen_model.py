import sys
import os

fi1 = open(sys.argv[1], 'r')
fi2 = open(sys.argv[2], 'r')
fo = open(sys.argv[3], 'w')

dims = int(fi1.readline().strip().split()[1])

fo.write('solver_type L2R_LR' + '\n')
fo.write('nr_class 2' + '\n')
fo.write('label 0 1' + '\n')
fo.write('nr_feature ' + str(dims) + '\n')
fo.write('bias -0.00001' + '\n')
fo.write('w' + '\n')

line_a = fi1.readline()
lst_a = line_a.strip().split()

line_b = fi1.readline()
lst_b = line_b.strip().split()

for i in range(1, len(lst_a)):
    #fo.write(str(float(lst_a[i]) - float(lst_b[i])) + '\n')
    fo.write(str(float(lst_a[i])) + '\n')
fi1.close()
fi2.close()
fo.close()