$scriptRoot = $PSScriptRoot

Push-Location

New-Item -ItemType Directory -Path "$scriptRoot/x64/Release/" -Force | Out-Null
Set-Location "$scriptRoot/x64/Release/"

if (Test-Path -Path "Official") { Remove-Item "Official" -Recurse -Force }
if (Test-Path -Path "Overseas") { Remove-Item "Overseas" -Recurse -Force }

$jobs = @()

# Job for cloning and sparse-checkout of Official repository
$jobs += Start-Job -ScriptBlock {
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
}

# Job for cloning and sparse-checkout of Overseas repository
$jobs += Start-Job -ScriptBlock {
    Write-Host "`nArknightsGameData_Yostar - Overseas"
    git clone --filter=blob:none --no-checkout --depth 1 --sparse `
        "https://github.com/ArknightsAssets/ArknightsGamedata" Overseas
    Set-Location Overseas
    git sparse-checkout set --no-cone `
        "/en/gamedata/excel/item_table.json" `
        "/en/gamedata/excel/building_data.json" `
        "/en/gamedata/excel/range_table.json" `
        "/en/gamedata/excel/character_table.json" `
        "/en/gamedata/excel/gacha_table.json" `
        "/en/gamedata/excel/roguelike_topic_table.json" `
        "/en/gamedata/excel/activity_table.json" `
        "/jp/gamedata/excel/item_table.json" `
        "/jp/gamedata/excel/building_data.json" `
        "/jp/gamedata/excel/range_table.json" `
        "/jp/gamedata/excel/character_table.json" `
        "/jp/gamedata/excel/gacha_table.json" `
        "/jp/gamedata/excel/roguelike_topic_table.json" `
        "/jp/gamedata/excel/activity_table.json" `
        "/kr/gamedata/excel/item_table.json" `
        "/kr/gamedata/excel/building_data.json" `
        "/kr/gamedata/excel/range_table.json" `
        "/kr/gamedata/excel/character_table.json" `
        "/kr/gamedata/excel/gacha_table.json" `
        "/kr/gamedata/excel/roguelike_topic_table.json" `
        "/kr/gamedata/excel/activity_table.json"
    git checkout
}

# Job for cloning and checking out the Taiwan repository
$jobs += Start-Job -ScriptBlock {
    Write-Host "`narknights-toolbox-update - Taiwan"
    git clone --filter=blob:none --no-checkout --depth 1 -b data-tw `
        "https://github.com/arkntools/arknights-toolbox-update" Overseas/tw/gamedata/excel
    Set-Location Overseas/tw/gamedata/excel
    git checkout
    Set-Location ..
}

# Job for fetching data from Penguin Stats
$jobs += Start-Job -ScriptBlock {
    $scriptRoot = $using:scriptRoot

    # Change to the target directory
    Set-Location -Path "$scriptRoot/x64/Release/Overseas/tw/gamedata/excel"

    # Define the base URL
    $baseUrl = "https://penguin-stats.io/PenguinStats/api/v2/stages?server="

    # Define the parameters
    $parameters = @("CN", "US", "JP", "KR")

    # Loop through each parameter and make a curl request in parallel
    $jobs = @()
    foreach ($param in $parameters) {
        $job = Start-Job -ScriptBlock {
            param ($baseUrl, $param)
            $url = "$baseUrl$param"
            $outputFile = "stages_$param.json"
            try {
                Invoke-RestMethod -Uri $url -OutFile $outputFile -ErrorAction Stop
                Write-Output "Successfully fetched data for $param"
            }
            catch {
                Write-Error "Failed to fetch data for $param"
                exit 1
            }
        } -ArgumentList $baseUrl, $param
        $jobs += $job
    }

    # Wait for all background jobs to complete
    $jobs | ForEach-Object { Receive-Job -Job $_ -Wait }
}

# Wait for all jobs to complete
$jobs | ForEach-Object { Receive-Job -Job $_ -Wait }

# Clean up jobs
$jobs | ForEach-Object { Remove-Job -Job $_ }

Pop-Location
