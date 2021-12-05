TargetDir="../x64/Release"
LatestTag=$(git describe --tags $(git rev-list --tags --max-count=1))
Package="../x64/MeoAssistance_"$LatestTag".zip"

cp ./*.url $TargetDir
7z.exe a $Package $TargetDir/resource $TargetDir/*.dll $TargetDir/aria2c.exe $TargetDir/MeoAsstGui.exe $TargetDir/*.url