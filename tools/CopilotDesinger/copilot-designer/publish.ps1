$DistDir = ".\dist"
$TargetDir = "..\copilot-designer-deprecated"

# Build
Write-Output "Building..."
npm run build
Write-Output "Done."

# Remove old files
Write-Output "Removing old files..."

Remove-Item -Path "$TargetDir\index.html"
Remove-Item -Path "$TargetDir\css\*" -Recurse
Remove-Item -Path "$TargetDir\js\*" -Recurse

Write-Output "Done."

# Copy new files
Write-Output "Copying new files..."
Copy-Item -Path "$DistDir\*" -Destination "$TargetDir" -Recurse -Force
Write-Output "Done."
