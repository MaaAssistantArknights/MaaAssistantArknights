#!/bin/bash

cd "./x64/Release/Official"

baseUrl="https://penguin-stats.io/PenguinStats/api/v2/stages?server="

parameters=("CN" "US" "JP" "KR")

for param in "${parameters[@]}"; do
    {
        if curl -s -o "stages_${param}.json" "${baseUrl}${param}"; then
            echo "Successfully fetched data for ${param}"
        else
            echo "Failed to fetch data for ${param}" >&2
            exit 1
        fi
    } &
done

wait
