#!/bin/bash

source setup_env.sh

echo "Running multi-core batch job"  

xterm -e './merge_run.sh 904 903 2' &
pid1=$!
echo "Job started for Run 904"

xterm -e './merge_run.sh 906 903 2' &
pid2=$!
echo "Job started for Run 906"

xterm -e './merge_run.sh 908 903 2' &
pid3=$!
echo "Job started for Run 908"

# Array to keep track of PIDs
pids=($pid1 $pid2 $pid3)
names=("Run 904" "Run 906" "Run 908")

# Loop until all processes are finished
while [[ ${#pids[@]} -gt 0 ]]; do
    for i in "${!pids[@]}"; do
        if ! kill -0 ${pids[i]} 2> /dev/null; then
            echo "Job finished for ${names[i]}"
            unset pids[i]  # Remove the PID from the list
            unset names[i] # Remove the name from the list
        fi
    done
    pids=("${pids[@]}") # Rebuild the array
    names=("${names[@]}") # Rebuild the array
    sleep 5  # Wait a bit before checking again
done

echo "Multi-core batch job finished"
