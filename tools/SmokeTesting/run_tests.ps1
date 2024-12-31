robocopy "resource" "x64/Debug/resource" /E /XD "Arknights-Tile-Pos" /MT:4 > $null
robocopy "resource/Arknights-Tile-Pos" "x64/Debug/resource/Arknights-Tile-Pos" "overview.json" > $null

$clients = @("Official", "YostarJP", "YostarEN", "YostarKR", "txwy")
$jobs = @()
$error_client = @()

New-Item -Path "./x64/Debug/debug" -ItemType Directory -Force -ErrorAction SilentlyContinue > $null

foreach ($client in $clients) {
  $jobs += Start-Job -ScriptBlock {
    param ($client)
    ./x64/Debug/Sample.exe $client > "./x64/Debug/debug/asst_$client.log"

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

Remove-Item ./x64/Debug/debug/asst.log -ErrorAction SilentlyContinue
foreach ($client in $clients) {
  Get-Content ./x64/Debug/debug/asst_$client.log | Add-Content ./x64/Debug/debug/asst.log
}

if ($error_client.Count -gt 0) {
  $pattern = "[ERR]"
  foreach ($client in $error_client) {
    Get-Content ./x64/Debug/debug/asst_$client.log | ForEach-Object {
      if ($_ -match $pattern) {
        Write-Host $_ -ForegroundColor DarkRed
      } else {
        Write-Host $_
      }
    }
  }
  exit 1
}
Write-Host "All tests passed" -ForegroundColor Green
