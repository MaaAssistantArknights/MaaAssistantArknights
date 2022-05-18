#!/bin/sh

. ./version.sh

Package="../x64/MaaBundle-Windows-x64-"$Version".zip"

TargetDir="../x64/Release"

7z.exe a $Package $TargetDir/resource $TargetDir/*.dll $TargetDir/Python $TargetDir/*.exe
