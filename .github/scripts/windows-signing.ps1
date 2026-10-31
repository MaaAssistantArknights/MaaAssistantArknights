param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('CheckMetadata', 'Prepare', 'Verify')]
    [string]$Operation,
    [Parameter(Mandatory = $true)]
    [string]$InstallDirectory,
    [string]$ArtifactDirectory,
    [Parameter(Mandatory = $true)]
    [string]$Version,
    [ValidateSet('release-signing', 'test-signing')]
    [string]$SigningPolicy = 'release-signing',
    [string]$ReleaseCertificateSha256
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
[xml]$configuration = Get-Content -LiteralPath "$PSScriptRoot/../../.signpath/windows.xml" -Raw
$files = @($configuration.SelectNodes("//*[local-name()='include']") | ForEach-Object { $_.GetAttribute('path') })
if ($files.Count -eq 0) {
    throw 'The signing configuration does not contain any files.'
}
if ($Operation -ne 'CheckMetadata' -and [string]::IsNullOrWhiteSpace($ArtifactDirectory)) {
    throw 'An artifact directory is required for signing preparation and verification.'
}
if ($Operation -eq 'Verify' -and $SigningPolicy -eq 'release-signing' -and $ReleaseCertificateSha256 -notmatch '^[0-9a-fA-F]{64}$') {
    throw 'SIGNPATH_RELEASE_CERTIFICATE_SHA256 must contain the SHA-256 fingerprint of the issued release certificate.'
}

if ($Operation -eq 'Prepare') {
    New-Item -ItemType Directory -Path $ArtifactDirectory -ErrorAction Stop | Out-Null
} elseif ($Operation -eq 'Verify') {
    $actualFiles = @(Get-ChildItem -LiteralPath $ArtifactDirectory -Recurse -File)
    if ($actualFiles.Count -ne $files.Count -or @($actualFiles | Where-Object { $_.Name -notin $files -or $_.DirectoryName -ne (Resolve-Path $ArtifactDirectory).Path }).Count -ne 0) {
        throw 'The signed artifact does not match the expected file list.'
    }
}

$testStore = $null
$temporaryTestCertificate = $null
$testThumbprint = '3F14001D72B92689CA179B100A67BDA7124F3F6F'
try {
    if ($Operation -eq 'Verify' -and $SigningPolicy -eq 'test-signing') {
        if ($env:GITHUB_ACTIONS -ne 'true' -or $env:RUNNER_ENVIRONMENT -ne 'github-hosted') {
            throw 'Test-certificate trust setup is restricted to disposable GitHub Actions runners.'
        }
        $testCertificate = (Get-AuthenticodeSignature -LiteralPath (Join-Path $ArtifactDirectory $files[0])).SignerCertificate
        if ($null -eq $testCertificate -or $testCertificate.Thumbprint -ne $testThumbprint) {
            throw 'The signing artifact does not use the pinned SignPath test certificate.'
        }
        # Trust only the pinned public test certificate for the duration of verification.
        # This avoids treating PowerShell's generic UnknownError as a valid signature.
        # CurrentUser Root can show a confirmation dialog and block unattended runners.
        $testStore = [System.Security.Cryptography.X509Certificates.X509Store]::new('Root', 'LocalMachine')
        $testStore.Open('ReadWrite')
        if ($testStore.Certificates.Find('FindByThumbprint', $testThumbprint, $false).Count -eq 0) {
            Write-Host 'Temporarily trusting the pinned test certificate on this disposable runner.'
            $testStore.Add($testCertificate)
            $temporaryTestCertificate = $testCertificate
        }
    }

    foreach ($name in $files) {
        $sourceDirectory = if ($Operation -eq 'Verify') { $ArtifactDirectory } else { $InstallDirectory }
        $file = Get-Item -LiteralPath (Join-Path $sourceDirectory $name)
        $metadata = $file.VersionInfo
        if ($metadata.ProductName -cne 'MAA' -or $metadata.ProductVersion -cne $Version -or $metadata.OriginalFilename -cne $name) {
            throw "Unexpected PE metadata in ${name}: product=$($metadata.ProductName), version=$($metadata.ProductVersion), original=$($metadata.OriginalFilename)"
        }

        if ($Operation -eq 'CheckMetadata') {
            continue
        } elseif ($Operation -eq 'Prepare') {
            Copy-Item -LiteralPath $file.FullName -Destination (Join-Path $ArtifactDirectory $name)
            continue
        }

        $signature = Get-AuthenticodeSignature -LiteralPath $file.FullName
        if ($null -eq $signature.SignerCertificate) {
            throw "Missing Authenticode signature: $name"
        }
        if ($SigningPolicy -eq 'release-signing') {
            if ($signature.Status -ne 'Valid' -or $null -eq $signature.TimeStamperCertificate -or $signature.SignerCertificate.GetNameInfo('SimpleName', $false) -cne 'SignPath Foundation' -or $signature.SignerCertificate.GetCertHashString('SHA256') -ine $ReleaseCertificateSha256) {
                throw "Invalid release signature in ${name}: $($signature.Status)"
            }
        } else {
            if ($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Thumbprint -ne $testThumbprint) {
                throw "Invalid test signature in ${name}: $($signature.Status)"
            }
        }
        Write-Host "$name signature verified: $($signature.SignerCertificate.Subject), $($signature.Status)"
    }
} finally {
    if ($null -ne $testStore) {
        if ($null -ne $temporaryTestCertificate) {
            $testStore.Remove($temporaryTestCertificate)
        }
        $testStore.Close()
    }
}

if ($Operation -eq 'Verify') {
    # Replace only our own binaries after every signature has passed validation.
    foreach ($name in $files) {
        Copy-Item -LiteralPath (Join-Path $ArtifactDirectory $name) -Destination (Join-Path $InstallDirectory $name) -Force
    }
}
