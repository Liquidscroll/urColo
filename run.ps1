[CmdletBinding()]
param(
    [ValidateSet('DX12','GL')]
    [string]$Backend = 'DX12',

    [int]$Threads,

    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$Rest
)

$scriptDir = Split-Path -Path $MyInvocation.MyCommand.Definition -Parent

# ---------- build the argument map for build.ps1 ---------------------------
$buildArgs = @{ Backend = $Backend }
if ($PSBoundParameters.ContainsKey('Threads')) {
    $buildArgs.Threads = $Threads
}

# ---------- invoke build.ps1, forwarding "rest" only if present -----------
if ($Rest -and $Rest.Count -gt 0) {
    & "$scriptDir/build.ps1" @buildArgs @Rest
} else {
    & "$scriptDir/build.ps1" @buildArgs
}

# ---------- locate the executable -----------------------------------------
$exe = Join-Path $scriptDir "build/urColo.exe"
$vsExe = Join-Path $scriptDir "build/Debug/urColo.exe"
if (Test-Path $vsExe) { $exe = $vsExe }

& $exe

