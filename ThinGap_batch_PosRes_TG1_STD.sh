#!/bin/bash

# List of commands to execute
commands=(
    "./merge_run.sh 579 564 gem_map_srs_fermilab_setup2_run_period4.txt gem_tracking.conf_setup2_run_period4_0degree 1 1"
    "./merge_run.sh 578 564 gem_map_srs_fermilab_setup2_run_period4.txt gem_tracking.conf_setup2_run_period4_3degree 1 1"
    "./merge_run.sh 580 564 gem_map_srs_fermilab_setup2_run_period4.txt gem_tracking.conf_setup2_run_period4_6degree 1 1"
    "./merge_run.sh 577 564 gem_map_srs_fermilab_setup2_run_period4.txt gem_tracking.conf_setup2_run_period4_9degree 1 1"
    "./merge_run.sh 576 564 gem_map_srs_fermilab_setup2_run_period4.txt gem_tracking.conf_setup2_run_period4_15degree 1 1"
    "./merge_run.sh 575 564 gem_map_srs_fermilab_setup2_run_period4.txt gem_tracking.conf_setup2_run_period4_21degree 1 1"
    "./merge_run.sh 574 564 gem_map_srs_fermilab_setup2_run_period4.txt gem_tracking.conf_setup2_run_period4_27degree 1 1"
    "./merge_run.sh 573 564 gem_map_srs_fermilab_setup2_run_period4.txt gem_tracking.conf_setup2_run_period4_33degree 1 1"
    "./merge_run.sh 572 564 gem_map_srs_fermilab_setup2_run_period4.txt gem_tracking.conf_setup2_run_period4_39degree 1 1"
    "./merge_run.sh 571 564 gem_map_srs_fermilab_setup2_run_period4.txt gem_tracking.conf_setup2_run_period4_45degree 1 1"
)

# Arrays to keep track of PIDs and corresponding command names
pids=()
names=()
TOTAL_CMDS=${#commands[@]}

# Function to run a command in xterm
run_command() {
    local cmd="$1"
    local index="$2"
    local name="Command #$index of $TOTAL_CMDS [ TG1 | STD | Position Resolution ]"

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

