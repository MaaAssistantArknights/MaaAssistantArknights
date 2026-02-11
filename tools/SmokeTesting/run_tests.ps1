$clients = @("Official", "YoStarJP", "YoStarEN", "YoStarKR", "txwy")
$jobs = @()
$error_client = @()

New-Item -Path "./install/debug" -ItemType Directory -Force -ErrorAction SilentlyContinue > $null

foreach ($client in $clients) {
  $jobs += Start-Job -ScriptBlock {
    param ($client)
    ./install/smoke_test.exe $client > "./install/debug/asst_$client.log"

    if ($LASTEXITCODE -ne 0) {
      return $client
    }
  } -ArgumentList $client
}

foreach ($job in $jobs) {
  $result = Receive-Job -Job $job -Wait
  Remove-Job -Job $job

  if ($result) {
    $error_client += $result
  }
}

Remove-Item ./install/debug/asst.log -ErrorAction SilentlyContinue
foreach ($client in $clients) {
  Get-Content ./install/debug/asst_$client.log | Add-Content ./install/debug/asst.log
}

if ($error_client.Count -gt 0) {
  $pattern = "[ERR]"
  foreach ($client in $error_client) {
    Get-Content ./install/debug/asst_$client.log | ForEach-Object {
      if ($_ -match $pattern) {
        Write-Host $_ -ForegroundColor DarkRed
      } elseif ($_ -match "[INF]") {
        Write-Host $_ -ForegroundColor Cyan
      } else {
        Write-Host $_
      }
    }
  }
  exit 1
}
Write-Host "All tests passed" -ForegroundColor Green
