import sys
import os

pred_file = sys.argv[1]
pair_file = sys.argv[2]
out_file = sys.argv[3]

score = []
fi = open(pred_file, 'r')
line = fi.readline()
pst = 0
if line.strip() == 'labels 0 1':
	pst = 2
elif line.strip() == 'labels 1 0':
	pst = 1
while True:
	line = fi.readline()
	if not line:
		break
	s = float(line.strip().split()[pst])
	score.append(s)
fi.close()

eid2cid2slst = {}
eidcid2name = {}
fi = open(pair_file, 'r')
cnt = 0
for line in fi:
	eid = line.strip().split()[3]
	cid = int(line.strip().split()[5])
	name = line.strip().split()[2]

	if eid2cid2slst.get(eid, None) == None:
		eid2cid2slst[eid] = {}
	if eid2cid2slst[eid].get(cid, None) == None:
		eid2cid2slst[eid][cid] = []
	eid2cid2slst[eid][cid].append(score[cnt])

	eidcid2name[(eid, cid)] = name

	cnt += 1
fi.close()

fo = open(out_file, 'w')
for eid, dic in eid2cid2slst.items():
	cid2sc = {}
	for cid, slst in dic.items():
		#cn = 0.0
		#sm = 0.0
		#for vl in slst:
		#	sm += vl
		#	cn += 1

		#if slst == []:
		#	cid2sc[cid] = 0
		#else:
		#	cid2sc[cid] = sm / cn

		mx = 0.0
		for vl in slst:
			mx = max(mx, vl)
		cid2sc[cid] = mx

	cid2sc = sorted(cid2sc.items(), key = lambda x:x[1], reverse = True)

	for cid, sc in cid2sc:
		name = eidcid2name.get((eid, cid), 'None"')
		fo.write(eid + '\t' + str(cid) + '\t' + str(sc) + '\t' + name + '\n')
fo.close()

