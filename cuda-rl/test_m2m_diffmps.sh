#!/bin/bash
echo "please ensure 'nvidia-smi -i 0 -c EXCLUSIVE_PROCESS' "
echo "press any key to continue"
read
echo quit | nvidia-cuda-mps-control

batchsize=128
echo "Batch Size:$batchsize"

engine_names=("bvlc_alexnet_bs${batchsize}.engine" "googlenet_bs${batchsize}.engine" "mobilenet_v2_bs${batchsize}.engine" "ResNet50_fp32_bs${batchsize}.engine")
OpenMPS(){
    nvidia-cuda-mps-control -d
    echo "[MPS]: set mps default_active_thread_percentage: $1"
    echo set_default_active_thread_percentage $1|nvidia-cuda-mps-control    
}

CloseMPS(){
    echo "[MPS]: close mps control daemon"
    echo quit | nvidia-cuda-mps-control
}

WaitUntilNotrtexec(){
    n=`ps|grep trtexec|wc -l`
    while(($n!=0))
    do
        sleep 1
        n=`ps|grep trtexec|wc -l`
    done
    echo "finish backengine"
}

SetParams(){
#$1--mps
case $1 in
    100)  iterations=500
    ;;
    75)  iterations=400
    ;;
    50)  iterations=300
    ;;
    25)  iterations=300
    ;;
    15)  iterations=200
    ;;
    10)  iterations=200
    ;;
    5)  iterations=200
    ;;
    *)  echo 'MPS OUT OF RANGE!!!!!!!!'
    ;;
esac
}
for((i=0;i<=3;i++))
do
    echo "==============================" > log_bs${batchsize}/${engine_names[$i]}.log
done 

for mps_percentage in 100 75 50 25 15 10 5
do  
    OpenMPS $mps_percentage

    SetParams $mps_percentage
    for((i=0;i<=3;i++))
    do
        echo "[MPS]: $mps_percentage" >> log_bs${batchsize}/${engine_names[$i]}.log
        for((j=0;j<=3;j++))
        do
            echo "RUN ENGINE: front:${engine_names[$i]} back:${engine_names[$j]} "
            nohup trtexec --batch=${batchsize} --loadEngine=${engine_names[$j]} --iterations=$iterations --workspace=2048 --fp16 &
            echo "run with ${engine_names[$j]}:" >> log_bs${batchsize}/${engine_names[$i]}.log
            sleep 1
            trtexec --batch=${batchsize} --loadEngine=${engine_names[$i]} --iterations=100 --workspace=2048 --fp16 2>&1 |grep mean |grep end |awk -F "]" '{print $NF}' >> log_bs${batchsize}/${engine_names[$i]}.log
            
            WaitUntilNotrtexec
        done
    done

    CloseMPS
done
#100 75 50 25 15 10 5
#1 16 32 64 128 256 512 1024
