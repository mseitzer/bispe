#!/usr/bin/env python
import os, sys, shutil

def usage():
	print("Usage: ./parse.py <data_file>")
	sys.exit(-1)

times = {}

if len(sys.argv) != 2:
	usage()


f = open(sys.argv[1], 'r')

program = ""

for line in f.readlines():
	try:
		time = float(line)
		times[program][0] += 1
		times[program][1] += time
	except ValueError:
		program = ""
		for p in times.iterkeys():
			if p in line:
				program = p
				break
		if program == "":
			program = line.rstrip("\n ")
			times.update({program:[0,0.0]})

f.close()

for prog, time in times.iteritems():
	n = time[0]
	s = time[1]

	if n == 0:
		continue

	print prog + ":\n" + str(n) + ", " + str(s) + ", " + str(s / n)