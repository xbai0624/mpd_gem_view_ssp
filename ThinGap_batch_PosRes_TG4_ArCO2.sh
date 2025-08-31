#!/bin/bash

# List of commands to execute
commands=(
    "./merge_run.sh 367 348 gem_map_srs_fermilab_setup2_run_period1.txt gem_tracking.conf_setup2_run_period1_0degree 1 1"
    "./merge_run.sh 368 348 gem_map_srs_fermilab_setup2_run_period1.txt gem_tracking.conf_setup2_run_period1_3degree 1 1"
    "./merge_run.sh 369 348 gem_map_srs_fermilab_setup2_run_period1.txt gem_tracking.conf_setup2_run_period1_6degree 1 1"
    "./merge_run.sh 360 348 gem_map_srs_fermilab_setup2_run_period1.txt gem_tracking.conf_setup2_run_period1_9degree 1 1"
    "./merge_run.sh 370 348 gem_map_srs_fermilab_setup2_run_period1.txt gem_tracking.conf_setup2_run_period1_12degree 1 1"
    "./merge_run.sh 361 348 gem_map_srs_fermilab_setup2_run_period1.txt gem_tracking.conf_setup2_run_period1_15degree 1 1"
    "./merge_run.sh 371 348 gem_map_srs_fermilab_setup2_run_period1.txt gem_tracking.conf_setup2_run_period1_18degree 1 1"
    "./merge_run.sh 362 348 gem_map_srs_fermilab_setup2_run_period1.txt gem_tracking.conf_setup2_run_period1_21degree 1 1"
    "./merge_run.sh 355 348 gem_map_srs_fermilab_setup2_run_period1.txt gem_tracking.conf_setup2_run_period1_24degree 1 1"
    "./merge_run.sh 363 348 gem_map_srs_fermilab_setup2_run_period1.txt gem_tracking.conf_setup2_run_period1_27degree 1 1"
    "./merge_run.sh 356 348 gem_map_srs_fermilab_setup2_run_period1.txt gem_tracking.conf_setup2_run_period1_30degree 1 1"
    "./merge_run.sh 364 348 gem_map_srs_fermilab_setup2_run_period1.txt gem_tracking.conf_setup2_run_period1_33degree 1 1"
    "./merge_run.sh 357 348 gem_map_srs_fermilab_setup2_run_period1.txt gem_tracking.conf_setup2_run_period1_36degree 1 1"
    "./merge_run.sh 365 348 gem_map_srs_fermilab_setup2_run_period1.txt gem_tracking.conf_setup2_run_period1_39degree 1 1"
    "./merge_run.sh 358 348 gem_map_srs_fermilab_setup2_run_period1.txt gem_tracking.conf_setup2_run_period1_42degree 1 1"
    "./merge_run.sh 359 348 gem_map_srs_fermilab_setup2_run_period1.txt gem_tracking.conf_setup2_run_period1_45degree 1 1"
)

# Arrays to keep track of PIDs and corresponding command names
pids=()
names=()
TOTAL_CMDS=${#commands[@]}

# Function to run a command in xterm
run_command() {
    local cmd="$1"
    local index="$2"
    local name="Command #$index of $TOTAL_CMDS [ TG4 | ArCO2 | Position Resolution ]"

    # Log start message to terminal
    echo "$name started: $cmd"

    # Run the command in xterm without any additional output
    xterm -e "$cmd" &

    # Store the PID and corresponding command name
    local pid=$!
    pids+=($pid)
    names+=("$name")
}

# Set max_jobs based on the number of physical cores initially
max_jobs=3

# Loop over the commands sequentially
for i in "${!commands[@]}"; do
    cmd="${commands[i]}"
    # Run the command in xterm
    run_command "$cmd" "$((i + 1))"

    # Check and wait if the number of jobs exceeds max_jobs
    while (( ${#pids[@]} >= max_jobs )); do
        printf "Waiting for jobs to finish. Current jobs: ${#pids[@]}\r"
        for j in "${!pids[@]}"; do
            if ! kill -0 ${pids[j]} 2>/dev/null; then
                echo -e "\n${names[j]} has completed execution"
                unset pids[j]
                unset names[j]
            fi
        done
        # Rebuild the arrays
        pids=("${pids[@]}")
        names=("${names[@]}")
        sleep 1
    done
done

# Wait for all remaining jobs to finish
while (( ${#pids[@]} > 0 )); do
    printf "Waiting for remaining jobs to finish. Current jobs: ${#pids[@]}\r"
    for j in "${!pids[@]}"; do
        if ! kill -0 ${pids[j]} 2>/dev/null; then
            echo -e "\n${names[j]} has completed execution"
            unset pids[j]
            unset names[j]
        fi
    done
    # Rebuild the arrays
    pids=("${pids[@]}")
    names=("${names[@]}")
    sleep 1
done

echo -e "\nAll commands have been processed."

