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

echo "rm -rf gem/lib/*"
rm -rf gem/lib/*
echo "rm -rf tracking_dev/lib/*"
rm -rf tracking_dev/lib/*
echo "rm .DS_Store"
rm .DS_Store

echo "rm -rf Makefile"
rm -rf Makefile
