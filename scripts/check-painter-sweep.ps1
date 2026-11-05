# Runs every focused painter leaf, or one shard of them, one check-painter.ps1
# run each, and fails if a leaf outside the known list fails. The one-leaf guard
# allows a single switch per run, and each run compiles the probe itself, so a
# sweep is a loop. nightly.yml runs this on the hosted rasteriser; it also runs
# on a real GPU, where -KnownFailures should be left out.
#
# The plain matrix is not a superset of the leaves: run it separately with
# check-painter.ps1 and no switch. The three Turbulence trial leaves need -TrialPath
# and a private file, so a sweep cannot run them. Three leaves exist only in
# Debug: a Release sweep leaves them out, and -DebugOnlyLeaves runs just those.
[CmdletBinding()]
param(
    [ValidateSet('x64-debug', 'x64-release')]
    [string] $Preset = 'x64-release',
    [ValidateRange(1, 64)]
    [int] $Shards = 1,
    [ValidateRange(0, 63)]
    [int] $Shard = 0,
    # Leaves whose pixel pins hold only on a real GPU: one leaf name per line,
    # without its dash, with the reason after '#'. A listed leaf that passes is
    # reported, not failed.
    [string] $KnownFailures,
    [switch] $DebugOnlyLeaves,
    [string] $OutputDirectory
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot 'check-path.ps1')

if ($Shard -ge $Shards) { throw 'Shard must be less than Shards.' }
if ($DebugOnlyLeaves -and $Preset -ne 'x64-debug') { throw 'DebugOnlyLeaves applies only to x64-debug.' }
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $PSScriptRoot '../out/validation/painter-sweep' }
$root = Join-Path (Resolve-CheckFileSystemPath $OutputDirectory) "$Preset-$Shard-of-$Shards"
$null = New-Item -ItemType Directory -Path $root -Force

$painter = Join-Path $PSScriptRoot 'check-painter.ps1'
$leaves = @((Get-Command $painter).Parameters.Keys | Where-Object { $_ -cmatch '^[A-Z].*Only$' -and $_ -cnotmatch '^Turbulence(Mover|Shoulder|Slab)Only$' -and $_ -cne 'CompileOnly' } | Sort-Object)
if ($leaves.Count -lt 50) { throw "Expected the painter's focused leaves, found $($leaves.Count)." }
# check-painter.ps1 refuses these outside Debug.
$debugLeaves = @('TeamTuneColoursOnly', 'TuneColoursOnly', 'WeaponTuneColoursOnly')
foreach ($name in $debugLeaves) {
    if ($name -cnotin $leaves) { throw "$name is no longer a painter leaf." }
}
$allLeaves = $leaves
if ($DebugOnlyLeaves) { $leaves = $debugLeaves }
elseif ($Preset -ne 'x64-debug') { $leaves = @($leaves | Where-Object { $_ -cnotin $debugLeaves }) }

$known = @()
if ($KnownFailures) {
    $known = @(Get-Content -LiteralPath $KnownFailures | ForEach-Object { ($_ -replace '#.*$', '').Trim() } | Where-Object { $_ })
    foreach ($name in $known) {
        if ($name -cnotin $allLeaves) { throw "Known failure $name is not a painter leaf." }
    }
}

$mine = @(for ($index = $Shard; $index -lt $leaves.Count; $index += $Shards) { $leaves[$index] })
$shell = if ($PSVersionTable.PSEdition -ceq 'Core') { 'pwsh' } else { 'powershell' }
$records = [System.Collections.Generic.List[object]]::new()
foreach ($leaf in $mine) {
    $timer = [Diagnostics.Stopwatch]::StartNew()
    $log = Join-Path $root "$leaf.log"
    # A child shell: a leaf's failure throws, and its scope stays its own.
    $ErrorActionPreference = 'Continue'
    & $shell -NoProfile -File $painter -Preset $Preset -SkipBuild "-$leaf" -OutputDirectory (Join-Path $root 'evidence') *>&1 |
        Out-File -LiteralPath $log -Encoding utf8
    $code = $LASTEXITCODE
    $ErrorActionPreference = 'Stop'
    $records.Add([ordered]@{
        leaf = $leaf
        outcome = if ($code -eq 0) { 'passed' } else { 'failed' }
        known = $leaf -cin $known
        seconds = [Math]::Round($timer.Elapsed.TotalSeconds, 1)
    })
    Write-Host ('{0,-40} {1,-7} {2,7:n1} s' -f $leaf, $records[-1].outcome, $records[-1].seconds)
}

$records | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $root 'sweep-summary.json') -Encoding utf8
$unexpected = @($records | Where-Object { $_.outcome -eq 'failed' -and -not $_.known } | ForEach-Object { $_.leaf })
$recovered = @($records | Where-Object { $_.outcome -eq 'passed' -and $_.known } | ForEach-Object { $_.leaf })
if ($recovered) { Write-Host "Known failures that passed; remove them from the list: $($recovered -join ', ')" }
if ($unexpected) { throw "Painter leaves failed: $($unexpected -join ', '). Logs: $root" }
Write-Host "Painter sweep passed $($mine.Count) leaves on shard $Shard of $Shards."
