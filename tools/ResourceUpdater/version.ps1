$hasChanges = $false
$hasPngChanges = $false

git add .

$allModifiedFiles = git diff --name-only HEAD 2>$null

git reset .

if ($allModifiedFiles.Count -eq 0) {
    Write-Output "No files to update."
}
else {
    $hasChanges = $true
    $modifiedFiles = $allModifiedFiles | Where-Object { $_ -notmatch '^resource([\\/]global[\\/][^\\\/]+[\\/]resource)?[\\/]tasks[\\/]' }   
    $directories = $modifiedFiles | ForEach-Object { Split-Path $_ } | Sort-Object -Unique
    
    # Build a list of directories containing version.json files to update
    $versionJsonDirs = @()
    
    foreach ($dir in $directories) {
        # If this is Arknights-Tile-Pos folder, we need its parent's version.json
        if ($dir -match '\\Arknights-Tile-Pos$') {
            $versionJsonDirs += Split-Path $dir
        }
        # Otherwise check for version.json in the current directory
        else {
            $versionJsonDirs += $dir
        }
    }
    
    # Remove duplicates
    $versionJsonDirs = $versionJsonDirs | Select-Object -Unique
    
    # Update all version.json files
    foreach ($dir in $versionJsonDirs) {
        $versionFile = Join-Path $dir "version.json"
    
        if (Test-Path $versionFile) {
            $json = Get-Content -Path $versionFile | ConvertFrom-Json
            $json.last_updated = (Get-Date).ToUniversalTime().ToString("yyyy-MM-dd HH:mm:ss.fff")
            $jsonFormatted = $json | ConvertTo-Json -Depth 3
            
            $jsonFormatted = $jsonFormatted -replace "  ", "    "
            $jsonFormatted | Set-Content -Path $versionFile
            
            Write-Output "Updated: $versionFile"
        }
    }

    # Check for PNG changes
    if ($allModifiedFiles | Where-Object { $_ -match '\.png$' }) {
        $hasPngChanges = $true
    }
}

Write-Output "Changes: $hasChanges"
Write-Output "PNG Changes: $hasPngChanges"
Write-Output "changes=$hasChanges" >> $env:GITHUB_OUTPUT
Write-Output "contains_png=$hasPngChanges" >> $env:GITHUB_OUTPUT
