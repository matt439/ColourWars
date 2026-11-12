# REL#3: check scripts/release-candidate.ps1's refusals without starting a
# build. A scratch Git repository holds a copy of the script and the helpers
# it dot-sources, with no build.ps1, so a refusal that came too late would
# fail differently. It checks that an untracked file and an edited tracked
# file are refused as a dirty tree, that -SkipBuild with no ZIP is refused,
# and that no refusal leaves an evidence folder. No CMake, compiler or game.
[CmdletBinding()]
param(
    [string]$OutputDirectory
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot 'check-path.ps1')

if (-not $OutputDirectory) { $OutputDirectory = Join-Path $PSScriptRoot '../out/validation/release-candidate-check' }
$root = Join-Path (Resolve-CheckFileSystemPath $OutputDirectory) ([Guid]::NewGuid().ToString('N'))
$repository = Join-Path $root 'repository'
$evidence = Join-Path $root 'evidence'
$scripts = Join-Path $repository 'scripts'
$null = New-Item -ItemType Directory -Path $scripts -Force
$shell = Join-Path $PSHOME $(if ($PSVersionTable.PSEdition -eq 'Desktop') { 'powershell.exe' } else { 'pwsh.exe' })
$checks = 0

function Assert-True([bool]$Condition, [string]$Message) {
    if (-not $Condition) { throw "check-release-candidate: $Message" }
    $script:checks++
}

function Invoke-Git([string[]]$Arguments) {
    $previous = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        $output = & git -C $repository @Arguments 2>&1
        if ($LASTEXITCODE -ne 0) { throw "git $($Arguments -join ' ') failed: $output" }
    }
    finally { $ErrorActionPreference = $previous }
}

# The candidate script in its own process. The wrapper prints the refusal's
# message as one plain line; the shells' error views wrap and colour it.
function Invoke-Candidate([string[]]$Arguments) {
    $quote = { param([string]$Text) "'" + $Text.Replace("'", "''") + "'" }
    $command = "try { & $(& $quote (Join-Path $scripts 'release-candidate.ps1')) -OutputDirectory $(& $quote $evidence) " +
        ($Arguments -join ' ') + " } catch { [Console]::Out.WriteLine('REFUSED: ' + `$_.Exception.Message); exit 1 }"
    $previous = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        $output = @(& $shell -NoProfile -ExecutionPolicy Bypass -Command $command 2>&1 | ForEach-Object { [string]$_ })
        return [pscustomobject]@{ ExitCode = $LASTEXITCODE; Output = ($output -join "`n") }
    }
    finally { $ErrorActionPreference = $previous }
}

try {
    foreach ($name in 'release-candidate.ps1', 'check-path.ps1', 'release-package-path.ps1') {
        Copy-Item -LiteralPath (Join-Path $PSScriptRoot $name) -Destination $scripts
    }
    [IO.File]::WriteAllText((Join-Path $repository 'CMakeLists.txt'),
        "cmake_minimum_required(VERSION 3.26)`nproject(ColourWars VERSION 0.1.0 LANGUAGES CXX RC)`n")
    [IO.File]::WriteAllText((Join-Path $repository '.gitignore'), "out/`n")
    Invoke-Git @('init', '-q', '-b', 'master')
    Invoke-Git @('config', 'user.name', 'check-release-candidate')
    Invoke-Git @('config', 'user.email', 'check-release-candidate@example.invalid')
    Invoke-Git @('config', 'core.autocrlf', 'false')
    Invoke-Git @('config', 'commit.gpgsign', 'false')
    Invoke-Git @('add', '--all')
    Invoke-Git @('commit', '-q', '-m', 'Fixture')

    [IO.File]::WriteAllText((Join-Path $repository 'stray.txt'), 'untracked')
    $result = Invoke-Candidate @()
    Assert-True ($result.ExitCode -ne 0) 'an untracked file was not refused.'
    Assert-True ($result.Output -match 'REFUSED: release-candidate: refusing a dirty tree' -and $result.Output -match 'stray\.txt') `
        "an untracked file was refused for another reason: $($result.Output)"
    Remove-Item -LiteralPath (Join-Path $repository 'stray.txt')

    [IO.File]::AppendAllText((Join-Path $repository 'CMakeLists.txt'), "# edited`n")
    $result = Invoke-Candidate @('-SkipBuild')
    Assert-True ($result.ExitCode -ne 0) 'an edited tracked file was not refused.'
    Assert-True ($result.Output -match 'REFUSED: release-candidate: refusing a dirty tree' -and $result.Output -match 'CMakeLists\.txt') `
        "an edited tracked file was refused for another reason: $($result.Output)"
    Invoke-Git @('checkout', '--', 'CMakeLists.txt')

    $result = Invoke-Candidate @('-SkipBuild')
    Assert-True ($result.ExitCode -ne 0) '-SkipBuild with no ZIP was not refused.'
    Assert-True ($result.Output -match 'REFUSED: release-candidate: no Release ZIP at .*ColourWars-0\.1\.0-windows-x64\.zip') `
        "-SkipBuild with no ZIP was refused for another reason: $($result.Output)"

    Assert-True (-not (Test-Path -LiteralPath $evidence)) 'a refusal left an evidence folder.'
    Write-Host "check-release-candidate: $checks checks passed (dirty tree, edited file, missing ZIP; no build started)"
}
finally {
    if (Test-Path -LiteralPath $root) { Remove-Item -LiteralPath $root -Recurse -Force }
}
