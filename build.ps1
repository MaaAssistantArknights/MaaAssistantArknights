if ($args.Length -ne 1) {
    throw "One argument required"
}

$target = $args[0]
$valid_targets = @("ReleaseCore", "ReleaseWpf", "ReleaseResource", "ReleaseBundle", "DevBuild")

if (!$valid_targets.Contains($target)) {
    throw "$target is not a valid target"
}

Set-Location -Path .\src\MeoAssistantBuilder

dotnet run -- --target $target

Set-Location -Path ..\..\
