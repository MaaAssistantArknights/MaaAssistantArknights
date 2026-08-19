# 生成 filelist.txt：安装目录内全部文件（除清单自身）的相对路径列表，每行一个，正斜杠分隔。
# 清单随完整包分发；OTA 增量包在文件集合变化时携带新版本清单，应用更新后自动刷新。
# UI 启动时读取清单做存在性检查，跳过 dll/exe/py。
param(
    [Parameter(Mandatory = $true)]
    [string]$InstallDir
)

$resolved = Resolve-Path $InstallDir
$files = Get-ChildItem -Path $resolved.Path -Recurse -File -FollowSymlink |
    Where-Object { $_.Name -ne 'filelist.txt' } |
    ForEach-Object { $_.FullName.Substring($resolved.Path.Length + 1).Replace('\', '/') } |
    Sort-Object
Set-Content -Path (Join-Path $resolved.Path 'filelist.txt') -Value $files -Encoding utf8NoBOM
Write-Host "filelist.txt generated with $($files.Count) entries"
