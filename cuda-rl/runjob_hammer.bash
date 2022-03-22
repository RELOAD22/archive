#!/bin/bash

# $1-core $2-jobname $3-batch

#max corenum = 5
corenum=$1
jobname=$2
jobsize=$3

random_id=$RANDOM
filename=${jobname}_${jobsize}_${random_id}

mps_percentage=`expr $corenum * 20`
echo "[MPS PERCENTAGE]: ${mps_percentage}"

filepath=${mps_percentage}/filename
touch ${filepath}

while [ -e ${filepath} ]
do 
done

echo "job finish"