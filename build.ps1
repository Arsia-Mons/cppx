#!/usr/bin/env pwsh
# Easy Windows build entry point for the SDL3/Clay UI reference app.
#
# Usage:
#   ./build.ps1                    # configure/build Debug hello
#   ./build.ps1 -Run               # build then run hello.exe
#   ./build.ps1 -Tests             # build all targets then run ctest
#   ./build.ps1 -Target shooter_ui_tests
#   ./build.ps1 -Config Release
#   ./build.ps1 -Clean

[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release', 'RelWithDebInfo')]
    [string]$Config = 'Debug',

    [string]$Target = 'hello',

    [string]$BuildDir = '',

    [switch]$Clean,

    [switch]$Run,

    [switch]$Tests,

    [switch]$Help,

    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$RunArgs = @()
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Fail($message) {
    Write-Host "build.ps1: $message" -ForegroundColor Red
    exit 1
}

function Info($message) {
    Write-Host "build.ps1: $message" -ForegroundColor Cyan
}

function Usage {
    Write-Host @'
Usage:
  ./build.ps1                    configure/build Debug hello
  ./build.ps1 -Run               build then run hello.exe
  ./build.ps1 -Tests             build all targets then run ctest
  ./build.ps1 -Target <name>     build one target, e.g. shooter_ui_tests
  ./build.ps1 -Config Release    use cmake-build-release
  ./build.ps1 -Clean             remove CMakeCache.txt + CMakeFiles first
'@
}

function Copy-RuntimeDllIfNeeded($dllName, $destinationDir, $searchRoot, $vcpkgRoot) {
    $destination = Join-Path $destinationDir $dllName
    if (Test-Path $destination) {
        return
    }

    $candidatePaths = @()
    if ($vcpkgRoot) {
        $candidatePaths += Join-Path $vcpkgRoot "installed\x64-windows\debug\bin\$dllName"
        $candidatePaths += Join-Path $vcpkgRoot "installed\x64-windows\bin\$dllName"
    }

    foreach ($candidatePath in $candidatePaths) {
        if (Test-Path $candidatePath) {
            Copy-Item -LiteralPath $candidatePath -Destination $destination -Force
            Write-Host "build.ps1: copied $dllName" -ForegroundColor DarkGray
            return
        }
    }

    $found = Get-ChildItem -LiteralPath $searchRoot -Recurse -Filter $dllName -File -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if ($found) {
        Copy-Item -LiteralPath $found.FullName -Destination $destination -Force
        Write-Host "build.ps1: copied $dllName" -ForegroundColor DarkGray
        return
    }

    Write-Host "build.ps1: warning: could not find $dllName to copy beside hello.exe" -ForegroundColor Yellow
}

if ($Help) {
    Usage
    exit 0
}

$repoDir = $PSScriptRoot
if (-not $BuildDir) {
    $suffix = $Config.ToLowerInvariant()
    $BuildDir = Join-Path $repoDir "cmake-build-$suffix"
} elseif (-not [System.IO.Path]::IsPathRooted($BuildDir)) {
    $BuildDir = Join-Path $repoDir $BuildDir
}

$binaryDir = [System.IO.Path]::GetFullPath($BuildDir)
New-Item -ItemType Directory -Force -Path $binaryDir | Out-Null

$repoDirSlash = $repoDir -replace '\\', '/'
$binaryDirSlash = $binaryDir -replace '\\', '/'
$busy = @(Get-CimInstance Win32_Process | Where-Object {
    $_.ProcessId -ne $PID -and
    $_.Name -match '^(cmake|ninja|cl|link|MSBuild|git)\.exe$' -and
    (
        $_.CommandLine -like "*$repoDir*" -or
        $_.CommandLine -like "*$repoDirSlash*" -or
        $_.CommandLine -like "*$binaryDir*" -or
        $_.CommandLine -like "*$binaryDirSlash*"
    )
})
if ($busy) {
    $summary = ($busy | ForEach-Object { "$($_.Name):$($_.ProcessId)" }) -join ', '
    Fail "a build/configure is already active in this repo ($summary). Wait for CLion/CMake/Ninja to finish before running the wrapper."
}

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $vswhere)) {
    Fail "vswhere.exe not found at $vswhere. Install Visual Studio Build Tools with the C++ x64 toolset."
}

$vsPath = (& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath | Select-Object -First 1)
if (-not $vsPath) {
    Fail "No Visual Studio install with the C++ x64 toolset was found."
}

$vcvars = Join-Path $vsPath 'VC\Auxiliary\Build\vcvars64.bat'
if (-not (Test-Path $vcvars)) {
    $vcvars = Join-Path $vsPath 'Common7\Tools\VsDevCmd.bat'
}
if (-not (Test-Path $vcvars)) {
    Fail "Could not find vcvars64.bat or VsDevCmd.bat under $vsPath."
}

$vcpkgRoot = $env:VCPKG_ROOT
if (-not $vcpkgRoot) {
    $vcpkgRoot = [Environment]::GetEnvironmentVariable('VCPKG_ROOT', 'User')
}
if (-not $vcpkgRoot) {
    $candidate = Join-Path $HOME 'vcpkg'
    if (Test-Path $candidate) {
        $vcpkgRoot = $candidate
    }
}

$toolchainArgs = ''
if ($vcpkgRoot) {
    $vcpkgToolchain = Join-Path $vcpkgRoot 'scripts\buildsystems\vcpkg.cmake'
    if (Test-Path $vcpkgToolchain) {
        $toolchainArgs = ' -DCMAKE_TOOLCHAIN_FILE="{0}"' -f $vcpkgToolchain
    } else {
        Write-Host "build.ps1: VCPKG_ROOT is set but no toolchain was found at $vcpkgToolchain" -ForegroundColor Yellow
    }
} else {
    Write-Host "build.ps1: VCPKG_ROOT not set; relying on CMake package discovery for dependencies." -ForegroundColor Yellow
}

$helloExe = Join-Path $binaryDir 'hello.exe'
$runningHello = @(Get-Process -Name 'hello' -ErrorAction SilentlyContinue |
    Where-Object { $_.Path -eq $helloExe })
if ($runningHello) {
    Fail ("hello.exe from $binaryDir is running (PID " + (($runningHello | ForEach-Object Id) -join ', ') + "). Close it before rebuilding.")
}

$lockFile = Join-Path $binaryDir '.ui-build.lock'
$lockStream = $null
$exitCode = 1

try {
    try {
        $lockStream = [System.IO.File]::Open($lockFile, [System.IO.FileMode]::CreateNew, [System.IO.FileAccess]::Write, [System.IO.FileShare]::None)
    } catch [System.IO.IOException] {
        $owner = Get-Content $lockFile -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($owner -and (Get-Process -Id ([int]$owner) -ErrorAction SilentlyContinue)) {
            Fail "another build holds the lock (PID $owner) on $binaryDir."
        }
        Write-Host "build.ps1: reclaiming stale lock in $binaryDir" -ForegroundColor Yellow
        try {
            Remove-Item $lockFile -Force
        } catch {
            Fail "the stale lock file is still held by another process: $lockFile"
        }
        $lockStream = [System.IO.File]::Open($lockFile, [System.IO.FileMode]::CreateNew, [System.IO.FileAccess]::Write, [System.IO.FileShare]::None)
    }

    $writer = [System.IO.StreamWriter]::new($lockStream)
    $writer.WriteLine($PID)
    $writer.WriteLine((Get-Date -Format o))
    $writer.Flush()

    if ($Clean) {
        Info "cleaning cache in $binaryDir"
        Remove-Item (Join-Path $binaryDir 'CMakeCache.txt') -Force -ErrorAction SilentlyContinue
        Remove-Item (Join-Path $binaryDir 'CMakeFiles') -Recurse -Force -ErrorAction SilentlyContinue
    }

    $buildTarget = $Target
    if ($Tests -and -not $PSBoundParameters.ContainsKey('Target')) {
        $buildTarget = 'all'
    }

    Write-Host "build.ps1: VS        = $vsPath" -ForegroundColor DarkGray
    if ($vcpkgRoot) {
        Write-Host "build.ps1: VCPKG_ROOT= $vcpkgRoot" -ForegroundColor DarkGray
    }
    Write-Host "build.ps1: build dir = $binaryDir" -ForegroundColor DarkGray
    Write-Host "build.ps1: target    = $buildTarget" -ForegroundColor DarkGray

    $configure = 'cmake -S "{0}" -B "{1}" -G Ninja -DCMAKE_BUILD_TYPE={2}{3}' -f $repoDir, $binaryDir, $Config, $toolchainArgs
    $build = 'cmake --build "{0}" --target "{1}" --config {2}' -f $binaryDir, $buildTarget, $Config
    $inner = 'call "{0}" >nul && cd /d "{1}" && {2} && {3}' -f $vcvars, $repoDir, $configure, $build
    if ($vcpkgRoot) {
        $inner = 'call "{0}" >nul && set "VCPKG_ROOT={1}" && cd /d "{2}" && {3} && {4}' -f $vcvars, $vcpkgRoot, $repoDir, $configure, $build
    }

    & cmd /d /s /c $inner
    $exitCode = $LASTEXITCODE
    if ($exitCode -ne 0) {
        Fail "build failed (exit $exitCode)."
    }

    if (($buildTarget -eq 'hello' -or $buildTarget -eq 'all' -or $Run) -and (Test-Path $helloExe)) {
        Copy-RuntimeDllIfNeeded 'SDL3.dll' $binaryDir $binaryDir $vcpkgRoot
        Copy-RuntimeDllIfNeeded 'SDL3_ttf.dll' $binaryDir $binaryDir $vcpkgRoot
    }

    if ($Tests) {
        Info "running tests"
        & ctest --test-dir $binaryDir --output-on-failure -C $Config
        $exitCode = $LASTEXITCODE
        if ($exitCode -ne 0) {
            Fail "tests failed (exit $exitCode)."
        }
    }
}
finally {
    if ($lockStream) {
        $lockStream.Close()
    }
    Remove-Item $lockFile -Force -ErrorAction SilentlyContinue
}

Write-Host "build.ps1: OK ($Config/$buildTarget)" -ForegroundColor Green

if ($Run) {
    if (-not (Test-Path $helloExe)) {
        Fail "cannot run because $helloExe was not produced."
    }
    Info "running $helloExe"
    & $helloExe @RunArgs
}
