# Builds the x64-analyze preset (MSVC /analyze over a Debug build) and counts
# its warnings. A report, not a check: it fails only when the configure or the
# build itself fails, never for what the analysis says.
#
# -Baseline compares this repository's warnings with a committed list
# (report-analyze-baseline.json, HYG#95) and reports new and vanished entries
# in analyze-summary.json, as GitHub annotations and in the job summary.
# -WarningsLog recounts an earlier run's analyze-warnings.log from this
# checkout instead of building, so a comparison can be tried in seconds.
[CmdletBinding()]
param([string] $OutputDirectory, [string] $Baseline, [string] $WarningsLog)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot 'check-path.ps1')
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$evidence = if ($OutputDirectory) { Resolve-CheckFileSystemPath $OutputDirectory }
    else { Join-Path $repo 'out/validation/analyze' }
[IO.Directory]::CreateDirectory($evidence) | Out-Null

$baselinePath = if ($Baseline) { Resolve-CheckFileSystemPath $Baseline } else { $null }
if ($baselinePath -and -not (Test-Path -LiteralPath $baselinePath -PathType Leaf)) { throw "No baseline at $baselinePath." }
$logPath = if ($WarningsLog) { Resolve-CheckFileSystemPath $WarningsLog } else { $null }
if ($logPath -and -not (Test-Path -LiteralPath $logPath -PathType Leaf)) { throw "No warnings log at $logPath." }

if (-not $logPath) {
    foreach ($command in 'cmake', 'cl') {
        if (-not (Get-Command $command -ErrorAction SilentlyContinue)) {
            throw "Run this script from an x64 Visual Studio Developer shell ($command is missing)."
        }
    }
}

function Invoke-Logged([string[]] $Arguments, [string] $LogPath) {
    # Windows PowerShell represents native stderr as ErrorRecord instances.
    # The executable's exit code decides success; preserve both streams.
    $ErrorActionPreference = 'Continue'
    $output = & cmake @Arguments 2>&1 | ForEach-Object { "$_" }
    $code = $LASTEXITCODE
    $output | Out-File -LiteralPath $LogPath -Encoding utf8
    if ($code -ne 0) {
        $output | Select-Object -Last 40 | ForEach-Object { Write-Host $_ }
        throw "cmake $($Arguments -join ' ') exited with $code. See $LogPath"
    }
    return @($output)
}

if ($logPath) {
    Write-Host "Recount $logPath without building"
    $build = @(Get-Content -LiteralPath $logPath -Encoding utf8)
} else {
    Push-Location -LiteralPath $repo
    try {
        Write-Host "Configure and build x64-analyze; logs: $evidence"
        $null = Invoke-Logged @('--preset', 'x64-analyze') (Join-Path $evidence 'configure.log')
        # Only a compile reports, so every run recompiles everything.
        $build = Invoke-Logged @('--build', '--preset', 'x64-analyze', '--clean-first') (Join-Path $evidence 'build.log')
    } finally { Pop-Location }
}

# /analyze numbers its warnings C6xxx, C26xxx and C28xxx. A header's warning
# repeats in every translation unit that includes it; count each line once.
$warnings = @($build | Where-Object { $_ -match ': warning C(6|26|28)\d{3}\b' } | Sort-Object -Unique)
$warnings | Out-File -LiteralPath (Join-Path $evidence 'analyze-warnings.log') -Encoding utf8

# The engine's findings are Labrador's to fix; they are reported apart.
$counts = [ordered]@{ client = @{}; engine = @{} }
foreach ($line in $warnings) {
    $owner = if ($line -match '[\\/]external[\\/]labrador[\\/]') { 'engine' } else { 'client' }
    $code = [regex]::Match($line, ': warning (C\d+)').Groups[1].Value
    $counts[$owner][$code] = 1 + [int]$counts[$owner][$code]
}
$summary = [ordered]@{ schema_version = 1; preset = 'x64-analyze'; total = $warnings.Count }
foreach ($owner in $counts.Keys) {
    $byCode = [ordered]@{}
    foreach ($code in @($counts[$owner].Keys | Sort-Object)) { $byCode[$code] = $counts[$owner][$code] }
    $total = 0
    foreach ($value in $byCode.Values) { $total += $value }
    $summary[$owner] = [ordered]@{ total = $total; by_code = $byCode }
    Write-Host "/analyze ${owner}: $total warning(s)$(if ($total) { ': ' + (@($byCode.Keys | ForEach-Object { "$_ x$($byCode[$_])" }) -join ', ') })"
}

# Matched by code and file, then by exact line; what is left in a group pairs
# in line order as a moved warning, and the rest are new or vanished. The
# engine's warnings are Labrador's and are not compared.
if ($baselinePath) {
    $accepted = @((Get-Content -LiteralPath $baselinePath -Raw -Encoding utf8 | ConvertFrom-Json).warnings)
    $root = $repo.TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
    $found = @(foreach ($line in $warnings) {
        $parts = [regex]::Match($line, '^(?<path>.*?)\((?<line>\d+)\)\s*:\s*warning (?<code>C\d+):\s*(?<text>.*)$')
        if (-not $parts.Success) { continue }
        $full = [IO.Path]::GetFullPath($parts.Groups['path'].Value)
        $file = if ($full.StartsWith($root, [StringComparison]::OrdinalIgnoreCase)) { $full.Substring($root.Length) } else { $full }
        $file = $file.Replace('\', '/')
        if ($file -like 'external/labrador/*') { continue }
        [pscustomobject]@{ code = $parts.Groups['code'].Value; file = $file
            line = [int]$parts.Groups['line'].Value; text = $parts.Groups['text'].Value.Trim() }
    })
    $new = [Collections.Generic.List[object]]::new()
    $vanished = [Collections.Generic.List[object]]::new()
    $keys = @(@($found) + @($accepted) | ForEach-Object { "$($_.code)|$($_.file)" } | Sort-Object -Unique)
    foreach ($key in $keys) {
        $now = [Collections.Generic.List[object]]::new()
        foreach ($entry in @($found | Where-Object { "$($_.code)|$($_.file)" -ceq $key } | Sort-Object line)) { $now.Add($entry) }
        $was = [Collections.Generic.List[object]]::new()
        foreach ($entry in @($accepted | Where-Object { "$($_.code)|$($_.file)" -ceq $key } | Sort-Object line)) { $was.Add($entry) }
        foreach ($entry in @($was)) {
            $same = @($now | Where-Object { $_.line -eq $entry.line })
            if ($same.Count) { $null = $now.Remove($same[0]); $null = $was.Remove($entry) }
        }
        $moved = [Math]::Min($now.Count, $was.Count)
        for ($index = $moved; $index -lt $now.Count; ++$index) { $new.Add($now[$index]) }
        for ($index = $moved; $index -lt $was.Count; ++$index) { $vanished.Add($was[$index]) }
    }
    $summary['baseline'] = [ordered]@{
        path = $baselinePath; accepted = $accepted.Count; found = $found.Count
        new = @($new | ForEach-Object { [ordered]@{ code = $_.code; file = $_.file; line = $_.line; text = $_.text } })
        vanished = @($vanished | ForEach-Object { [ordered]@{ code = $_.code; file = $_.file; line = $_.line; function = $_.function } })
    }
    $verdict = if ($new.Count -or $vanished.Count) {
        "$($new.Count) new and $($vanished.Count) vanished against the $($accepted.Count) accepted"
    } else { "all $($found.Count) match the $($accepted.Count) accepted" }
    Write-Host "/analyze baseline: $verdict"
    foreach ($entry in $new) { Write-Host "  new: $($entry.code) $($entry.file)($($entry.line)) $($entry.text)" }
    foreach ($entry in $vanished) { Write-Host "  vanished: $($entry.code) $($entry.file) $($entry.function)" }
    if ($env:GITHUB_ACTIONS -eq 'true') {
        foreach ($entry in $new) {
            Write-Host "::warning file=$($entry.file),line=$($entry.line),title=New /analyze warning::$($entry.code) $($entry.text)"
        }
        foreach ($entry in $vanished) {
            Write-Host "::notice title=/analyze warning gone::$($entry.code) $($entry.file) $($entry.function); remove it from the baseline"
        }
    }
    if ($env:GITHUB_STEP_SUMMARY) {
        $markdown = @('### /analyze against the baseline', '', "This repository: $verdict.", '')
        if ($new.Count -or $vanished.Count) {
            $markdown += @('| | Code | File | Line | |', '|---|---|---|---|---|')
            foreach ($entry in $new) { $markdown += "| new | $($entry.code) | ``$($entry.file)`` | $($entry.line) | $($entry.text.Replace('|', '\|')) |" }
            foreach ($entry in $vanished) { $markdown += "| vanished | $($entry.code) | ``$($entry.file)`` | $($entry.line) | $($entry.function) |" }
        }
        [IO.File]::AppendAllText($env:GITHUB_STEP_SUMMARY, (($markdown -join "`n") + "`n`n"), [Text.UTF8Encoding]::new($false))
    }
}
[IO.File]::WriteAllText((Join-Path $evidence 'analyze-summary.json'), ($summary | ConvertTo-Json -Depth 5),
    [Text.UTF8Encoding]::new($false))
Write-Host "REPORT static analysis: $($warnings.Count) distinct warning(s). Evidence: $evidence"
