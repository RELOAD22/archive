#!/bin/bash

# $1-core $2-jobname $3-batch

GetLogName(){
    log_name=log/${1}_${2}*${3}.log
}

corenum=$1
jobname=$2
jobsize=$3

batchsize=`expr $jobsize / $corenum`
#engine_names=("bvlc_alexnet_bs${batchsize}.engine" "ResNet50_fp32_bs${batchsize}.engine" "googlenet_bs${batchsize}.engine" "mobilenet_v2_bs${batchsize}.engine")
#engine_names=("bvlc_alexnet_bs1024.engine" "ResNet50_fp32_bs1024.engine" "googlenet_bs1024.engine" "mobilenet_v2_bs1024.engine")
#engine_names=("bvlc_alexnet_bs256.engine" "ResNet50_fp32_bs256.engine" "googlenet_bs256.engine" "mobilenet_v2_bs512.engine")

case "$jobname" in
   "AlexNet") engine_index=0 
   ;;
   "ResNet") engine_index=1
   ;;   
   "GoogleNet") engine_index=2
   ;;
   "MobileNet") engine_index=3  
   ;;
esac

# choose engine size
if [ $batchsize -le 64 ]
then
   engine_batchsize=64
elif [ $batchsize -le 128 ]
then
   engine_batchsize=128
elif [ $batchsize -le 256 ]
then
   engine_batchsize=256
else
   engine_batchsize=512
fi

engine_names=("bvlc_alexnet_bs${engine_batchsize}.engine" "ResNet50_fp32_bs${engine_batchsize}.engine" "googlenet_bs${engine_batchsize}.engine" "mobilenet_v2_bs${engine_batchsize}.engine")

GetLogName ${engine_names[$engine_index]} $batchsize $corenum

echo "================================================" > ${log_name}
echo "RUN ENGINE: ${engine_names[${engine_index}]}"
echo "[JOB]: jobnum: $corenum  batchsize: $batchsize * $corenum" >> ${log_name}

echo "mpirun -np $corenum --allow-run-as-root trtexec --batch=${batchsize} --loadEngine=${engine_names[${engine_index}]} --iterations=1 --avgRuns=1 --workspace=2048 --fp16 --warmUp=0 --duration=0 2>&1 |grep mean |grep end |awk -F "]" '{print $NF}' >> ${log_name}"
mpirun -np $corenum --allow-run-as-root trtexec --batch=${batchsize} --loadEngine=${engine_names[${engine_index}]} --iterations=1 --avgRuns=1 --workspace=2048 --fp16 --warmUp=0 --duration=0 2>&1  >> ${log_name}
#mpirun -np $corenum --allow-run-as-root trtexec --batch=${batchsize} --loadEngine=${engine_names[${engine_index}]} --iterations=1 --workspace=2048 --fp16 2>&1 |grep mean |grep end |awk -F "]" '{print $NF}' >> ${log_name}
