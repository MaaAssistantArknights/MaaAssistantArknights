param(
    [Parameter(Mandatory = $true)]
    [string]$InstallDirectory,
    [Parameter(Mandatory = $true)]
    [string]$OutputPath
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = (Resolve-Path -LiteralPath $InstallDirectory).Path
$report = @(Get-ChildItem -LiteralPath $root -Recurse -File |
    Where-Object { $_.Extension -in @('.exe', '.dll') } |
    Sort-Object FullName |
    ForEach-Object {
        $signature = Get-AuthenticodeSignature -LiteralPath $_.FullName
        [pscustomobject]@{
            File = $_.FullName.Substring($root.Length + 1).Replace('\', '/')
            Status = [string]$signature.Status
            Signer = if ($signature.SignerCertificate) { $signature.SignerCertificate.Subject } else { $null }
            CertificateSha256 = if ($signature.SignerCertificate) { $signature.SignerCertificate.GetCertHashString('SHA256') } else { $null }
            Product = $_.VersionInfo.ProductName
            Version = $_.VersionInfo.ProductVersion
            OriginalFilename = $_.VersionInfo.OriginalFilename
        }
    })
ConvertTo-Json -InputObject $report -Depth 4 | Set-Content -LiteralPath $OutputPath -Encoding utf8
$report | Group-Object Status | ForEach-Object { Write-Host "$($_.Name): $($_.Count) PE files" }
Write-Host "Signature audit saved to $OutputPath. Unsigned dependencies are not automatically submitted for signing."
