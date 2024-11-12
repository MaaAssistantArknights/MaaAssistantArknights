#!/bin/sh

. ./version.sh

sed -i "s/DEBUG VERSION/"$Version"/" ../src/MaaCore/Version.h
