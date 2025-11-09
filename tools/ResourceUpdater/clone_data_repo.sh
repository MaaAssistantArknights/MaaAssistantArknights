#!/usr/bin/env bash

script_root="$(dirname "$(realpath "$0")")"

# Save current directory to return to it later
pushd . > /dev/null

cd "$script_root"
work_dir="$(pwd)"

# Clean up existing directories
[ -d "Official" ] && rm -rf "Official"
[ -d "Overseas" ] && rm -rf "Overseas"

# Array to store all background process IDs
pids=()

# Clone and sparse-checkout Official repository
{
    echo "ArknightsGameResource - Official"
    git clone --filter=blob:none --no-checkout --depth 1 --sparse \
        "https://github.com/yuanyan3060/ArknightsGameResource" Official
    cd Official
    git sparse-checkout set --no-cone \
        "/levels.json" \
        "/item" \
        "/building_skill" \
        "/gamedata/excel/item_table.json" \
        "/gamedata/excel/building_data.json" \
        "/gamedata/excel/range_table.json" \
        "/gamedata/excel/character_table.json" \
        "/gamedata/excel/gacha_table.json" \
        "/gamedata/excel/roguelike_topic_table.json" \
        "/gamedata/excel/activity_table.json"
    git checkout
} &
pids+=($!)

# Clone and sparse-checkout Overseas repository
{
    echo -e "\nArknightsGameData_Yostar - Overseas"
    git clone --filter=blob:none --no-checkout --depth 1 --sparse \
        "https://github.com/ArknightsAssets/ArknightsGamedata" Overseas
    cd Overseas
    git sparse-checkout set --no-cone \
        "/en/gamedata/excel/item_table.json" \
        "/en/gamedata/excel/building_data.json" \
        "/en/gamedata/excel/range_table.json" \
        "/en/gamedata/excel/character_table.json" \
        "/en/gamedata/excel/gacha_table.json" \
        "/en/gamedata/excel/roguelike_topic_table.json" \
        "/en/gamedata/excel/activity_table.json" \
        "/jp/gamedata/excel/item_table.json" \
        "/jp/gamedata/excel/building_data.json" \
        "/jp/gamedata/excel/range_table.json" \
        "/jp/gamedata/excel/character_table.json" \
        "/jp/gamedata/excel/gacha_table.json" \
        "/jp/gamedata/excel/roguelike_topic_table.json" \
        "/jp/gamedata/excel/activity_table.json" \
        "/kr/gamedata/excel/item_table.json" \
        "/kr/gamedata/excel/building_data.json" \
        "/kr/gamedata/excel/range_table.json" \
        "/kr/gamedata/excel/character_table.json" \
        "/kr/gamedata/excel/gacha_table.json" \
        "/kr/gamedata/excel/roguelike_topic_table.json" \
        "/kr/gamedata/excel/activity_table.json"
    git checkout
} &
pids+=($!)

# Clone and checkout the Taiwan repository
{
    echo -e "\narknights-toolbox-update - Taiwan"
    mkdir -p Overseas/tw/gamedata/excel
    cd Overseas/tw/gamedata/excel
    git clone --filter=blob:none --no-checkout --depth 1 -b main \
        "https://github.com/arkntools/arknights-data-tw-for-maa" .
    git checkout
} &
pids+=($!)

# Fetch data from Penguin Stats
{
    mkdir -p "$work_dir/Overseas/tw/gamedata/excel"
    cd "$work_dir/Overseas/tw/gamedata/excel"

    # Define the base URL and parameters
    base_url="https://penguin-stats.io/PenguinStats/api/v2/stages?server="
    parameters=("CN" "US" "JP" "KR")

    # Loop through each parameter and make curl requests
    curl_pids=()
    for param in "${parameters[@]}"; do
        {
            url="${base_url}${param}"
            output_file="stages_${param}.json"
            echo "Fetching data for ${param}..."
            
            if curl -s -o "${output_file}" "${url}"; then
                echo "Successfully fetched data for ${param}"
            else
                echo "Failed to fetch data for ${param}" >&2
                exit 1
            fi
        } &
        curl_pids+=($!)
    done

    # Wait for all curl processes to complete
    for pid in "${curl_pids[@]}"; do
        wait "$pid"
    done
} &
pids+=($!)

# Wait for all main processes to complete
for pid in "${pids[@]}"; do
    wait "$pid"
done

# Return to original directory
popd > /dev/null
