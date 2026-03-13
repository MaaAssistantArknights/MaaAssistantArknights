$totalClearedSize = 0
$cacheList = gh cache list --json id,key,ref,sizeInBytes,createdAt | ConvertFrom-Json

# Filter for dev-v2 branch caches only
$devCaches = $cacheList | Where-Object { $_.ref -eq "refs/heads/dev-v2" }

if (-not $devCaches) {
    Write-Output "No caches found for dev-v2 branch."
    exit
}

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

foreach ($pattern in $keyPatterns) {
    Write-Output "Processing dev-v2 branch caches for pattern: $pattern"
    
    # Filter for caches matching the current key pattern within dev-v2 branch
    $matchingCaches = $devCaches | Where-Object { $_.key -like "*$pattern*" }

    if (-not $matchingCaches) {
        Write-Output "  No dev-v2 branch caches found for pattern: $pattern"
        continue
    }

    # Sort by creation time (newest first)
    $sortedCaches = $matchingCaches | Sort-Object -Property createdAt -Descending

    # Keep the first one (latest) and delete the rest
    $latestCache = $sortedCaches[0]
    Write-Output "  Keeping latest dev-v2 branch cache: '$($latestCache.key)' (ID: $($latestCache.id))"

    # Delete all except the latest one
    for ($i = 1; $i -lt $sortedCaches.Count; $i++) {
        $cache = $sortedCaches[$i]
        $cacheId = $cache.id
        $cacheKey = $cache.key

        Write-Output "  Deleting dev-v2 branch cache: '$cacheKey' (ID: $cacheId)"
        gh cache delete $cacheId
        
        $totalClearedSize += $cache.sizeInBytes
    }
    
    Write-Output ""
}

$formattedSize = "{0:N2}" -f ($totalClearedSize / 1MB)
Write-Output "Total cleared size: $formattedSize MB"
