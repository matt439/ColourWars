# Builds the x64-analyze preset (MSVC /analyze over a Debug build) and counts
# its warnings. A report, not a check: it fails only when the configure or the
# build itself fails, never for what the analysis says.
[CmdletBinding()]
param([string] $OutputDirectory)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot 'check-path.ps1')
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$evidence = if ($OutputDirectory) { Resolve-CheckFileSystemPath $OutputDirectory }
    else { Join-Path $repo 'out/validation/analyze' }
[IO.Directory]::CreateDirectory($evidence) | Out-Null

foreach ($command in 'cmake', 'cl') {
    if (-not (Get-Command $command -ErrorAction SilentlyContinue)) {
        throw "Run this script from an x64 Visual Studio Developer shell ($command is missing)."
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

Push-Location -LiteralPath $repo
try {
    Write-Host "Configure and build x64-analyze; logs: $evidence"
    $null = Invoke-Logged @('--preset', 'x64-analyze') (Join-Path $evidence 'configure.log')
    # Only a compile reports, so every run recompiles everything.
    $build = Invoke-Logged @('--build', '--preset', 'x64-analyze', '--clean-first') (Join-Path $evidence 'build.log')
} finally { Pop-Location }

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
[IO.File]::WriteAllText((Join-Path $evidence 'analyze-summary.json'), ($summary | ConvertTo-Json -Depth 4),
    [Text.UTF8Encoding]::new($false))
Write-Host "REPORT static analysis: $($warnings.Count) distinct warning(s). Evidence: $evidence"
