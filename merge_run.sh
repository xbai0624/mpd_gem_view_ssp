#!/bin/bash

THIS_DIR=$(dirname "$0") 
THIS_DIR=$(cd "$THIS_DIR" && pwd)

source "$THIS_DIR/setup_env.sh"

RUN=$1
PEDESTAL=$2
GEM_MAP=${3:-""}
TRACK_CONFIG=${4:-""}
SPLITS=${5:-1}
BATCH_MODE=${6:-0}

for ((splitNum = 0; splitNum < $SPLITS; splitNum++))
do
	# replay run
	
	echo "analyzing run:" $RUN 
        echo "split #:" $splitNum 

	"$THIS_DIR/bin/replay" -c 0 -t 0 -z 1 -n -1 --tracking on --pedestal "$THIS_DIR/database/gem_ped_$PEDESTAL.dat" --common_mode "$THIS_DIR/database/CommonModeRange_$PEDESTAL.txt" --gem_map "$THIS_DIR/database/$GEM_MAP" --tracking_config "$THIS_DIR/config/$TRACK_CONFIG" "/media/minh-dao/Dual_OS_Drive/Liyanage_MPGD_Files/DAQ_Analysis_Software/data/fermilab_beamtest_$RUN.evio.$splitNum"
	# rename runs
        mv "$THIS_DIR/Rootfiles/cluster_0_fermilab_beamtest_$RUN..root_data_quality_check.root" "$THIS_DIR/Rootfiles/cluster_0_fermilab_beamtest_$RUN_$splitNum..root_data_quality_check.root"

	# in the case for all other split values
	if [ "$splitNum" -ne 0 ] 
	then
		# merge runs
		hadd "$THIS_DIR/Rootfiles/cluster_0_fermilab_beamtest_temp_$RUN..root_data_quality_check.root" "$THIS_DIR/Rootfiles/cluster_0_fermilab_beamtest_prev_$RUN..root_data_quality_check.root" "$THIS_DIR/Rootfiles/cluster_0_fermilab_beamtest_$RUN_$splitNum..root_data_quality_check.root"

		#move output file to different name for next root concatenation
		mv "$THIS_DIR/Rootfiles/cluster_0_fermilab_beamtest_temp_$RUN..root_data_quality_check.root" "$THIS_DIR/Rootfiles/cluster_0_fermilab_beamtest_prev_$RUN..root_data_quality_check.root"

		# delete split runs
		rm "$THIS_DIR/Rootfiles/cluster_0_fermilab_beamtest_$RUN_$splitNum..root_data_quality_check.root"
	
	# in the case for split 0 
	else
	    	mv "$THIS_DIR/Rootfiles/cluster_0_fermilab_beamtest_$RUN_$splitNum..root_data_quality_check.root" "$THIS_DIR/Rootfiles/cluster_0_fermilab_beamtest_prev_$RUN..root_data_quality_check.root"

	fi
done

mv "$THIS_DIR/Rootfiles/cluster_0_fermilab_beamtest_prev_$RUN..root_data_quality_check.root" "$THIS_DIR/Rootfiles/cluster_0_fermilab_beamtest_$RUN..root_data_quality_check.root"

echo "Finished analyzing Run #"$1 

# plot run
ROOT_FILE=''$THIS_DIR'/scripts/plot_quality_results.cpp("Rootfiles/cluster_0_fermilab_beamtest_'${RUN}'..root_data_quality_check.root")'
root -b -q $ROOT_FILE

# Plot position resolution (ONLY FOR THIN GAP PROJECT) 
posRes="$THIS_DIR/scripts/plot_posRes_fit.cpp(\"Rootfiles/cluster_0_fermilab_beamtest_${RUN}..root_data_quality_check.root\", ${RUN})"
root -b -q  "$posRes"

if [ "$BATCH_MODE" -eq 0 ]
then
	# show pos res
	evince $'THIS_DIR'/Rootfiles/fitted_positionResolution_$RUN.root.pdf & 

	# show run
	evince $'THIS_DIR'/Rootfiles/cluster_0_fermilab_beamtest_$RUN..root_data_quality_check.root.pdf &
fi
