#!/bin/bash -i

# for linux
#export LD_LIBRARY_PATH=${PWD}/decoder/lib:${PWD}/gem/lib:${PWD}/epics/lib:${PWD}/tracking_dev/lib:${LD_LIBRARY_PATH}

# for macOS
export DYLD_LIBRARY_PATH=${PWD}/decoder/lib:${PWD}/gem/lib:${PWD}/epics/lib:${PWD}/tracking_dev/lib:${DYLD_LIBRARY_PATH}


