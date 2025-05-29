#!/bin/bash

set -e

source env.sh

NUM_FPGA_NODE=2
NUM_REPS=100
MB_ARRAY=(1 2 4 8 16 32 64 128 256) # Can't go to 512MB
FPGA_ARRAY=(2) 

# rm -rf results

for FPGA in "${FPGA_ARRAY[@]}"; do
  for MB in "${MB_ARRAY[@]}"; do
    mkdir -p results/${FPGA}
    echo "Running ${MB} MB experiment for ${FPGA} FPGAs"
    LD_PRELOAD=/lib/x86_64-linux-gnu/libSegFault.so mpirun -np ${FPGA} --hosts a3:2,a2:2,a1:2,a0:2 ./main eth0.xclbin ${NUM_FPGA_NODE} ${NUM_REPS} ${MB} 2> results/${FPGA}/res_${MB}.log > output.log
  done
done