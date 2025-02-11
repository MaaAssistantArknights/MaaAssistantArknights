Push-Location

# Stage changes otherwise git diff doesn't work
git add .

# Change to the root directory of the repository
Set-Location -Path (git rev-parse --show-toplevel)

# Check git status
Write-Output "Check git status..."
$gitStatus = git status
Write-Output $gitStatus

Write-Output "-------------------------------------------------------------------------------------------"

# Start to diff the file changes
Write-Output "Start to diff the file changes..."
$gitdiff = git diff --numstat HEAD 2>$null | findstr -i resource
if ($LASTEXITCODE -ne 0) {
    Write-Output "no diff"
    exit 0
}
Write-Output $gitdiff

Write-Output "-------------------------------------------------------------------------------------------"

$diff = $false
$hasPngDiff = $false
$listPerServer = @{}
$mainlandChina = "mainlandChina"

# Parse the diff output
$diffLines = $gitdiff -split "`n"
foreach ($line in $diffLines) {
    $parts = $line -split "\s+"
    $added = $parts[0]
    $deleted = $parts[1]
    $pathname = $parts[2]
    $server = if ($pathname -like "resource/global/*") { $pathname -split "/" | Select-Object -Index 2 } else { $mainlandChina }
    if (-not $listPerServer.ContainsKey($server)) {
        $listPerServer[$server] = @()
    }
    $listPerServer[$server] += , @($added, $deleted, $pathname)
}

# Improved logging for parsed diff
Write-Output "Parsed diff result:"
foreach ($server in $listPerServer.Keys) {
    Write-Output "${server}: ["
    foreach ($entry in $listPerServer[$server]) {
        Write-Output "  [ '$($entry[0])', '$($entry[1])', '$($entry[2])' ],"
    }
    Write-Output "]`n"
}

Write-Output "-------------------------------------------------------------------------------------------"

# Process each server's changes
foreach ($server in $listPerServer.Keys) {
    $serverName = if ($server -eq $mainlandChina) { "[mainlandChina]" } else { "[$server]" }
    $localDiff = $false
    $localHasPngDiff = $false
    foreach ($entry in $listPerServer[$server]) {
        $added = $entry[0]
        $deleted = $entry[1]
        $pathname = $entry[2]
        if ($pathname -like "*.png") {
            $localHasPngDiff = $true
            $localDiff = $true
            Write-Output "$serverName PNG changed: $pathname, valid."
            continue
        }
        if ($added -gt 1 -or $added -ne $deleted) {
            Write-Output "$serverName File changed: $pathname, valid."
            $localDiff = $true
            continue
        }
        if ($pathname -like "*/version.json") {
            $versionDiff = git diff --unified=0 HEAD -- $pathname 2>$null
            $addedLines = @($versionDiff -split "`n" | Where-Object { $_ -match "^\+" -and $_ -notmatch "^\+\+\+" })
            $deletedLines = @($versionDiff -split "`n" | Where-Object { $_ -match "^\-" -and $_ -notmatch "^\-\-\-" })
            if ($addedLines.Count -eq 1 -and $deletedLines.Count -eq 1 -and $addedLines[0] -match "^\+\s*`"last_updated`":" -and $deletedLines[0] -match "^\-\s*`"last_updated`":") {
                Write-Output "$serverName Version-date changed only: $pathname, not valid."
                continue
            }
            else {
                Write-Output "$serverName File changed: $pathname, valid."
                $localDiff = $true
                continue
            }
        }
    }
    if ($localHasPngDiff) {
        $hasPngDiff = $true
    }
    if ($localDiff) {
        $diff = $true
    }
    else {
        Write-Output "$serverName No valid changes found in server: $server"
        Write-Output "$serverName Revert the file changes..."
        foreach ($entry in $listPerServer[$server]) {
            $pathname = $entry[2]
            git checkout HEAD -- $pathname
        }
    }
    Write-Output "$serverName Server check result: { localHasPngDiff = $localHasPngDiff, localDiff = $localDiff }`n"
    Write-Output "-------------------------------------------------------------------------------------------"
}


# Track if push is needed
$update_resources = $false

# Hardcoded paths for tasks.json files
# DO NOT CHANGE THE ORDER
$taskFiles = @(
    "resource/global/YoStarEN/resource/tasks.json",     #EN
    "resource/global/YoStarJP/resource/tasks.json",     #JP
    "resource/global/YoStarKR/resource/tasks.json",     #KR
    "resource/global/txwy/resource/tasks.json"          #TW
)

# Retrieve original files from Git and create *.original versions
$originalFiles = @()
foreach ($relativePath in $taskFiles) {
    $originalPath = "$relativePath.original"
    git show HEAD:"$relativePath" 2>$null | Out-File -Encoding utf8 $originalPath

    if (-not (Test-Path $originalPath)) {
        Write-Output "Original file not found in Git ($relativePath). Assuming new file, push required."
        $update_resources = $true
    } else {
        $originalFiles += $originalPath
    }
}

# If all original files are missing, no need to continue
if ($update_resources) {
    "push=$update_resources" | Out-File -FilePath $githubOutput -Append
    Write-Output "Push required: $update_resources"
    exit 0
}

$global_resources = "EN:$($originalFiles[0]),KR:$($originalFiles[1]),JP:$($originalFiles[2]),TW:$($originalFiles[3])"

# Run Python sorting script on both modified and original files
& py ./tools/TaskSorter/TaskSorter.py --global_resources $global_resources

# Run Prettier on all files
& npx prettier -w $originalFiles[0] $originalFiles[1] $originalFiles[2] $originalFiles[3]

# Compare sorted & formatted versions
foreach ($relativePath in $taskFiles) {
    $originalPath = "$relativePath.original"
    
    $modifiedContent = Get-Content -Raw -Path $relativePath
    $originalContent = Get-Content -Raw -Path $originalPath

    if ($modifiedContent -ne $originalContent) {
        Write-Output "Differences detected in $relativePath, push required."
        $update_resources = $true
    } else {
        Write-Output "No substantial changes in $relativePath."
    }

    # Clean up temp file
    Remove-Item -Force $originalPath
}

Write-Output "Diff check result:"
Write-Output "hasPngDiff: $hasPngDiff"
Write-Output "diff: $diff"
Write-Output "update: $update_resources"
Write-Output "contains_png=$hasPngDiff" >> $env:GITHUB_OUTPUT
Write-Output "changes=$diff" >> $env:GITHUB_OUTPUT
Write-Output "update=$update_resources" >> $env:GITHUB_OUTPUT

Pop-Location
