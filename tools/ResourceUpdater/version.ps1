git add .

$modifiedFiles = git diff --name-only HEAD 2>$null | Where-Object { $_ -notmatch 'tasks\.json$'}

$directories = $modifiedFiles | ForEach-Object { Split-Path $_ } | Sort-Object -Unique

foreach ($dir in $directories) {
    $versionFile = Join-Path $dir "version.json"

    if (Test-Path $versionFile) {
        $json = Get-Content -Path $versionFile | ConvertFrom-Json
        $json.last_updated = (Get-Date).ToString("yyyy-MM-dd HH:mm:ss.fff")
        $jsonFormatted = $json | ConvertTo-Json -Depth 3
        
        $jsonFormatted = $jsonFormatted -replace "  ", "    "
        $jsonFormatted | Set-Content -Path $versionFile
        
        Write-Output "Updated: $versionFile"
    }
}

