#!/bin/bash

# usage: ./benchmark.sh <file_with_commands_to_run> <iterations>

FILES=$1
ITER=$2
NPROG=`cat ${FILES} | wc -l`

TIMEFORMAT="real %3R"

for i in `seq 1 ${NPROG}`
do
	PROG=`sed -n "${i}p" ${FILES}`
	echo "${PROG}"
	for (( j=0; j < ${ITER}; j++ ))
	do
		TIME=`{ time ${PROG}; } 2>&1 | grep real | awk '{print $2}'`
		echo "${TIME}"
	done
done