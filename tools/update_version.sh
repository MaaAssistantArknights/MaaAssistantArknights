#!/bin/sh

. ./version.sh

sed -i "s/DEBUG VERSION/"$Version"/" ../src/MeoAssistant/Version.h
