$scriptRoot = $PSScriptRoot

# Change to the target directory
Set-Location -Path "$scriptRoot/x64/Release/Official"

# Define the base URL
$baseUrl = "https://penguin-stats.io/PenguinStats/api/v2/stages?server="

# Define the parameters
$parameters = @("CN", "US", "JP", "KR")

# Loop through each parameter and make a curl request in parallel
$jobs = @()
foreach ($param in $parameters) {
    $job = Start-Job -ScriptBlock {
        param ($baseUrl, $param)
        $url = "$baseUrl$param"
        $outputFile = "stages_$param.json"
        try {
            Invoke-RestMethod -Uri $url -OutFile $outputFile -ErrorAction Stop
            Write-Output "Successfully fetched data for $param"
        } catch {
            Write-Error "Failed to fetch data for $param"
            exit 1
        }
    } -ArgumentList $baseUrl, $param
    $jobs += $job
}

# Wait for all background jobs to complete
$jobs | ForEach-Object { Receive-Job -Job $_ -Wait }
