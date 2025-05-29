#!/bin/bash

set -e

source ../env.sh

make clean && make

NUM_FPGA_NODE=2
XCLBINS_DIR=/home/pedro.rosso/TRETS/xclbins
NUM_REPS=5
BUFFER_SIZES=(128)
STENCIL_TYPES=('heat')
STENCIL_DIMS=('1d')
STENCIL_INSTANCES=(32)
FPGA_ARRAY=(2) 

# rm -rf results

for STENCIL_TYPE in "${STENCIL_TYPES[@]}"; do
  mkdir -p results/${STENCIL_TYPE}
  for STENCIL_DIM in "${STENCIL_DIMS[@]}"; do
    mkdir -p results/${STENCIL_TYPE}/${STENCIL_DIM}
    for STENCIL_INSTANCE in "${STENCIL_INSTANCES[@]}"; do
      for FPGA in "${FPGA_ARRAY[@]}"; do
        mkdir -p results/${STENCIL_TYPE}/${STENCIL_DIM}/${STENCIL_INSTANCE}
        RESULT_FOLDER=results/${STENCIL_TYPE}/${STENCIL_DIM}/${STENCIL_INSTANCE}
        for BUFFER_SIZE in "${BUFFER_SIZES}"; do
          echo "Executing ${STENCIL_TYPE} ${STENCIL_DIM} with ${STENCIL_INSTANCE} instances in ${FPGA} FPGAs"

          # echo "Results for ${STENCIL_TYPE} ${STENCIL_DIM} with ${STENCIL_INSTANCE} instances" > ${RESULT_FOLDER}/res_${FPGA}_${BUFFER_SIZE}.out
          # echo "Results for ${STENCIL_TYPE} ${STENCIL_DIM} with ${STENCIL_INSTANCE} instances" > ${RESULT_FOLDER}/res_${FPGA}_${BUFFER_SIZE}.err
          
          XCLBIN_FILE=${XCLBINS_DIR}/${STENCIL_TYPE}/${STENCIL_DIM}/${STENCIL_INSTANCE}.xclbin
          
          mpirun -np ${FPGA} --hosts a3:2,a2:2,a1:2,a0:2 ./main ${XCLBIN_FILE} ${NUM_FPGA_NODE} ${NUM_REPS} ${BUFFER_SIZE} ${STENCIL_INSTANCE} >> ${RESULT_FOLDER}/res_${FPGA}_${BUFFER_SIZE}.out 2>> ${RESULT_FOLDER}/res_${FPGA}_${BUFFER_SIZE}.err 
        done
      done
    done
  done
done