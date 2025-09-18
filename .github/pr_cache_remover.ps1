$totalClearedSize = 0
$cacheList = gh cache list --json id,ref,sizeInBytes | ConvertFrom-Json
$prCaches = $cacheList | Where-Object { $_.ref -like "*merge*" }

if (-not $prCaches) {
    Write-Output "No PR caches found."
    exit
}

foreach ($cache in $prCaches) {
    $cacheId = $cache.id
    $cacheRef = $cache.ref
    $cacheSizeInBytes = $cache.sizeInBytes  

    if ($cacheRef -match "refs/pull/(\d+)/merge") {
        $prNumber = $matches[1]
 
        Write-Host "PR #${prNumber}: " -NoNewline
 
        $prStatus = gh pr view $prNumber --json state | ConvertFrom-Json
 
        Write-Host $prStatus.state -NoNewline
 
        if ($prStatus.state -eq "MERGED" -or $prStatus.state -eq "CLOSED") {
            Write-Host " -> DELETING"
            
            gh cache delete $cacheId

            $totalClearedSize += $cacheSizeInBytes
        }
        else {
            Write-Host ""
        }
    }
}

$formattedSize = "{0:N2}" -f ($totalClearedSize / 1MB)
Write-Output "Cleared size: $formattedSize MB"
