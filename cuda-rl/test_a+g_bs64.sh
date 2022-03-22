#!/bin/bash
echo "please ensure 'nvidia-smi -i 0 -c EXCLUSIVE_PROCESS' "
echo "press any key to continue"
read
echo quit | nvidia-cuda-mps-control

batchsize=64
echo "Batch Size:$batchsize"
for mps_percentage in 100 75 50 25 15 10 5
do  
    nvidia-cuda-mps-control -d
    echo set_default_active_thread_percentage $mps_percentage|nvidia-cuda-mps-control
    
    backfile="googlenet_bs${batchsize}.engine"
    frontfile="bvlc_alexnet_bs${batchsize}.engine"
    echo "RUN ENGINE: $backfile $frontfile"
    nohup trtexec --batch=${batchsize} --loadEngine=$backfile --iterations=500 --workspace=2048 --fp16 2>&1 > log_bs${batchsize}/latency_back_mps$mps_percentage.log &
    trtexec --batch=${batchsize} --loadEngine=$frontfile --iterations=500 --workspace=2048 --fp16 2>&1 > log_bs${batchsize}/latency_front_mps$mps_percentage.log
    n=`ps|grep trtexec|wc -l`
    while(($n!=0))
    do
        sleep 1
        n=`ps|grep trtexec|wc -l`
    done
    echo "$backfile finish"
    echo quit | nvidia-cuda-mps-control
done
#1 16 32 64 128 256 512 1024
