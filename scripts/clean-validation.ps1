# Lists, and with -Apply prunes, the check scripts' local run folders (HYG#93).
# A run is a folder named by a 32-digit hex GUID, as every check script names
# its runs, or a painter sweep shard: a folder named <preset>-<n>-of-<m> or
# holding sweep-summary.json, which check-painter-sweep.ps1 fills with one GUID
# leaf per painter run under evidence/<preset>. A shard is one run, kept or
# removed whole, so its summary never points at a leaf that is gone. Runs that
# share a parent folder are one kind (for example
# out/validation/painter/x64-release). Without -Apply nothing is changed: the
# script lists each root and kind by size and age and what -Apply would remove.
# With -Apply it keeps the newest -Keep runs of each kind, by creation time (a
# shard's is its newest of the folder and its summary), and removes the rest;
# -ReceiptsOnly also trims the kept runs to their JSON, log and CSV files, the
# receipts the nightly uploads, except inside a run whose result.json says it
# failed, which keeps its probe as check-painter.ps1 left it. Nothing outside a
# run is touched, so a reused folder such as out/acceptance/<preset> is only
# listed.
[CmdletBinding()]
param(
    # Roots to scan. Default: out/validation, out/acceptance and every out/hyg*
    # folder of this checkout.
    [string[]] $Path,
    [ValidateRange(0, 1000)]
    [int] $Keep = 3,
    [switch] $ReceiptsOnly,
    [switch] $Apply,
    # Also return the plan, one object per run, for a caller to inspect.
    [switch] $PassThru
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot 'check-path.ps1')
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$out = Join-Path $repo 'out'

if (-not $Path) {
    $Path = @((Join-Path $out 'validation'), (Join-Path $out 'acceptance'))
    if (Test-Path -LiteralPath $out -PathType Container) {
        $Path += @(Get-ChildItem -LiteralPath $out -Directory -Filter 'hyg*' | ForEach-Object { $_.FullName })
    }
}
$receipt = @('.json', '.log', '.csv')
$runName = '^[0-9a-fA-F]{32}$'
$shardName = '^.+-\d+-of-\d+$'

function Test-Shard([IO.DirectoryInfo] $Directory) {
    ($Directory.Name -match $shardName) -or (Test-Path -LiteralPath (Join-Path $Directory.FullName 'sweep-summary.json') -PathType Leaf)
}

# When a run was made: a GUID folder's creation time; for a shard, which a later
# sweep with the same output folder reuses, the newer of that and its summary's.
function Get-RunTime([IO.DirectoryInfo] $Run) {
    $time = $Run.CreationTimeUtc
    if ($Run.Name -notmatch $runName) {
        $summary = Join-Path $Run.FullName 'sweep-summary.json'
        if (Test-Path -LiteralPath $summary -PathType Leaf) {
            $written = (Get-Item -LiteralPath $summary -Force).LastWriteTimeUtc
            if ($written -gt $time) { $time = $written }
        }
    }
    return $time
}

# The run itself and any GUID folder inside it whose result.json records a
# failure; a receipts-only trim leaves everything under them alone.
function Get-FailedFolders([IO.DirectoryInfo] $Run) {
    $folders = @($Run) + @(Get-ChildItem -LiteralPath $Run.FullName -Recurse -Directory -Force -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -match $runName })
    foreach ($folder in $folders) {
        $result = Join-Path $folder.FullName 'result.json'
        if (-not (Test-Path -LiteralPath $result -PathType Leaf)) { continue }
        try { $outcome = (Get-Content -LiteralPath $result -Raw | ConvertFrom-Json).outcome } catch { $outcome = $null }
        if ($outcome -eq 'failed') { $folder.FullName }
    }
}

function Get-TreeBytes([string] $Directory) {
    $sum = 0L
    foreach ($file in @(Get-ChildItem -LiteralPath $Directory -Recurse -File -Force -ErrorAction SilentlyContinue)) {
        $sum += $file.Length
    }
    return $sum
}

function Format-Bytes([long] $Bytes) {
    if ($Bytes -ge 1GB) { return '{0:n1} GB' -f ($Bytes / 1GB) }
    if ($Bytes -ge 1MB) { return '{0:n1} MB' -f ($Bytes / 1MB) }
    return '{0:n0} KB' -f ($Bytes / 1KB)
}

# Every run under a root - a GUID folder or a sweep shard - without descending
# into one.
function Find-Runs([string] $Directory) {
    foreach ($child in @(Get-ChildItem -LiteralPath $Directory -Directory -Force -ErrorAction SilentlyContinue)) {
        if (($child.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { continue }
        if ($child.Name -match $runName -or (Test-Shard $child)) { $child } else { Find-Runs $child.FullName }
    }
}

$roots = @(foreach ($entry in $Path) {
    $full = (Resolve-CheckFileSystemPath $entry).TrimEnd('\', '/')
    if ([IO.Path]::GetPathRoot($full).TrimEnd('\', '/') -eq $full) { throw "Refusing a drive root: $full" }
    if (Test-Path -LiteralPath (Join-Path $full '.git')) { throw "Refusing a checkout: $full" }
    $build = Join-Path $out 'build'
    if ($full -eq $build -or $full.StartsWith($build + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing the build tree: $full"
    }
    if (Test-Path -LiteralPath $full -PathType Container) { $full }
})

$plan = [Collections.Generic.List[object]]::new()
$now = [DateTime]::UtcNow
$mode = if ($Apply) { 'Apply' } else { 'Dry run (pass -Apply to change anything)' }
Write-Host "$mode; keep the newest $Keep run(s) of each kind$(if ($ReceiptsOnly) { ', trimmed to their receipts' })."
foreach ($root in $roots) {
    $runs = @(Find-Runs $root)
    Write-Host ''
    Write-Host ('{0}  {1}, {2} run(s)' -f $root, (Format-Bytes (Get-TreeBytes $root)), $runs.Count)
    foreach ($kind in @($runs | Group-Object { $_.Parent.FullName } | Sort-Object Name)) {
        $ordered = @($kind.Group | Sort-Object { Get-RunTime $_ } -Descending)
        $freed = 0L
        $total = 0L
        for ($index = 0; $index -lt $ordered.Count; ++$index) {
            $run = $ordered[$index]
            $bytes = Get-TreeBytes $run.FullName
            $total += $bytes
            $action = if ($index -ge $Keep) { 'remove' } elseif ($ReceiptsOnly) { 'trim' } else { 'keep' }
            $trimmed = @()
            if ($action -eq 'trim') {
                $failed = @(Get-FailedFolders $run)
                $trimmed = @(Get-ChildItem -LiteralPath $run.FullName -Recurse -File -Force |
                    Where-Object { $receipt -notcontains $_.Extension.ToLowerInvariant() } |
                    Where-Object {
                        $file = $_.FullName
                        -not @($failed | Where-Object { $file.StartsWith($_ + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase) }).Count
                    })
                foreach ($file in $trimmed) { $freed += $file.Length }
            } elseif ($action -eq 'remove') { $freed += $bytes }
            $plan.Add([pscustomobject]@{
                root = $root; kind = $kind.Name; run = $run.FullName; action = $action
                bytes = $bytes; created = (Get-RunTime $run); shard = ($run.Name -notmatch $runName)
            })
            if ($Apply -and $action -eq 'remove') {
                Remove-Item -LiteralPath $run.FullName -Recurse -Force
            } elseif ($Apply -and $action -eq 'trim') {
                foreach ($file in $trimmed) { Remove-Item -LiteralPath $file.FullName -Force }
                # Folders left empty by the trim, deepest first.
                foreach ($folder in @(Get-ChildItem -LiteralPath $run.FullName -Recurse -Directory -Force |
                        Sort-Object { $_.FullName.Length } -Descending)) {
                    if (-not (Get-ChildItem -LiteralPath $folder.FullName -Force)) { Remove-Item -LiteralPath $folder.FullName -Force }
                }
            }
        }
        $relative = if ($kind.Name.Length -gt $root.Length) { $kind.Name.Substring($root.Length + 1) } else { '.' }
        $newest = [Math]::Round(($now - (Get-RunTime $ordered[0])).TotalDays, 1)
        $oldest = [Math]::Round(($now - (Get-RunTime $ordered[-1])).TotalDays, 1)
        $verb = if ($Apply) { 'freed' } else { 'would free' }
        Write-Host ('  {0,-48} {1,4} run(s) {2,10}  newest {3} d, oldest {4} d; {5} {6}' -f
            $relative, $ordered.Count, (Format-Bytes $total), $newest, $oldest, $verb, (Format-Bytes $freed))
    }
}
$freedTotal = 0L
foreach ($entry in $plan) {
    if ($entry.action -eq 'remove') { $freedTotal += $entry.bytes }
}
$removed = @($plan | Where-Object { $_.action -eq 'remove' }).Count
Write-Host ''
if ($Apply) { Write-Host "Removed $removed run(s), $(Format-Bytes $freedTotal) before any trim." }
else { Write-Host "Would remove $removed run(s), $(Format-Bytes $freedTotal) before any trim. Nothing was changed." }
if ($PassThru) { $plan }
