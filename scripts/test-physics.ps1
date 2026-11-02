$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
Set-Location $projectRoot
cmake --preset physics-only
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
cmake --build --preset physics-only
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
ctest --preset physics-only
exit $LASTEXITCODE
