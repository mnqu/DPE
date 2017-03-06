import sys
import os

fi1 = open(sys.argv[1], 'r')
fi2 = open(sys.argv[2], 'r')
fo = open(sys.argv[3], 'w')

fo.write('solver_type L2R_LR' + '\n')
fo.write('nr_class 2' + '\n')
fo.write('label 0 1' + '\n')
fo.write('nr_feature 100' + '\n')
fo.write('bias -1' + '\n')
fo.write('w' + '\n')

fi1.readline()
line = fi1.readline()
lst = line.strip().split()
for i in range(1, len(lst)):
        fo.write(lst[i].strip() + '\n')
fi1.close()
fi2.close()
fo.close()