#!/bin/bash
set -Eeuo pipefail
trap 'echo "Error on line $LINENO"; exit 1' ERR

THIS_DIR=$(cd -- "$(dirname -- "$0")" && pwd)
cd "$THIS_DIR"
source "$THIS_DIR/setup_env.sh"

RUN=${1:?missing RUN}
PEDESTAL=${2:?missing PEDESTAL}
GEM_MAP=${3:-""}
TRACK_CONFIG=${4:-""}
SPLITS=${5:-1}
BATCH_MODE=${6:-0}

for ((splitNum = 0; splitNum < SPLITS; splitNum++)); do
  echo "analyzing run: $RUN, split: $splitNum"

  "$THIS_DIR/bin/replay" \
    -c 0 -t 0 -z 1 -n -1 --tracking on \
    --pedestal       "$THIS_DIR/database/gem_ped_${PEDESTAL}.dat" \
    --common_mode    "$THIS_DIR/database/CommonModeRange_${PEDESTAL}.txt" \
    --gem_map        "$THIS_DIR/database/$GEM_MAP" \
    --tracking_config "$THIS_DIR/config/$TRACK_CONFIG" \
    "/mnt/Dual_OS_Drive/Liyanage_MPGD_Files/DAQ_Analysis_Software/data/fermilab_beamtest_${RUN}.evio.${splitNum}"

  # rename split file
  mv "$THIS_DIR/Rootfiles/cluster_0_fermilab_beamtest_${RUN}..root_data_quality_check.root" \
     "$THIS_DIR/Rootfiles/cluster_0_fermilab_beamtest_${RUN}_${splitNum}..root_data_quality_check.root"

  if (( splitNum != 0 )); then
    # merge previous + current → temp
    hadd "$THIS_DIR/Rootfiles/cluster_0_fermilab_beamtest_temp_${RUN}..root_data_quality_check.root" \
         "$THIS_DIR/Rootfiles/cluster_0_fermilab_beamtest_prev_${RUN}..root_data_quality_check.root" \
         "$THIS_DIR/Rootfiles/cluster_0_fermilab_beamtest_${RUN}_${splitNum}..root_data_quality_check.root"

    # promote temp to prev for next iteration
    mv "$THIS_DIR/Rootfiles/cluster_0_fermilab_beamtest_temp_${RUN}..root_data_quality_check.root" \
       "$THIS_DIR/Rootfiles/cluster_0_fermilab_beamtest_prev_${RUN}..root_data_quality_check.root"

    rm -f "$THIS_DIR/Rootfiles/cluster_0_fermilab_beamtest_${RUN}_${splitNum}..root_data_quality_check.root"
  else
    mv "$THIS_DIR/Rootfiles/cluster_0_fermilab_beamtest_${RUN}_${splitNum}..root_data_quality_check.root" \
       "$THIS_DIR/Rootfiles/cluster_0_fermilab_beamtest_prev_${RUN}..root_data_quality_check.root"
  fi
done

# final name
mv "$THIS_DIR/Rootfiles/cluster_0_fermilab_beamtest_prev_${RUN}..root_data_quality_check.root" \
   "$THIS_DIR/Rootfiles/cluster_0_fermilab_beamtest_${RUN}..root_data_quality_check.root"

echo "Finished analyzing Run #$RUN"

ROOTFILE="$THIS_DIR/Rootfiles/cluster_0_fermilab_beamtest_${RUN}..root_data_quality_check.root"

# ROOT in batch; quote arguments fully (absolute paths)
root -b -q "$THIS_DIR/scripts/plot_quality_results.cpp(\"$ROOTFILE\")"
root -b -q "$THIS_DIR/scripts/plot_posRes_fit.cpp(\"$ROOTFILE\", $RUN)"

# In parallel runs, do NOT open viewers
if [[ "$BATCH_MODE" -eq 0 ]]; then
  # launch viewers only when running interactively / single job
  evince "$THIS_DIR/Rootfiles/fitted_positionResolution_${RUN}.root.pdf" &
  evince "$THIS_DIR/Rootfiles/cluster_0_fermilab_beamtest_${RUN}..root_data_quality_check.root.pdf" &
fi
