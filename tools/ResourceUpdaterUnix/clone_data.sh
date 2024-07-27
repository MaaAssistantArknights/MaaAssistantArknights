#!/bin/bash

# Set initial location
initialLocation=$(pwd)

# Create necessary directories
mkdir -p build/
cd build/

# Remove existing directories if they exist
if [ -d "Official" ]; then rm -rf "Official"; fi
if [ -d "Overseas" ]; then rm -rf "Overseas"; fi

# Clone Official repository
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

# Go back to Release directory
cd ..

# Clone Overseas repository
echo -e "\nArknightsGameData_Yostar - Overseas"
git clone --filter=blob:none --no-checkout --depth 1 --sparse \
    "https://github.com/Kengxxiao/ArknightsGameData_Yostar" Overseas
cd Overseas
git sparse-checkout set --no-cone \
    "/en_US/gamedata/excel/item_table.json" \
    "/en_US/gamedata/excel/building_data.json" \
    "/en_US/gamedata/excel/range_table.json" \
    "/en_US/gamedata/excel/character_table.json" \
    "/en_US/gamedata/excel/gacha_table.json" \
    "/en_US/gamedata/excel/roguelike_topic_table.json" \
    "/en_US/gamedata/excel/activity_table.json" \
    "/ja_JP/gamedata/excel/item_table.json" \
    "/ja_JP/gamedata/excel/building_data.json" \
    "/ja_JP/gamedata/excel/range_table.json" \
    "/ja_JP/gamedata/excel/character_table.json" \
    "/ja_JP/gamedata/excel/gacha_table.json" \
    "/ja_JP/gamedata/excel/roguelike_topic_table.json" \
    "/ja_JP/gamedata/excel/activity_table.json" \
    "/ko_KR/gamedata/excel/item_table.json" \
    "/ko_KR/gamedata/excel/building_data.json" \
    "/ko_KR/gamedata/excel/range_table.json" \
    "/ko_KR/gamedata/excel/character_table.json" \
    "/ko_KR/gamedata/excel/gacha_table.json" \
    "/ko_KR/gamedata/excel/roguelike_topic_table.json" \
    "/ko_KR/gamedata/excel/activity_table.json"
git checkout

# Clone Taiwan repository
echo -e "\narknights-toolbox-update - Taiwan"
git clone --filter=blob:none --no-checkout --depth 1 -b data-tw \
    "https://github.com/arkntools/arknights-toolbox-update" zh_TW/gamedata/excel
cd zh_TW/gamedata/excel
git checkout

# Return to the initial location
cd "$initialLocation"
