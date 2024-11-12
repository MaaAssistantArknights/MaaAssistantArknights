#!/bin/sh

TargetDir=""

if [ $1 ]; then
    TargetDir=$1
else
    echo "Please enter TargetDir"
    exit 1
fi

mkdir -p $TargetDir
cp -r "../resource/" $TargetDir
cp -r "../src/Python" $TargetDir
exit 0
