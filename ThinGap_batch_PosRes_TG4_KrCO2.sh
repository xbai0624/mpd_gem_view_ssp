#!/bin/bash

# List of commands to execute
commands=(
    "./merge_run.sh 403 348 gem_map_srs_fermilab_setup2_run_period1.txt gem_tracking.conf_setup2_run_period1_0degree_Kr 1 1"
    "./merge_run.sh 404 348 gem_map_srs_fermilab_setup2_run_period1.txt gem_tracking.conf_setup2_run_period1_3degree_Kr 1 1"
    "./merge_run.sh 405 348 gem_map_srs_fermilab_setup2_run_period1.txt gem_tracking.conf_setup2_run_period1_6degree_Kr 1 1"
    "./merge_run.sh 406 348 gem_map_srs_fermilab_setup2_run_period1.txt gem_tracking.conf_setup2_run_period1_9degree_Kr 1 1"
    "./merge_run.sh 407 348 gem_map_srs_fermilab_setup2_run_period1.txt gem_tracking.conf_setup2_run_period1_12degree_Kr 1 1"
    "./merge_run.sh 409 348 gem_map_srs_fermilab_setup2_run_period1.txt gem_tracking.conf_setup2_run_period1_15degree_Kr 1 1"
    "./merge_run.sh 416 348 gem_map_srs_fermilab_setup2_run_period1.txt gem_tracking.conf_setup2_run_period1_18degree_Kr 1 1"
    "./merge_run.sh 410 348 gem_map_srs_fermilab_setup2_run_period1.txt gem_tracking.conf_setup2_run_period1_21degree_Kr 1 1"
    "./merge_run.sh 411 348 gem_map_srs_fermilab_setup2_run_period1.txt gem_tracking.conf_setup2_run_period1_27degree_Kr 1 1"
    "./merge_run.sh 412 348 gem_map_srs_fermilab_setup2_run_period1.txt gem_tracking.conf_setup2_run_period1_33degree_Kr 1 1"
    "./merge_run.sh 414 348 gem_map_srs_fermilab_setup2_run_period1.txt gem_tracking.conf_setup2_run_period1_39degree_Kr 1 1"
    "./merge_run.sh 415 348 gem_map_srs_fermilab_setup2_run_period1.txt gem_tracking.conf_setup2_run_period1_45degree_Kr 1 1"
)

# Arrays to keep track of PIDs and corresponding command names
pids=()
names=()
TOTAL_CMDS=${#commands[@]}

# Function to run a command in xterm
run_command() {
    local cmd="$1"
    local index="$2"
    local name="Command #$index of $TOTAL_CMDS [ TG4 | KrCO2 | Position Resolution ]"

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
max_jobs=5

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

