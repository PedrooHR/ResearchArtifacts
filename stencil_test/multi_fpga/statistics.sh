#!/bin/bash

PRINT_MODE="${MODE:-DEBUG}"

NUM_ELEMENTS=33554432
declare -A STENCIL_FLOPS=(['jacobi-1d']=3 ['jacobi-2d']=5 ['jacobi-3d']=7 ['heat-1d']=4 ['heat-2d']=10 ['heat-3d']=15)
STENCIL_TYPE=('heat' 'jacobi')
STENCIL_SIZE=('1d' '2d' '3d')
FPGAs=(1 2 3 4 5 6 7 8)

cd results
for TYPE in "${STENCIL_TYPE[@]}"; do
  cd ${TYPE}
  for SIZE in "${STENCIL_SIZE[@]}"; do
    cd ${SIZE}     
    [[ "$PRINT_MODE" == "TABLE" ]] && printf "%7s %4s %9s %4s %10s %10s %10s\n" "Stencil" "Type" "Instances" "FPGA" "Avg" "Int Low" "Int High"
    [[ "$PRINT_MODE" == "CSV" ]] && printf "%s;%s;%s;%s;%s;%s;%s\n" "Stencil" "Type" "Instances" "FPGA" "Avg" "Int Low" "Int High"

    for file in $(printf "%s\n" ./* | sort -V); do     
      instances=${file:2}

      for FPGA in "${FPGAs[@]}"; do
        r_file="${file}/res_${FPGA}_128.err"

        iterations=$(bc<<<"scale=0;1024/($instances*$FPGA)")
        iterations=$(bc<<<"scale=0;$iterations*$instances*$FPGA")

        # Global Variables
        count=0
        z_score=3.291 # 99,9% confidence interval

        # Calculate Average
        sum=0
        while read -r line; do
          sum=$(bc<<<"scale=2;$sum+$line")
          let "count++"
        done < "$r_file"
        average=$(bc<<<"scale=2;$sum/$count")
        [[ "$PRINT_MODE" == "DEBUG" ]] && printf "Stencil: %7s | Size: ${SIZE} | Instances: %3s | Num FPGAs: ${FPGA} -> Average: ${average}\n" ${TYPE} ${instances}

        # Calculate Standard Deviation
        dev_sum=0
        while read -r line; do
          sq_deviation=$(bc<<<"scale=2;($average-$line)^2")
          dev_sum=$(bc<<<"scale=2;$sum+$sq_deviation")
        done < "$r_file"
        std_deviation=$(bc<<<"scale=2;sqrt($dev_sum/$count)")
        [[ "$PRINT_MODE" == "DEBUG" ]] && printf "%59s -> Std. Deviation: ${std_deviation}\n" " "
    
        # Standard Error
        std_error=$(bc<<<"scale=2;$std_deviation/sqrt($count)")
        [[ "$PRINT_MODE" == "DEBUG" ]] && printf "%59s -> Std. Error: ${std_error}\n" " "

        # 95% confidence interval
        error_999=$(bc<<<"scale=2;$std_error*$z_score")
        [[ "$PRINT_MODE" == "DEBUG" ]] && printf "%59s -> Confidence error: ${error_999}\n" " "

        # Table entry
        interval_low=$(bc<<<"scale=4;($average-$error_999)/1000000")
        interval_high=$(bc<<<"scale=4;($average+$error_999)/1000000")
        average_sec=$(bc<<<"scale=4;$average/1000000")

        flops_stencil=${STENCIL_FLOPS["${TYPE}-${SIZE}"]}
        gflops_total=$(bc<<<"scale=4;($NUM_ELEMENTS*$flops_stencil*$iterations)/1000000000")

        gflops_sec_avg=$(bc<<<"scale=4;$gflops_total/$average_sec")
        gflops_sec_low=$(bc<<<"scale=4;$gflops_total/$interval_low")
        gflops_sec_high=$(bc<<<"scale=4;$gflops_total/$interval_high")
        [[ "$PRINT_MODE" == "TABLE" ]] && printf "%7s %4s %9s %4s %10s %10s %10s\n" $TYPE $SIZE $instances $FPGA $gflops_sec_avg $gflops_sec_low $gflops_sec_high
        [[ "$PRINT_MODE" == "CSV" ]] && printf "%s;%s;%s;%s;%s;%s;%s\n" $TYPE $SIZE $instances $FPGA $gflops_sec_avg $gflops_sec_low $gflops_sec_high
      done
    done
    [[ "$PRINT_MODE" == "TABLE" ]] && printf "\n"
    [[ "$PRINT_MODE" == "CSV" ]] && printf "\n"
    cd ..
  done
  cd ..
done

