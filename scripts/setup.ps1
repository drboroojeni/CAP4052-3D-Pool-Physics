[CmdletBinding()]
param(
    [ValidateSet('x64', 'arm64')][string]$Architecture = 'x64',
    [switch]$NoRun
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$projectRoot = Split-Path -Parent $PSScriptRoot

function Invoke-Checked {
    param([string]$FilePath, [string[]]$Arguments)
    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) { throw "Command failed ($LASTEXITCODE): $FilePath $($Arguments -join ' ')" }
}

try {
    Set-Location $projectRoot
    if (Test-Path (Join-Path $projectRoot 'CMakeUserPresets.json')) {
        throw 'This folder contains CMakeUserPresets.json from an earlier setup. Extract the revised ZIP into a fresh folder, then open it in VS 2026 or rerun this optional helper.'
    }
    if ($projectRoot.Length -gt 100) {
        Write-Warning 'Use a short extracted path such as C:\CAP4052\DF5Pool to avoid Windows build path-length errors.'
    }
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (!(Test-Path $vswhere)) {
        throw 'Visual Studio Installer was not found. Install Visual Studio 2026 with Desktop development with C++, C++ CMake tools, and a Windows SDK.'
    }
    $installationsJson = & $vswhere -all -products '*' -version '[18.0,19.0)' -format json -utf8
    if ($LASTEXITCODE -ne 0) { throw 'vswhere failed while locating Visual Studio 2026.' }
    $installations = @($installationsJson | ConvertFrom-Json)
    if ($installations.Count -eq 0) {
        throw 'Visual Studio 2026 / Build Tools 2026 was not found. Use its installer to add Desktop development with C++.'
    }
    $vsRoot = $null
    $hostArch = $null
    foreach ($installation in ($installations | Sort-Object installationVersion -Descending)) {
        $candidateRoot = $installation.installationPath
        $toolsRoot = Join-Path $candidateRoot 'VC\Tools\MSVC'
        if (!(Test-Path $toolsRoot)) { continue }
        foreach ($toolVersion in (Get-ChildItem $toolsRoot -Directory | Sort-Object Name -Descending)) {
            $targetFolder = if ($Architecture -eq 'x64') { 'x64' } else { 'arm64' }
            $hosts = if ($env:PROCESSOR_ARCHITECTURE -eq 'ARM64' -or $env:PROCESSOR_ARCHITEW6432 -eq 'ARM64') {
                @('arm64', 'x64')
            } else { @('x64') }
            foreach ($hostCandidate in $hosts) {
                $compiler = Join-Path $toolVersion.FullName "bin\Host$hostCandidate\$targetFolder\cl.exe"
                if (Test-Path $compiler) {
                    $vsRoot = $candidateRoot
                    $hostArch = if ($hostCandidate -eq 'x64') { 'amd64' } else { 'arm64' }
                    break
                }
            }
            if ($vsRoot) { break }
        }
        if ($vsRoot) { break }
    }
    if (!$vsRoot) {
        if ($Architecture -eq 'arm64') {
            throw 'No installed ARM64 C++ target compiler was found. Add the ARM64 C++ tools in Visual Studio Installer, or rerun without -Architecture arm64 to use the x64 build under Windows ARM emulation.'
        }
        throw 'No x64 C++ compiler was found in VS 2026. Add MSVC x64/x86 build tools and a Windows SDK in Visual Studio Installer.'
    }
    $targetArch = if ($Architecture -eq 'x64') { 'amd64' } else { 'arm64' }
    $devCmd = Join-Path $vsRoot 'Common7\Tools\VsDevCmd.bat'
    $tempCmd = Join-Path ([IO.Path]::GetTempPath()) ([IO.Path]::GetRandomFileName() + '.cmd')
    try {
        @("@echo off", "call `"$devCmd`" -no_logo -arch=$targetArch -host_arch=$hostArch >nul", 'if errorlevel 1 exit /b 1', 'set') | Set-Content $tempCmd -Encoding ASCII
        $environmentLines = & $env:ComSpec /d /c "`"$tempCmd`""
        if ($LASTEXITCODE -ne 0) { throw 'VS developer environment initialization failed.' }
        foreach ($line in $environmentLines) {
            $equals = $line.IndexOf('=')
            if ($equals -gt 0) {
                [Environment]::SetEnvironmentVariable($line.Substring(0, $equals), $line.Substring($equals + 1), 'Process')
            }
        }
    } finally { Remove-Item $tempCmd -ErrorAction SilentlyContinue }

    $cmakeBin = Join-Path $vsRoot 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin'
    $ninjaBin = Join-Path $vsRoot 'Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja'
    if (!(Test-Path (Join-Path $cmakeBin 'cmake.exe')) -or !(Test-Path (Join-Path $ninjaBin 'ninja.exe'))) {
        throw 'VS CMake/Ninja tools are missing. Add C++ CMake tools for Windows in Visual Studio Installer.'
    }
    $env:PATH = "$cmakeBin;$ninjaBin;$env:PATH"
    $cmake = Join-Path $cmakeBin 'cmake.exe'
    $preset = "windows-$Architecture-debug"
    Write-Host "Configuring $preset. CMake prepares the bundled source libraries on the first run..."
    Invoke-Checked $cmake @('--preset', $preset)
    Invoke-Checked $cmake @('--build', '--preset', $preset, '--parallel', '4')
    $exe = Join-Path $projectRoot "b\$Architecture-debug\CAP4052_DF5_Pool.exe"
    Write-Host "Build succeeded. Executable: $exe"
    Write-Host "In VS 2026, File > Open > Folder: $projectRoot, then choose $preset and CAP4052_DF5_Pool.exe."
    Write-Host "Physics tests are optional. See docs/SETUP_WINDOWS.md to run them separately."
    if (!$NoRun) { Start-Process -FilePath $exe -WorkingDirectory $projectRoot }
} catch {
    Write-Host "SETUP FAILED: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}
