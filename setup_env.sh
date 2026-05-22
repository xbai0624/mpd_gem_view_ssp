#!/bin/bash -i

# for linux
#export LD_LIBRARY_PATH=${PWD}/decoder/lib:${PWD}/gem/lib:${PWD}/epics/lib:${PWD}/tracking_dev/lib:${PWD}/gui/online_monitor/lib:${LD_LIBRARY_PATH}

# for macOS
# (gui/online_monitor/lib only exists when built with CONFIG+=et; harmless otherwise)
export DYLD_LIBRARY_PATH=${PWD}/decoder/lib:${PWD}/gem/lib:${PWD}/epics/lib:${PWD}/tracking_dev/lib:${PWD}/gui/online_monitor/lib:${DYLD_LIBRARY_PATH}


