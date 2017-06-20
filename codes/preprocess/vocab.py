import sys
import os

fi = open(sys.argv[1], 'r')
fo = open(sys.argv[2], 'w')
word2flag = {}
for line in fi:
	lst = line.strip().split()
	u = lst[0]
	v = lst[1]
	word2flag[u] = 1
	word2flag[v] = 1
for word in word2flag.keys():
	fo.write(word + '\n')
fi.close()
fo.close()
