import sys
import os

syn_file = sys.argv[1]
pair_file = sys.argv[2]
out_file = sys.argv[3]

pair2lb = {}

fi = open(syn_file, 'r')
for line in fi:
	lst = line.strip().split()
	u = lst[0]
	v = lst[1]
	pair2lb[(u,v)] = 1
fi.close()

fi = open(pair_file, 'r')
fo = open(out_file, 'w')
for line in fi:
	lst = line.strip().split()
	u = lst[0]
	v = lst[1]
	lb = pair2lb.get((u,v), 0)
	fo.write(str(lb) + '\t' + u + '\t' + v + '\n')
fi.close()
fo.close()
