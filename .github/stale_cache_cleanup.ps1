$totalClearedSize = 0
$cacheList = gh cache list --json id,key,ref,sizeInBytes,createdAt | ConvertFrom-Json

# List of keys to process
$keyPatterns = @(
    "Windows-x64-nuget",
    "Windows-x64-maadeps",
    "Windows-arm64-nuget",
    "Windows-arm64-maadeps",
    "macOS-x64-maadeps",
    "macOS-arm64-maadeps",
    "Linux-x64-maadeps",
    "Linux-arm64-maadeps",
    "Smoke-testing",
    "prek-v1|Linux|X64"
)

# Filter caches matching any of our key patterns
$matchingCaches = $cacheList | Where-Object { 
    $cache = $_
    ($keyPatterns | Where-Object { $cache.key -like "*$_*" }).Count -gt 0 
}

# Get unique branch references from the matching caches
$branches = $matchingCaches | Select-Object -Property ref -Unique | ForEach-Object { $_.ref }

Write-Output "Found caches across $($branches.Count) branches"

foreach ($branch in $branches) {
    # Extract branch name from ref for display
    $branchName = $branch -replace "refs/heads/", ""
    if ($branch -match "refs/pull/(\d+)/merge") {
        $branchName = "PR #$($matches[1])"
    }
    
    Write-Output "Processing caches for branch: $branchName"
    
    # Filter for current branch caches
    $branchCaches = $matchingCaches | Where-Object { $_.ref -eq $branch }
    
    foreach ($pattern in $keyPatterns) {
        Write-Output "  Processing $branchName branch caches for pattern: $pattern"
        
        # Filter for caches matching the current key pattern within branch
        $patternCaches = $branchCaches | Where-Object { $_.key -like "*$pattern*" }

        if (-not $patternCaches) {
            Write-Output "    No $branchName branch caches found for pattern: $pattern"
            continue
        }

        # Sort by creation time (newest first)
        $sortedCaches = $patternCaches | Sort-Object -Property createdAt -Descending

        # Keep the first one (latest) and delete the rest
        $latestCache = $sortedCaches[0]
        Write-Output "    Keeping latest cache for $branchName/${pattern}: '$($latestCache.key)' (ID: $($latestCache.id))"

        # Delete all except the latest one
        for ($i = 1; $i -lt $sortedCaches.Count; $i++) {
            $cache = $sortedCaches[$i]
            $cacheId = $cache.id
            $cacheKey = $cache.key

            Write-Output "    Deleting cache for $branchName/${pattern}: '$cacheKey' (ID: $cacheId)"
            gh cache delete $cacheId
            
            $totalClearedSize += $cache.sizeInBytes
        }
    }
    
    Write-Output ""
}

$formattedSize = "{0:N2}" -f ($totalClearedSize / 1MB)
Write-Output "Total cleared size: $formattedSize MB"
