#!/bin/bash

source setup_env.sh

for RUN in {283..286}
do	
    	echo "analyzing run" $RUN

	# replay run
	./bin/replay -c 0 -t 0 -z 1 -n -1 --tracking on --pedestal database/gem_ped_310.dat --common_mode database/CommonModeRange_310.txt /home/daq/coda/data/fermilab_beamtest_$RUN.evio.0

	# plot run
	ROOT_FILE='scripts/plot_quality_results.cpp("Rootfiles/cluster_0_fermilab_beamtest_'${RUN}'..root_data_quality_check.root")'
	root -b -q $ROOT_FILE

done
