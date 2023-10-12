$tag = (Invoke-Expression "git describe --abbrev=0 --tags") -replace '.*?/', ''
$tag -match '\d+\.\d+\.\d+'
(Get-Content ..\MaaWpfGui\MaaWpfGui.csproj) |
  ForEach-Object {
    $_ -replace '(?<a>(<(Assembly|File)Version>)).*?(?<b>(</(Assembly|File)Version>))', ('${a}' + $Matches[0] + '${b}')`
      -replace '(?<a>(<InformationalVersion>)).*?(?<b>(</InformationalVersion>))', ('${a}' + $tag + '${b}')
  } |
    Set-Content ..\MaaWpfGui\MaaWpfGui.csproj
