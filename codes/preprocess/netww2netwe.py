import sys
import os

fi = open(sys.argv[1], 'r')
fo = open(sys.argv[2], 'w')
cnt = 0
for line in fi:
	if cnt % 1000000 == 0:
		print cnt
	cnt += 1
	v = line.strip().split()[1]
	if v.find('||') == -1:
		continue
	fo.write(line)
print cnt
fi.close()
fo.close()

