#!/bin/bash

set -e

source ../env.sh

make clean && make

XCLBINS_DIR=/home/pedro.rosso/TRETS/xclbins
NUM_REPS=10
BUFFER_SIZES=(128)
STENCIL_TYPES=('heat')
STENCIL_DIMS=('3d')
STENCIL_INSTANCES=(1 2 4 8)

# rm -rf results

for STENCIL_TYPE in "${STENCIL_TYPES[@]}"; do
  mkdir -p results/${STENCIL_TYPE}
  for STENCIL_DIM in "${STENCIL_DIMS[@]}"; do
    mkdir -p results/${STENCIL_TYPE}/${STENCIL_DIM}
    for STENCIL_INSTANCE in "${STENCIL_INSTANCES[@]}"; do
      mkdir -p results/${STENCIL_TYPE}/${STENCIL_DIM}/${STENCIL_INSTANCE}
      RESULT_FOLDER=results/${STENCIL_TYPE}/${STENCIL_DIM}/${STENCIL_INSTANCE}
      for BUFFER_SIZE in "${BUFFER_SIZES}"; do
        echo "Executing ${STENCIL_TYPE} ${STENCIL_DIM} with ${STENCIL_INSTANCE} instances"

        # echo "Results for ${STENCIL_TYPE} ${STENCIL_DIM} with ${STENCIL_INSTANCE} instances" > ${RESULT_FOLDER}/results.out
        # echo "Results for ${STENCIL_TYPE} ${STENCIL_DIM} with ${STENCIL_INSTANCE} instances" > ${RESULT_FOLDER}/results.err
        
        XCLBIN_FILE=${XCLBINS_DIR}/${STENCIL_TYPE}/${STENCIL_DIM}/${STENCIL_INSTANCE}.xclbin
        
        ./main ${XCLBIN_FILE} ${NUM_REPS} ${BUFFER_SIZE} ${STENCIL_INSTANCE} >> ${RESULT_FOLDER}/results.out 2>> ${RESULT_FOLDER}/results.err 
      done
    done
  done
done