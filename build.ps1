[CmdletBinding()]
<#
Usage: .\build.ps1 [-Backend DX12|GL] [-Threads <int>]
Build the project using CMake with the selected backend and thread count.
#>
param(
    [ValidateSet('DX12','GL')]
    [string]$Backend = 'DX12',

    [int]$Threads 
)

$scriptDir = Split-Path -Path $MyInvocation.MyCommand.Definition -Parent

if (-not $Threads) {
    $Threads = $Env:NUMBER_OF_PROCESSORS
    if (-not $Threads) { $Threads = 1 }
}

$buildDir = "$scriptDir/build"

if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}

# ---------------------------------------------------------------------------
# Configure CMake:  pass the chosen backend as a cache entry
# ---------------------------------------------------------------------------
& cmake -DCMAKE_BUILD_TYPE=Debug `
        -DURCOLO_IMGUI_BACKEND=$Backend `
        -S $scriptDir `
        -B $buildDir

# Use the cross-platform --parallel option so MSBuild / Ninja / Make all work
& cmake --build $buildDir --parallel $Threads
