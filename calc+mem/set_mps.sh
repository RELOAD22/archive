#!/bin/bash
echo quit | nvidia-cuda-mps-control
mps_percentage=$1

OpenMPS(){
    nvidia-cuda-mps-control -d
    echo "[MPS]: set mps default_active_thread_percentage: $1"
    echo set_default_active_thread_percentage $1|nvidia-cuda-mps-control    
}

OpenMPS ${mps_percentage}