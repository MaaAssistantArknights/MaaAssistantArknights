if ($args.Length -ne 2) {
    throw "Need two arguments"
}

$fs = Get-ChildItem -Path $args[0] -Filter $args[1]

$index = 0
foreach($f in $fs) {

    $name = $f.Name
    $path = $f.FullName
    Write-Output "::set-output name=name_$index::$name"
    Write-Output "::set-output name=path_$index::$path"

    $index = $index + 1
}
