#!/bin/bash
echo "please ensure 'nvidia-smi -i 0 -c EXCLUSIVE_PROCESS' "
echo "press any key to continue"
read

OpenMPS(){
    nvidia-cuda-mps-control -d
    echo "[MPS]: set mps default_active_thread_percentage: $1"
    echo set_default_active_thread_percentage $1|nvidia-cuda-mps-control    
}

CloseMPS(){
    echo "[MPS]: close mps control daemon"
    echo quit | nvidia-cuda-mps-control
}


CloseMPS
OpenMPS 10
