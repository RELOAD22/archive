#!/bin/bash
echo "press any key to continue"
read
for dir in `ls -d */ |grep -v engine|grep -v google|grep -v Mobile|grep -v resnet10`
do  
    cd $dir
    for caffemodel in `ls *.caffemodel`
    do  
        for batchsize in 6 7 8 9 10 12 16 21 32 64
        do
            trtexec --batch=$batchsize --iterations=100 --workspace=2048 --deploy=deploy.prototxt --model=${caffemodel} --output=prob --fp16 --saveEngine=/root/workspace/models/engines/engines_10/${caffemodel%%.*}_bs${batchsize}.engine
        done
    done
    cd ../
done
#1 16 32 64 128 256 512 1024
#6 7 8 9 10 12 16 21 32 64
#51 53 56 60 64 68 73 78 85 93 102 113 128 146 170 204 256 341 512 1024