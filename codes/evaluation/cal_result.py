import sys
import os

eval_file = sys.argv[1]
score_file = sys.argv[2]
k_max = int(sys.argv[3])

check = {}
eid2train = {}
eid2test = {}

fi = open(eval_file, 'r')
while True:
	eid = fi.readline()
	trains = fi.readline()
	tests = fi.readline()

	if not eid:
		break
	if not trains:
		break
	if not tests:
		break

	eid = eid.strip()
	if check.get(eid, None) == None:
		check[eid] = {}

	trainl = trains.strip().split()
	testl = tests.strip().split()
	ntestl = []
	for ent in testl:
		if ent in trainl:
			continue
		ntestl.append(ent)
	testl = ntestl

	eid2train[eid] = trainl
	eid2test[eid] = testl

	validl = testl

	for ent in validl:
		#word = ent[0:ent.find('||')]
		word = ent
		check[eid][word] = 1	
fi.close()

eid2score = {}

fi = open(score_file, 'r')
for line in fi:
	eid = line.strip().split()[0]
	ent = line.strip().split()[1]
	val = float(line.strip().split()[2])
	if eid2score.get(eid, None) == None:
		eid2score[eid] = {}
	eid2score[eid][ent] = val
fi.close()

sh = [0.0 for i in range(k_max)]
sp = [0.0 for i in range(k_max)]
sr = [0.0 for i in range(k_max)]

for eid, dic in eid2score.items():
	if check.get(eid, None) == None:
		continue

	dic = sorted(dic.items(), key = lambda x:x[1], reverse = True)

	ch = [0.0 for i in range(k_max)]
	cp = [0.0 for i in range(k_max)]
	cr = [0.0 for i in range(k_max)]
	nhit = 0
	nprec = 0
	remain = len(eid2test[eid])
	#print eid, len(eid2test[eid])
	#exit(0)
	for k in range(k_max):
		ent = dic[k][0]
		#word = ent[0:ent.find('||')]
		word = ent		

		#print eid, ent, check[eid].get(word, 0)

		if check[eid].get(word, 0) == 1:
			nhit += 1
		ch[k] = nhit
		if remain > 0:
			nprec += 1
		if check[eid].get(word, 0) == 1:
			remain -= 1
		cp[k] = nprec
		cr[k] = len(eid2test[eid])

	for k in range(k_max):
		sh[k] += ch[k]
		sp[k] += cp[k]
		sr[k] += cr[k]

for k in range(k_max):
	print 'P@' + str(k+1), sh[k] / sp[k]
	#print sh[k] / sp[k]
for k in range(k_max):
	print 'R@' + str(k+1), sh[k] / sr[k]
	#print sh[k] / sr[k]

