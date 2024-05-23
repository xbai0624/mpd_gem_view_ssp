#!/bin/bash

source ./setup_env.sh

RUN=$1
PEDESTAL=$2
SPLITS=${3:-1}

for ((splitNum = 0; splitNum < $SPLITS; splitNum++))
do
	# replay run
	
	echo "analyzing run:" $RUN 
        echo "split #:" $splitNum 

	./bin/replay -c 0 -t 0 -z 1 -n -1 --tracking on --pedestal database/gem_ped_$PEDESTAL.dat --common_mode database/CommonModeRange_$PEDESTAL.txt /home/daq/coda/data/fermilab_beamtest_$RUN.evio.$splitNum 

	# rename runs
        mv "Rootfiles/cluster_0_fermilab_beamtest_$RUN..root_data_quality_check.root" "Rootfiles/cluster_0_fermilab_beamtest_$RUN_$splitNum..root_data_quality_check.root"

	# in the case for all other split values
	if [ "$splitNum" -ne 0 ] 
	then
		# merge runs
		hadd "Rootfiles/cluster_0_fermilab_beamtest_temp_$RUN..root_data_quality_check.root" "Rootfiles/cluster_0_fermilab_beamtest_prev_$RUN..root_data_quality_check.root" "Rootfiles/cluster_0_fermilab_beamtest_$RUN_$splitNum..root_data_quality_check.root"

		#move output file to different name for next root concatenation
		mv "Rootfiles/cluster_0_fermilab_beamtest_temp_$RUN..root_data_quality_check.root" "Rootfiles/cluster_0_fermilab_beamtest_prev_$RUN..root_data_quality_check.root"

		# delete split runs
		rm "Rootfiles/cluster_0_fermilab_beamtest_$RUN_$splitNum..root_data_quality_check.root"
	
	# in the case for split 0 
	else
	    	mv "Rootfiles/cluster_0_fermilab_beamtest_$RUN_$splitNum..root_data_quality_check.root" "Rootfiles/cluster_0_fermilab_beamtest_prev_$RUN..root_data_quality_check.root"

	fi
done

mv "Rootfiles/cluster_0_fermilab_beamtest_prev_$RUN..root_data_quality_check.root" "Rootfiles/cluster_0_fermilab_beamtest_$RUN..root_data_quality_check.root"

echo "Finished analyzing Run #"$1 

# plot run
ROOT_FILE='scripts/plot_quality_results.cpp("Rootfiles/cluster_0_fermilab_beamtest_'${RUN}'..root_data_quality_check.root")'
root -b -q $ROOT_FILE

# Plot position resolution (ONLY FOR THIN GAP PROJECT) 
posRes="scripts/plot_posRes_fit.cpp(\"Rootfiles/cluster_0_fermilab_beamtest_${RUN}..root_data_quality_check.root\", ${RUN})"
root -b -q  "$posRes"

# show pos res
evince Rootfiles/fitted_positionResolution_$RUN.root.pdf & 

# show run
evince Rootfiles/cluster_0_fermilab_beamtest_$RUN..root_data_quality_check.root.pdf &

