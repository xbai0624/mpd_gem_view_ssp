#!/bin/tcsh
# define apps root dir
set MY_APPS=/u/home/pcrad/apps
set WORK_APPS=/work/hallb/prad/apps

setenv PATH ${MY_APPS}/bin:${PATH}
setenv LD_LIBRARY_PATH ${MY_APPS}/lib:${LD_LIBRARY_PATH}

# load cmake and gcc first
module use /group/jpsi-007/local/modulefiles
#echo "load cmake 3.20.1"
#module load cmake/3.20.1
echo "load module gcc-8.2.0"
module load gcc/8.2.0

# set these to make sure cmake is using the correct C/C++ compilers
setenv CC `which gcc`
setenv CXX `which g++`

# load qt libraries
#set QT_DIR=${MY_APPS}/qt-5.15.1
#setenv PATH ${QT_DIR}/bin:${PATH}
#setenv LD_LIBRARY_PATH ${QT_DIR}/lib:${LD_LIBRARY_PATH}
#echo "load qt-5.15.1 at ${QT_DIR}"


# load root, only this root is compatible, do not use other version of root in /apps
source ${MY_APPS}/root-6.22.02/bin/thisroot.csh
echo "load root at $ROOTSYS"


set sourced=($_)
set rootdir = `dirname $sourced[2]`
set envdir = `cd $rootdir && pwd`

set beamtestlib = $envdir/lib64
setenv LD_LIBRARY_PATH ${beamtestlib}:${LD_LIBRARY_PATH}

# setup mpd_gem_view_ssp
setenv LD_LIBRARY_PATH ${LD_LIBRARY_PATH}:${PWD}/decoder/lib:${PWD}/gem/lib:${PWD}/epics/lib:${PWD}/tracking_dev/lib

