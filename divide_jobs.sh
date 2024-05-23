#!/bin/bash

source setup_env.sh

RUN=$1

echo "analyzing run" $RUN

for i in {0..8}
do
    # replay run
    job_events=100000
    start_event=$((i*job_events))
    xterm -e './bin/replay -c 0 -t 0 -z 1 -n $job_events -s $start_event --tracking off --pedestal database/gem_ped_903.dat --common_mode database/CommonModeRange_903.txt /home/daq/coda/data/fermilab_beamtest_$RUN.evio.0' &

    echo $job_events $start_event
done
