# REL#3: build and check a release candidate in one command, and write its
# receipt. Refuses a dirty tree (the rule build-info.txt's -dirty suffix
# states) and an engine submodule that is not at the pin, then:
#
#   1. builds and packages x64-release (scripts/build.ps1 -Package);
#   2. inspects the ZIP (scripts/check-release-package.ps1);
#   3. starts the extracted game with the accepted bank, a system-only DLL
#      search path and an isolated save (scripts/check-startup.ps1 -RequireAudio);
#   4. starts a fresh extraction without the bank (-RequireNoAudio), leaving
#      the first intact;
#
# and writes candidate.json beside the evidence: commit, engine pin, version,
# the ZIP's SHA256, the packaged build-info.txt, and each check's outcome, log
# and result.json. Every check runs even when an earlier one failed; the script
# fails at the end if any did. -SkipBuild checks the ZIP already built, which
# must still name this commit. docs/releasing.md says what this proves and what
# it does not.
#
#   pwsh -NoProfile -File scripts/release-candidate.ps1
#   pwsh -NoProfile -File scripts/release-candidate.ps1 -OutputDirectory <dir> [-SkipBuild]
[CmdletBinding()]
param(
    [string]$OutputDirectory,
    [switch]$SkipBuild
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot 'check-path.ps1')
. (Join-Path $PSScriptRoot 'release-package-path.ps1')

$repository = Split-Path -Parent $PSScriptRoot
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $repository 'out/validation/release-candidate' }
$outputRoot = Resolve-CheckFileSystemPath $OutputDirectory
$shell = Join-Path $PSHOME $(if ($PSVersionTable.PSEdition -eq 'Desktop') { 'powershell.exe' } else { 'pwsh.exe' })

function Invoke-Git([string[]]$Arguments) {
    $output = & git -C $repository @Arguments
    if ($LASTEXITCODE -ne 0) { throw "release-candidate: git $($Arguments -join ' ') failed (exit $LASTEXITCODE)." }
    return $output
}

# The refusals come first, before any build or evidence folder.
$changes = @(Invoke-Git @('status', '--porcelain', '--untracked-files=normal'))
if ($changes.Count -gt 0) {
    throw ("release-candidate: refusing a dirty tree; commit or remove these first:`n  " + ($changes -join "`n  "))
}
$zip = Get-ColourWarsReleasePackagePath $repository
if ($SkipBuild -and -not (Test-Path -LiteralPath $zip -PathType Leaf)) {
    throw "release-candidate: no Release ZIP at $zip; run without -SkipBuild to build one."
}
# An uninitialised engine is not a change git status reports.
$submodule = @(Invoke-Git @('submodule', 'status', '--', 'external/labrador'))
if ($submodule.Count -ne 1 -or $submodule[0] -match '^[-+U]') {
    throw "release-candidate: external/labrador is not checked out at the pin: $($submodule -join ' ')"
}
$commit = [string](Invoke-Git @('rev-parse', 'HEAD'))
$pin = [string](Invoke-Git @('rev-parse', 'HEAD:external/labrador'))
$version = [regex]::Match([IO.Path]::GetFileName($zip), '^ColourWars-(.+)-windows-x64\.zip$').Groups[1].Value

$run = Join-Path $outputRoot ('{0}-{1}' -f $commit.Substring(0, 12), [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssZ'))
if (Test-Path -LiteralPath $run) { throw "release-candidate: $run already exists." }
$null = New-Item -ItemType Directory -Path $run -Force
$receipt = [ordered]@{
    format = 1
    outcome = 'failed'
    commit = $commit
    engine_pin = $pin
    version = $version
    started_utc = [DateTime]::UtcNow.ToString('o')
    finished_utc = $null
    powershell = $PSVersionTable.PSVersion.ToString()
    zip = $null
    build_info = $null
    checks = @()
    not_covered = 'Listening, physical pads and other hardware, and the Windows Sandbox run (REL#2) are separate.'
}
$checks = [Collections.Generic.List[object]]::new()

# One step in its own process, so a failure is recorded and the next still runs.
function Invoke-Step([string]$Name, [string]$Script, [string[]]$Arguments, [string]$ResultDirectory) {
    $log = Join-Path $run "$Name.log"
    $step = [ordered]@{ name = $Name; outcome = 'failed'; exit_code = $null; log = $log; result = $null }
    $previous = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        & $shell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot $Script) @Arguments 2>&1 |
            ForEach-Object { $line = [string]$_; Write-Host $line; $line } |
            Set-Content -LiteralPath $log -Encoding utf8
        $step.exit_code = $LASTEXITCODE
    }
    finally { $ErrorActionPreference = $previous }
    if ($ResultDirectory -and (Test-Path -LiteralPath $ResultDirectory)) {
        $result = @(Get-ChildItem -LiteralPath $ResultDirectory -Filter 'result.json' -File -Recurse)
        if ($result.Count -eq 1) { $step.result = $result[0].FullName }
    }
    if ($step.exit_code -eq 0) { $step.outcome = 'passed' }
    $script:checks.Add($step)
    return $step.outcome -eq 'passed'
}

function Add-Failure([string]$Name, [string]$Diagnostic) {
    Write-Host "FAIL $Name`: $Diagnostic"
    $script:checks.Add([ordered]@{ name = $Name; outcome = 'failed'; diagnostic = $Diagnostic })
}

try {
    $built = $true
    if (-not $SkipBuild) {
        $built = Invoke-Step 'build' 'build.ps1' @('-Preset', 'x64-release', '-Package') $null
        $buildLog = Join-Path $repository 'out/build-x64-release.log'
        if (Test-Path -LiteralPath $buildLog) { Copy-Item -LiteralPath $buildLog -Destination (Join-Path $run 'build-transcript.log') }
    }
    if (-not $built) { throw 'release-candidate: the build failed; see build.log.' }
    if (-not (Test-Path -LiteralPath $zip -PathType Leaf)) { throw "release-candidate: the build left no Release ZIP at $zip." }
    $receipt.zip = [ordered]@{
        path = $zip
        sha256 = (Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash.ToLowerInvariant()
        bytes = (Get-Item -LiteralPath $zip).Length
    }
    Copy-Item -LiteralPath $zip -Destination (Join-Path $run ([IO.Path]::GetFileName($zip)))

    $packageRoot = Join-Path $run 'package'
    $inspected = Invoke-Step 'package' 'check-release-package.ps1' @('-Zip', $zip, '-OutputDirectory', $packageRoot) $null
    $game = Join-Path $packageRoot 'ColourWars-windows-x64'

    # The packaged stamp, not the build tree's: it is what a player has.
    $buildInfo = Join-Path $game 'build-info.txt'
    if (Test-Path -LiteralPath $buildInfo -PathType Leaf) {
        Copy-Item -LiteralPath $buildInfo -Destination (Join-Path $run 'build-info.txt')
        $receipt.build_info = Join-Path $run 'build-info.txt'
        $details = @{}
        foreach ($line in Get-Content -LiteralPath $buildInfo) {
            if ($line -match '^([^:]+): (.*)$') { $details[$Matches[1]] = $Matches[2] }
        }
        $expected = [ordered]@{ Version = $version; ColourWars = $commit; Labrador = $pin; Configuration = 'Release' }
        $wrong = @($expected.Keys | Where-Object { -not $details.ContainsKey($_) -or $details[$_] -cne $expected[$_] })
        if ($wrong.Count -eq 0) {
            $checks.Add([ordered]@{ name = 'build-info'; outcome = 'passed' })
        }
        else {
            $found = ($wrong | ForEach-Object { "$_ is '$(if ($details.ContainsKey($_)) { $details[$_] })', expected '$($expected[$_])'" }) -join '; '
            Add-Failure 'build-info' "the packaged build-info.txt does not describe this clean commit: $found"
        }
    }
    else { Add-Failure 'build-info' 'the package has no build-info.txt' }

    if ($inspected) {
        $startup = Join-Path $run 'startup'
        $null = Invoke-Step 'startup' 'check-startup.ps1' @('-Executable', (Join-Path $game 'ColourWars.exe'),
            '-RequireAudio', '-SystemOnlyPath', '-OutputDirectory', $startup) $startup

        $fallback = Join-Path $run 'fallback/ColourWars-windows-x64'
        Expand-Archive -LiteralPath $zip -DestinationPath $fallback
        Remove-Item -LiteralPath (Join-Path $fallback 'sounds/paint_exe.xwb')
        $fallbackStartup = Join-Path $run 'fallback-startup'
        $null = Invoke-Step 'fallback-startup' 'check-startup.ps1' @('-Executable', (Join-Path $fallback 'ColourWars.exe'),
            '-RequireNoAudio', '-SystemOnlyPath', '-OutputDirectory', $fallbackStartup) $fallbackStartup
    }
    else {
        Add-Failure 'startup' 'not run: the package inspection failed'
        Add-Failure 'fallback-startup' 'not run: the package inspection failed'
    }
}
catch {
    Add-Failure 'candidate' $_.Exception.Message
}
finally {
    $receipt.checks = @($checks.ToArray())
    if (@($checks | Where-Object { $_.outcome -ne 'passed' }).Count -eq 0 -and $checks.Count -gt 0) { $receipt.outcome = 'passed' }
    $receipt.finished_utc = [DateTime]::UtcNow.ToString('o')
    $receipt | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $run 'candidate.json') -Encoding utf8
    Write-Host "Release candidate receipt: $(Join-Path $run 'candidate.json')"
}
if ($receipt.outcome -ne 'passed') { throw 'release-candidate: a check failed; candidate.json names it.' }
Write-Host "PASS release candidate $version at $commit (ZIP SHA256 $($receipt.zip.sha256))"
