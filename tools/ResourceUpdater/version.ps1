$hasChanges = $false
$hasPngChanges = $false
$hasVersionChanges = $false

git add .

$allModifiedFiles = git diff --name-only HEAD 2>$null

git reset .

if ($allModifiedFiles.Count -eq 0) {
    Write-Output "No files to update."
}
else {
    $hasChanges = $true

    $modifiedFiles = $allModifiedFiles | Where-Object { $_ -notmatch 'tasks\.json$' }  # Ignore only for version.json updates
    $directories = $modifiedFiles | ForEach-Object { Split-Path $_ } | Sort-Object -Unique

    foreach ($dir in $directories) {
        $versionFile = Join-Path $dir "version.json"
    
        if (Test-Path $versionFile) {
            $json = Get-Content -Path $versionFile | ConvertFrom-Json
            $json.last_updated = (Get-Date).ToString("yyyy-MM-dd HH:mm:ss.fff")
            $jsonFormatted = $json | ConvertTo-Json -Depth 3
            
            $jsonFormatted = $jsonFormatted -replace "  ", "    "
            $jsonFormatted | Set-Content -Path $versionFile
            
            $hasVersionChanges = $true
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
Write-Output "update_resources=$hasVersionChanges" >> $env:GITHUB_OUTPUT
