import sys
import os
import json

ent2dic = {}
fi = open(sys.argv[1], 'r')
fo = open(sys.argv[2], 'w')
lineid = 0
for line in fi:
	if lineid % 100000 == 0:
		print lineid

	lst = line.strip().split()
	for k in range(len(lst)):
		#if lst[k].find('||') == -1:
		#	continue
		ent = lst[k]
		if ent2dic.get(ent, None) == None:
			ent2dic[ent] = {}
		ent2dic[ent][lineid] = k

	lineid += 1

for ent, dic in ent2dic.items():
	fo.write(ent)
	for lid, pst in dic.items():
		fo.write(' ' + str(lid) + ':' + str(pst))
	fo.write('\n')

fi.close()
fo.close()
