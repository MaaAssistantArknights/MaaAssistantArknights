#!/bin/sh

LatestTagId=$(git rev-list --tags --max-count=1 --abbrev-commit)
LatestCommitId=$(git rev-list --all --max-count=1 --abbrev-commit)

if [ $LatestTagId = $LatestCommitId ]; then
    Version=$(git describe --tags --match "v*" $LatestTagId)
else
    Version="c""$LatestCommitId"
fi

echo "$Version"
