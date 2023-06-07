#!/bin/bash -i

# if you want to save ROOT files to a specific directory:
#./bin/replay --pedestal database/gem_ped_55.dat --common_mode database/CommonModeRange_55.txt --output_root_path /home/xinzhan/xb /home/xinzhan/evio_data/fermilab_test/fermilab_beamtest_6.evio.0

# if you want to save ROOT files to the default directory: "./Rootfiles/xxxxx.root"
#./bin/replay --pedestal database/gem_ped_94.dat --common_mode database/CommonModeRange_94.txt /home/xinzhan/evio_data/fermilab_test/fermilab_beamtest_96.evio.0



# macOS use following lines
source ~/.zshrc

# clustering replay with tracking
./bin/replay.app/Contents/MacOS/replay --pedestal database/gem_ped_94.dat --common_mode database/CommonModeRange_94.txt ../data/fermilab_beamtest_6.evio.0

# normal hit replay with zero suppression
#./bin/replay.app/Contents/MacOS/replay -c 0 -t 1 -z 0 ../data/fermilab_beamtest_6.evio.0

# convert evio file to root files
#./bin/replay.app/Contents/MacOS/replay -c 1 -t 0 -z 0 ../data/fermilab_beamtest_6.evio.0

