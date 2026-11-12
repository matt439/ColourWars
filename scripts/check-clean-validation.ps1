# Checks clean-validation.ps1 against a synthetic run tree: the dry run changes
# nothing, -Apply keeps the newest runs of each kind and removes only runs, a
# painter sweep shard is kept or removed whole, -ReceiptsOnly trims the kept
# runs but not a failed one, and unsafe roots are refused.
# Starts no build, compiler or game.
[CmdletBinding()]
param([string] $OutputDirectory)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot 'check-path.ps1')
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $PSScriptRoot '../out/validation/clean-validation-check' }
# Not a GUID name, so a cleanup of out/validation never takes the fixture's own folder.
$scratch = Join-Path (Resolve-CheckFileSystemPath $OutputDirectory) ('fixture-' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss-fff'))
$cleaner = Join-Path $PSScriptRoot 'clean-validation.ps1'
$checks = 0

function Assert-Clean([bool] $Condition, [string] $Message) {
    if (-not $Condition) { throw "check-clean-validation: $Message" }
    $script:checks++
}

function Set-Age([string] $Folder, [int] $AgeDays) {
    $time = [DateTime]::UtcNow.AddDays(-$AgeDays)
    foreach ($item in @(Get-Item -LiteralPath $Folder) + @(Get-ChildItem -LiteralPath $Folder -Recurse -Force)) {
        $item.CreationTimeUtc = $time
        $item.LastWriteTimeUtc = $time
    }
}

function New-Run([string] $Parent, [int] $AgeDays, [string] $Outcome = 'passed') {
    $run = Join-Path $Parent ([Guid]::NewGuid().ToString('N'))
    $captures = Join-Path $run 'captures'
    $null = New-Item -ItemType Directory -Path $captures -Force
    [IO.File]::WriteAllBytes((Join-Path $run 'painter_visual.exe'), [byte[]]::new(4096))
    [IO.File]::WriteAllBytes((Join-Path $captures 'frame.ppm'), [byte[]]::new(2048))
    [IO.File]::WriteAllText((Join-Path $run 'result.json'), ('{"outcome":"' + $Outcome + '"}'))
    [IO.File]::WriteAllText((Join-Path $run 'run.log'), 'PASS')
    [IO.File]::WriteAllText((Join-Path $captures 'rows.csv'), 'a,b')
    Set-Age $run $AgeDays
    return $run
}

# A check-painter-sweep.ps1 shard: <preset>-<n>-of-<m>/evidence/<preset>/<guid>
# per leaf, the last one failed, plus sweep-summary.json unless it is unfinished.
function New-Shard([string] $Parent, [int] $Index, [int] $AgeDays, [switch] $Unfinished) {
    $shard = Join-Path $Parent "x64-release-$Index-of-4"
    $evidence = Join-Path $shard 'evidence/x64-release'
    $leaves = @(foreach ($outcome in 'passed', 'passed', 'passed', 'passed', 'failed') { New-Run $evidence $AgeDays $outcome })
    if (-not $Unfinished) { [IO.File]::WriteAllText((Join-Path $shard 'sweep-summary.json'), '[]') }
    [IO.File]::WriteAllText((Join-Path $shard 'leaf.log'), 'log')
    Set-Age $shard $AgeDays
    return [pscustomobject]@{ root = $shard; leaves = $leaves; failed = $leaves[-1] }
}

function New-Fixture {
    if (Test-Path -LiteralPath $scratch) { Remove-Item -LiteralPath $scratch -Recurse -Force }
    $tree = [ordered]@{
        painter = @(foreach ($age in 1, 2, 3, 4) { New-Run (Join-Path $scratch 'validation/painter/x64-release') $age })
        startup = @(foreach ($age in 5, 1) { New-Run (Join-Path $scratch 'validation/startup') $age })
        # Shard 2 is unfinished: no summary yet, known by its name.
        shards = @(
            (New-Shard (Join-Path $scratch 'hyg-old/painter-sweep') 0 9),
            (New-Shard (Join-Path $scratch 'hyg-old/painter-sweep') 1 8),
            (New-Shard (Join-Path $scratch 'hyg-old/painter-sweep') 2 7 -Unfinished))
        failed = New-Run (Join-Path $scratch 'validation/painter/x64-debug') 1 'failed'
    }
    # Not runs: a reused per-preset folder and a non-GUID folder.
    $reused = Join-Path $scratch 'acceptance/x64-debug'
    $null = New-Item -ItemType Directory -Path $reused -Force
    [IO.File]::WriteAllBytes((Join-Path $reused 'ammo.exe'), [byte[]]::new(1024))
    $named = Join-Path $scratch 'validation/painter/x64-release/not-a-run'
    $null = New-Item -ItemType Directory -Path $named -Force
    [IO.File]::WriteAllText((Join-Path $named 'keep.txt'), 'kept')
    $tree.others = @((Join-Path $reused 'ammo.exe'), (Join-Path $named 'keep.txt'))
    return $tree
}

function Get-Snapshot {
    @(Get-ChildItem -LiteralPath $scratch -Recurse -Force | ForEach-Object { $_.FullName } | Sort-Object)
}

function Assert-Refused([string[]] $Roots, [string] $Message) {
    $refused = $false
    try { & $cleaner -Path $Roots 6>$null | Out-Null } catch { $refused = $_.Exception.Message.Contains($Message) }
    Assert-Clean $refused "expected the cleaner to refuse with '$Message'"
}

try {
    $roots = @((Join-Path $scratch 'validation'), (Join-Path $scratch 'acceptance'), (Join-Path $scratch 'hyg-old'))

    # Dry run: the plan names the oldest runs, and nothing changes.
    $tree = New-Fixture
    $before = Get-Snapshot
    $plan = @(& $cleaner -Path $roots -Keep 2 -PassThru 6>$null)
    Assert-Clean (@(Compare-Object $before (Get-Snapshot)).Count -eq 0) 'the dry run changed the tree'
    Assert-Clean ($plan.Count -eq 10) "the plan has $($plan.Count) runs, not 10"
    Assert-Clean (@($plan | Where-Object { $_.shard }).Count -eq 3) 'the plan did not see three sweep shards'
    $planned = @($plan | Where-Object { $_.action -eq 'remove' } | ForEach-Object { $_.run } | Sort-Object)
    $expected = @(@($tree.painter[2], $tree.painter[3], $tree.shards[0].root) | Sort-Object)
    Assert-Clean (@(Compare-Object $planned $expected).Count -eq 0) "the dry run planned to remove: $($planned -join ', ')"

    # Apply: the newest two of each kind stay whole, a sweep shard with every
    # leaf, failed or not, and its summary; the rest go; non-runs stay.
    $null = & $cleaner -Path $roots -Keep 2 -Apply 6>$null
    foreach ($run in @($tree.painter[0], $tree.painter[1]) + $tree.startup + $tree.shards[1].leaves + $tree.shards[2].leaves + @($tree.failed)) {
        Assert-Clean (Test-Path -LiteralPath (Join-Path $run 'painter_visual.exe')) "a kept run lost its files: $run"
    }
    Assert-Clean (Test-Path -LiteralPath (Join-Path $tree.shards[1].root 'sweep-summary.json')) 'a kept shard lost its summary'
    foreach ($run in @($tree.painter[2], $tree.painter[3], $tree.shards[0].root)) {
        Assert-Clean (-not (Test-Path -LiteralPath $run)) "an old run was not removed: $run"
    }
    foreach ($file in $tree.others) { Assert-Clean (Test-Path -LiteralPath $file) "a file outside every run was removed: $file" }

    # ReceiptsOnly: kept runs keep only JSON, log and CSV; empty folders go; a
    # failed run, alone or a sweep leaf, keeps its probe and evidence.
    $tree = New-Fixture
    $null = & $cleaner -Path $roots -Keep 1 -ReceiptsOnly -Apply 6>$null
    foreach ($run in @($tree.painter[0], $tree.startup[1]) + @($tree.shards[2].leaves | Select-Object -First 4)) {
        $left = @(Get-ChildItem -LiteralPath $run -Recurse -File | ForEach-Object { $_.Name } | Sort-Object)
        Assert-Clean (($left -join ',') -eq 'result.json,rows.csv,run.log') "a trimmed run kept: $($left -join ', ')"
    }
    foreach ($run in @($tree.failed, $tree.shards[2].failed)) {
        Assert-Clean (Test-Path -LiteralPath (Join-Path $run 'painter_visual.exe')) "the trim took a failed run's probe: $run"
        Assert-Clean (Test-Path -LiteralPath (Join-Path $run 'captures/frame.ppm')) "the trim took a failed run's evidence: $run"
    }
    Assert-Clean (Test-Path -LiteralPath (Join-Path $tree.shards[2].root 'leaf.log')) "the trim took a shard's log"
    $tree2 = $tree.painter[0]
    Assert-Clean (Test-Path -LiteralPath (Join-Path $tree2 'captures')) 'the trim removed a folder that still holds a receipt'
    foreach ($run in @($tree.painter[1], $tree.startup[0], $tree.shards[0].root, $tree.shards[1].root)) {
        Assert-Clean (-not (Test-Path -LiteralPath $run)) "an old run survived ReceiptsOnly: $run"
    }

    # Keep 0 removes every run and nothing else.
    $tree = New-Fixture
    $null = & $cleaner -Path $roots -Keep 0 -Apply 6>$null
    Assert-Clean (@(Get-ChildItem -LiteralPath $scratch -Recurse -Directory | Where-Object { $_.Name -match '^[0-9a-f]{32}$' -or $_.Name -match '-of-' }).Count -eq 0) 'Keep 0 left a run'
    foreach ($file in $tree.others) { Assert-Clean (Test-Path -LiteralPath $file) "Keep 0 removed a file outside every run: $file" }

    # Unsafe roots.
    Assert-Refused @([IO.Path]::GetPathRoot($scratch)) 'Refusing a drive root'
    Assert-Refused @((Join-Path $PSScriptRoot '..')) 'Refusing a checkout'
    Assert-Refused @((Join-Path $PSScriptRoot '../out/build/x64-debug')) 'Refusing the build tree'

    Write-Host "PASS clean-validation: $checks checks; no build, compiler or game started."
}
finally {
    if (Test-Path -LiteralPath $scratch) { Remove-Item -LiteralPath $scratch -Recurse -Force }
}
