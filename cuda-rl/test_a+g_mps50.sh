#!/bin/bash
echo "press any key to continue"
read

for batchsize in 1 16 32 64
do
    echo "Batch Size:$batchsize"

    backfile="googlenet_bs${batchsize}.engine"
    frontfile="bvlc_alexnet_bs${batchsize}.engine"
    echo "RUN ENGINE: $backfile $frontfile"

    nohup trtexec --batch=${batchsize} --loadEngine=$backfile --iterations=8000 --workspace=2048 --fp16 2>&1 > latency_back_bs$batchsize.log &
    trtexec --batch=${batchsize} --loadEngine=$frontfile --iterations=1000 --workspace=2048 --fp16 2>&1 > latency_front_bs$batchsize.log
    n=`ps|grep trtexec|wc -l`
    while(($n!=0))
    do
        sleep 1
        n=`ps|grep trtexec|wc -l`
    done
    echo "$backfile finish"
done

for batchsize in 128 256 512 1024
do
    echo "Batch Size:$batchsize"

    backfile="googlenet_bs${batchsize}.engine"
    frontfile="bvlc_alexnet_bs${batchsize}.engine"
    echo "RUN ENGINE: $backfile $frontfile"

    nohup trtexec --batch=${batchsize} --loadEngine=$backfile --iterations=500 --workspace=2048 --fp16 2>&1 > latency_back_bs$batchsize.log &
    sleep 2
    trtexec --batch=${batchsize} --loadEngine=$frontfile --iterations=500 --workspace=2048 --fp16 2>&1 > latency_front_bs$batchsize.log
    n=`ps|grep trtexec|wc -l`
    while(($n!=0))
    do
        sleep 1
        n=`ps|grep trtexec|wc -l`
    done
    echo "$backfile finish"
done
#1 16 32 64 128 256 512 1024
