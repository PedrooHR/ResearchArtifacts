#!/bin/bash

PRINT_MODE="${MODE:-DEBUG}"

FPGAs=(2 3 4 5 6 7 8)

for FPGA in "${FPGAs[@]}"; do
  [[ "$PRINT_MODE" == "TABLE" ]] && printf "Results for %s FPGAs\n" ${FPGA}
  [[ "$PRINT_MODE" == "CSV" ]] && printf "Results for %s FPGAs\n" ${FPGA}

  [[ "$PRINT_MODE" == "TABLE" ]] && printf "%5s %10s %10s %10s\n" "Size" "Avg" "Int Low" "Int High"
  [[ "$PRINT_MODE" == "CSV" ]] && printf "%s;%s;%s;%s\n" "Size" "Avg" "Int Low" "Int High"
  for file in results/${FPGA}/*; do
    size=${file:14:-4}
    [[ "$PRINT_MODE" == "DEBUG" ]] && echo "File ${file} - Size of Transfers: ${size}"
    
    # Global Variables
    count=0
    z_score=1.96 # 95% confidence interval

    # Calculate Average
    sum=0
    while read -r line; do
      sum=$(bc<<<"scale=2;$sum+$line")
      let "count++"
    done < "$file"
    average=$(bc<<<"scale=2;$sum/$count")
    [[ "$PRINT_MODE" == "DEBUG" ]] && echo "Average: ${average}"

    # Calculate Standard Deviation
    dev_sum=0
    while read -r line; do
      sq_deviation=$(bc<<<"scale=2;($average-$line)^2")
      dev_sum=$(bc<<<"scale=2;$sum+$sq_deviation")
    done < "$file"
    std_deviation=$(bc<<<"scale=2;sqrt($dev_sum/$count)")
    [[ "$PRINT_MODE" == "DEBUG" ]] &&  echo "Standard Deviation: ${std_deviation}"

    # Standard Error
    std_error=$(bc<<<"scale=2;$std_deviation/sqrt($count)")
    [[ "$PRINT_MODE" == "DEBUG" ]] && echo "Standard Error: ${std_error}"

    # 95% confidence interval
    error_95=$(bc<<<"scale=2;$std_error*$z_score")
    [[ "$PRINT_MODE" == "DEBUG" ]] && echo "95% confidence error: ${error_95}"

    # Table entry
    interval_low=$(bc<<<"scale=2;$average-$error_95")
    interval_high=$(bc<<<"scale=2;$average+$error_95")
    [[ "$PRINT_MODE" == "TABLE" ]] && printf "%5s %10s %10s %10s\n" $size $average $interval_low $interval_high
    [[ "$PRINT_MODE" == "CSV" ]] && printf "%s;%s;%s;%s\n" $size $average $interval_low $interval_high
  done
  [[ "$PRINT_MODE" == "TABLE" ]] && printf "\n"
  [[ "$PRINT_MODE" == "CSV" ]] && printf "\n"
done

