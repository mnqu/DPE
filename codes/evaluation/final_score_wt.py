import sys
import os

embed_rst = sys.argv[1]
bow_rst = sys.argv[2]
output_file = sys.argv[3]
wt = float(sys.argv[4])

eid2score = {}
fi = open(embed_rst, 'r')
for line in fi:
	eid = line.strip().split()[0]
	ent = line.strip().split()[1]
	val = float(line.strip().split()[3])
	if eid2score.get(eid, None) == None:
		eid2score[eid] = {}
	eid2score[eid][ent] = val
fi.close()

fi = open(bow_rst, 'r')
for line in fi:
	eid = line.strip().split()[0]
	val = float(line.strip().split()[2])
	ent = line.strip().split()[3]
	eid2score[eid][ent] += wt * val
fi.close()

fo = open(output_file, 'w')
for eid, dic in eid2score.items():
	dic = sorted(dic.items(), reverse = True, key = lambda x:x[1])
	for ent, val in dic:
		fo.write(eid + '\t' + ent + '\t' + str(val) + '\n')
fo.close()

