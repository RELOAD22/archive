#!/bin/bash
echo "please ensure 'nvidia-smi -i 0 -c EXCLUSIVE_PROCESS' "
echo "press any key to continue"
read
echo quit | nvidia-cuda-mps-control

mps_percentage=100
echo "MPS percentage:$mps_percentage"

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

GetLogName(){
    engine_names=("bvlc_alexnet_bs${batchsize}.engine" "googlenet_bs${batchsize}.engine" "mobilenet_v2_bs${batchsize}.engine" "ResNet50_fp32_bs${batchsize}.engine")
    name=${engine_names[$1]}
    log_name=log_mps${mps_percentage}/${name:0:12}.log
}

SetParams(){
#$1--batchsize
case $1 in
    1)  iterations=10000
    ;;
    16)  iterations=3000
    ;;
    32)  iterations=2000
    ;;
    64)  iterations=1000
    ;;
    128)  iterations=500
    ;;
    256)  iterations=500
    ;;
    512)  iterations=300
    ;;
    1024)  iterations=200
    ;;
    *)  echo 'BATCHSIZE OUT OF RANGE!!!!!!!!'
    ;;
esac
}
for((i=0;i<=3;i++))
do
    GetLogName i
    echo "==============================" > $log_name
done 

OpenMPS $mps_percentage

for batchsize in 1 16 32 64 128 256 512 1024
do  
    engine_names=("bvlc_alexnet_bs${batchsize}.engine" "googlenet_bs${batchsize}.engine" "mobilenet_v2_bs${batchsize}.engine" "ResNet50_fp32_bs${batchsize}.engine")
    SetParams $batchsize

    for((i=0;i<=3;i++))
    do
        GetLogName i
        echo "[BS]: $batchsize" >> $log_name
        for((j=0;j<=3;j++))
        do
            sleep 3
            echo "RUN ENGINE: front:${engine_names[$i]} back:${engine_names[$j]} "
            nohup trtexec --batch=${batchsize} --loadEngine=${engine_names[$j]} --iterations=$iterations --workspace=2048 --fp16 &
            echo "run with ${engine_names[$j]}:" >> $log_name
            sleep 1
            trtexec --batch=${batchsize} --loadEngine=${engine_names[$i]} --iterations=100 --workspace=2048 --fp16 2>&1 |grep mean |grep end |awk -F "]" '{print $NF}' >> $log_name
            #trtexec --batch=${batchsize} --loadEngine=${engine_names[$i]} --iterations=100 --workspace=2048 --fp16 2>&1 |grep mean |grep end
            
            #trtexec --batch=${batchsize} --loadEngine=${engine_names[$i]} --iterations=100 --workspace=2048 --fp16
            WaitUntilNotrtexec

        done
    done
done

CloseMPS
#100 75 50 25 15 10 5
#1 16 32 64 128 256 512 1024
