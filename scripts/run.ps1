[CmdletBinding()]
param([ValidateSet('x64', 'arm64')][string]$Architecture = 'x64')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$exe = Join-Path $projectRoot "b\$Architecture-debug\CAP4052_DF5_Pool.exe"
if (!(Test-Path $exe)) {
    throw "Open this folder in VS 2026, select windows-$Architecture-debug and CAP4052_DF5_Pool.exe, then build. The optional command-line helper is scripts/setup.ps1 -Architecture $Architecture."
}
Start-Process -FilePath $exe -WorkingDirectory $projectRoot
