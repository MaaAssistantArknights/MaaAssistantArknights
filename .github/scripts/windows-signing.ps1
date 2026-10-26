param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('Prepare', 'Verify')]
    [string]$Operation,
    [Parameter(Mandatory = $true)]
    [string]$InstallDirectory,
    [Parameter(Mandatory = $true)]
    [string]$ArtifactDirectory,
    [Parameter(Mandatory = $true)]
    [string]$Version,
    [ValidateSet('release-signing', 'test-signing')]
    [string]$SigningPolicy = 'release-signing'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
[xml]$configuration = Get-Content -LiteralPath "$PSScriptRoot/../../.signpath/windows.xml" -Raw
$files = @($configuration.SelectNodes("//*[local-name()='include']") | ForEach-Object { $_.GetAttribute('path') })
if ($files.Count -eq 0) {
    throw 'The signing configuration does not contain any files.'
}

if ($Operation -eq 'Prepare') {
    New-Item -ItemType Directory -Path $ArtifactDirectory -ErrorAction Stop | Out-Null
} else {
    $actualFiles = @(Get-ChildItem -LiteralPath $ArtifactDirectory -Recurse -File)
    if ($actualFiles.Count -ne $files.Count -or @($actualFiles | Where-Object { $_.Name -notin $files -or $_.DirectoryName -ne (Resolve-Path $ArtifactDirectory).Path }).Count -ne 0) {
        throw 'The signed artifact does not match the expected file list.'
    }
}

foreach ($name in $files) {
    $sourceDirectory = if ($Operation -eq 'Prepare') { $InstallDirectory } else { $ArtifactDirectory }
    $file = Get-Item -LiteralPath (Join-Path $sourceDirectory $name)
    $metadata = $file.VersionInfo
    if ($metadata.ProductName -cne 'MAA' -or $metadata.ProductVersion -cne $Version -or $metadata.OriginalFilename -cne $name) {
        throw "Unexpected PE metadata in ${name}: product=$($metadata.ProductName), version=$($metadata.ProductVersion), original=$($metadata.OriginalFilename)"
    }

    if ($Operation -eq 'Prepare') {
        Copy-Item -LiteralPath $file.FullName -Destination (Join-Path $ArtifactDirectory $name)
        continue
    }

    $signature = Get-AuthenticodeSignature -LiteralPath $file.FullName
    if ($null -eq $signature.SignerCertificate) {
        throw "Missing Authenticode signature: $name"
    }
    if ($SigningPolicy -eq 'release-signing') {
        if ($signature.Status -ne 'Valid' -or $null -eq $signature.TimeStamperCertificate -or $signature.SignerCertificate.GetNameInfo('SimpleName', $false) -cne 'SignPath Foundation') {
            throw "Invalid release signature in ${name}: $($signature.Status)"
        }
    } else {
        # This self-signed certificate is only accepted in the unpublished CI smoke test.
        $testThumbprint = '3F14001D72B92689CA179B100A67BDA7124F3F6F'
        if ($signature.Status -notin 'Valid', 'NotTrusted' -or $signature.SignerCertificate.Thumbprint -ne $testThumbprint) {
            throw "Invalid test signature in ${name}: $($signature.Status)"
        }
    }
    Write-Host "$name signature verified: $($signature.SignerCertificate.Subject), $($signature.Status)"
}

if ($Operation -eq 'Verify') {
    # Replace only our own binaries after every signature has passed validation.
    foreach ($name in $files) {
        Copy-Item -LiteralPath (Join-Path $ArtifactDirectory $name) -Destination (Join-Path $InstallDirectory $name) -Force
    }
}
