$tag = (Invoke-Expression "git describe --abbrev=0 --tags") -replace '.*?/', ''
$tag -match '\d+(\.\d+){0,3}'
$match = $Matches[0]
$props = "<Project>
 <PropertyGroup>
    <AssemblyVersion>$match</AssemblyVersion>
    <FileVersion>$match</FileVersion>
    <InformationalVersion>$tag</InformationalVersion>
    <Version>$match</Version>
 </PropertyGroup>
</Project>"
Set-Content -Path Directory.Build.props -Value $props
