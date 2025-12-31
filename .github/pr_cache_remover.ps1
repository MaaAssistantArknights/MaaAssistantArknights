$totalClearedSize = 0
$cacheList = gh cache list --json id,ref,sizeInBytes | ConvertFrom-Json
$branchSet = @{}
git ls-remote --heads origin | ForEach-Object { 
    if ($_ -match 'refs/heads/(.+)$') { $branchSet[$matches[1]] = $true }
}

foreach ($cache in $cacheList) {
    $shouldDelete = $false

    if ($cache.ref -match "refs/pull/(\d+)/merge") {
        $prNumber = $matches[1]
        Write-Host "PR #${prNumber}: " -NoNewline
        
        try {
            $prStatus = (gh pr view $prNumber --json state | ConvertFrom-Json).state
            Write-Host "$prStatus" -NoNewline
            $shouldDelete = $prStatus -in @("MERGED", "CLOSED")
        }
        catch {
            Write-Host "NOT FOUND" -NoNewline
            $shouldDelete = $true
        }
    }
    elseif ($cache.ref -match "refs/heads/(.+)" -and $matches[1] -ne "dev") {
        $branchName = $matches[1]
        $exists = $branchSet.ContainsKey($branchName)
        
        Write-Host "Branch '$branchName': $(if ($exists) { 'EXISTS' } else { 'DELETED' })" -NoNewline
        $shouldDelete = -not $exists
    }
    else {
        continue
    }

    if ($shouldDelete) {
        Write-Host " -> DELETING"
        gh cache delete $cache.id
        $totalClearedSize += $cache.sizeInBytes
    }
    else {
        Write-Host ""
    }
}

Write-Output "Cleared size: $("{0:N2}" -f ($totalClearedSize / 1MB)) MB"
