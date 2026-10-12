# Builds the reproductions from this checkout and runs every case, each in its own process (a defect may crash or
# hang). Run from a Visual Studio developer shell, or anywhere CMake can find a compiler:
#
#     pwsh cbe/repro/run.ps1 [-BuildDir <dir>]
param([string]$BuildDir = "$PSScriptRoot/../../build-cbe-repro")

$ErrorActionPreference = 'Stop'
cmake -S $PSScriptRoot -B $BuildDir | Out-Null
cmake --build $BuildDir --config Release | Out-Null
if ($LASTEXITCODE -ne 0) { throw 'the reproductions did not build' }

$exe = Get-ChildItem $BuildDir -Recurse -Filter 'repro*' | Where-Object { $_.Extension -in '.exe', '' -and -not $_.PSIsContainer } |
    Select-Object -First 1
$cases = 'reorder-loop', 'coverage-loop', 'reorder-out-of-range', 'coverage-out-of-range', 'create-out-of-range',
    'stress-no-actor'
$failed = 0
foreach ($case in $cases) {
    $output = & $exe.FullName $case 2>&1
    $code = $LASTEXITCODE
    $last = ($output | Select-Object -Last 1)
    $verdict = switch ($code) { 0 { 'fixed' } default { "DEFECT: $last" } }
    if ($code -ne 0) { $failed++ }
    '{0,-24} {1}' -f $case, $verdict
}
if ($failed -gt 0) { exit 1 }
