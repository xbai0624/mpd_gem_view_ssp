#!/bin/bash

folders=(alignment decoder epics fadc gem gui replay tracking_dev .)

echo "start"

for i in ${folders[@]}
do
    echo "rm -rf $i/.qmake.stash"
    rm -rf $i/.qmake.stash
    echo "rm -rf $i/Makefile"
    rm -rf $i/Makefile
    echo "rm -rf $i/obj"
    rm -rf $i/obj/*.o
done

echo "rm -rf Makefile"
rm -rf Makefile
