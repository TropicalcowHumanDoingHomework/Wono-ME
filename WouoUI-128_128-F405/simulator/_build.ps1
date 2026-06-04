$simDir = $PSScriptRoot
Write-Host "Building in: $simDir"

# Clean and reconfigure
Remove-Item -Path "$simDir/build" -Recurse -Force -ErrorAction SilentlyContinue
cmake -B "$simDir/build" -S "$simDir" -G "Ninja" 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "CMAKE CONFIGURE FAILED"
    exit $LASTEXITCODE
}

# Build
cmake --build "$simDir/build" --config Release 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "CMAKE BUILD FAILED"
    exit $LASTEXITCODE
}

Write-Host "BUILD SUCCEEDED"
