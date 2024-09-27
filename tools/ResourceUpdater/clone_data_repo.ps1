$scriptRoot = $PSScriptRoot

Push-Location

New-Item -ItemType Directory -Path "$scriptRoot/x64/Release/" -Force | Out-Null
Set-Location "$scriptRoot/x64/Release/"

if (Test-Path -Path "Official") { Remove-Item "Official" -Recurse -Force }
if (Test-Path -Path "Overseas") { Remove-Item "Overseas" -Recurse -Force }

Write-Host "ArknightsGameResource - Official"
git clone --filter=blob:none --no-checkout --depth 1 --sparse `
    "https://github.com/yuanyan3060/ArknightsGameResource" Official
Set-Location Official
git sparse-checkout set --no-cone `
    "/levels.json" `
    "/item" `
    "/building_skill" `
    "/gamedata/excel/item_table.json" `
    "/gamedata/excel/building_data.json" `
    "/gamedata/excel/range_table.json" `
    "/gamedata/excel/character_table.json" `
    "/gamedata/excel/gacha_table.json" `
    "/gamedata/excel/roguelike_topic_table.json" `
    "/gamedata/excel/activity_table.json"
git checkout

Set-Location ..

Write-Host "`nArknightsGameData_Yostar - Overseas"
git clone --filter=blob:none --no-checkout --depth 1 --sparse `
    "https://github.com/Kengxxiao/ArknightsGameData_Yostar" Overseas
Set-Location Overseas
git sparse-checkout set --no-cone `
    "/en_US/gamedata/excel/item_table.json" `
    "/en_US/gamedata/excel/building_data.json" `
    "/en_US/gamedata/excel/range_table.json" `
    "/en_US/gamedata/excel/character_table.json" `
    "/en_US/gamedata/excel/gacha_table.json" `
    "/en_US/gamedata/excel/roguelike_topic_table.json" `
    "/en_US/gamedata/excel/activity_table.json" `
    "/ja_JP/gamedata/excel/item_table.json" `
    "/ja_JP/gamedata/excel/building_data.json" `
    "/ja_JP/gamedata/excel/range_table.json" `
    "/ja_JP/gamedata/excel/character_table.json" `
    "/ja_JP/gamedata/excel/gacha_table.json" `
    "/ja_JP/gamedata/excel/roguelike_topic_table.json" `
    "/ja_JP/gamedata/excel/activity_table.json" `
    "/ko_KR/gamedata/excel/item_table.json" `
    "/ko_KR/gamedata/excel/building_data.json" `
    "/ko_KR/gamedata/excel/range_table.json" `
    "/ko_KR/gamedata/excel/character_table.json" `
    "/ko_KR/gamedata/excel/gacha_table.json" `
    "/ko_KR/gamedata/excel/roguelike_topic_table.json" `
    "/ko_KR/gamedata/excel/activity_table.json"
git checkout

Write-Host "`narknights-toolbox-update - Taiwan"
git clone --filter=blob:none --no-checkout --depth 1 -b data-tw `
    "https://github.com/arkntools/arknights-toolbox-update" zh_TW/gamedata/excel
Set-Location zh_TW/gamedata/excel
git checkout

Pop-Location

pause
