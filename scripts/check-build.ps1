# First-run build failures, with private source trees and no compiler/download.
# Run under Windows PowerShell 5.1 and pwsh; each fixture runs in a child shell.
[CmdletBinding()]
param([string]$OutputDirectory)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot 'check-path.ps1')
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $PSScriptRoot '../out/validation/build-helper' }
$fixtureRoot = Join-Path (Resolve-CheckFileSystemPath $OutputDirectory) ([Guid]::NewGuid().ToString('N'))
$null = New-Item -ItemType Directory -Path $fixtureRoot -Force
$shell = (Get-Process -Id $PID).Path
$checks = 0

function Assert-True([bool]$Condition, [string]$Message) {
    if (-not $Condition) { throw "check-build: $Message" }
    $script:checks++
}

foreach ($case in 'package-debug', 'download-zip', 'missing-vs') {
    $tree = Join-Path $fixtureRoot $case
    $scripts = Join-Path $tree 'scripts'
    $null = New-Item -ItemType Directory -Path $scripts -Force
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'build.ps1') -Destination $scripts
    if ($case -eq 'missing-vs') {
        $engine = Join-Path $tree 'external/labrador'
        $null = New-Item -ItemType Directory -Path $engine -Force
        [IO.File]::WriteAllText((Join-Path $engine 'CMakeLists.txt'), '# fixture: no build')
    }
    $wrapper = Join-Path $tree 'invoke.ps1'
    [IO.File]::WriteAllText($wrapper, @'
param([string]$Case)
if ($Case -eq 'missing-vs') {
    [Environment]::SetEnvironmentVariable('ProgramFiles(x86)', (Join-Path $PSScriptRoot 'no-visual-studio'))
}
$arguments = @{ Preset = 'x64-debug' }
if ($Case -eq 'package-debug') { $arguments.Package = $true }
& (Join-Path $PSScriptRoot 'scripts/build.ps1') @arguments
exit $LASTEXITCODE
'@)
    $previous = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        $output = & $shell -NoProfile -ExecutionPolicy Bypass -File $wrapper $case 2>&1 | Out-String
        $code = $LASTEXITCODE
    }
    finally { $ErrorActionPreference = $previous }
    [IO.File]::WriteAllText((Join-Path $tree 'child-output.txt'), $output)
    Assert-True ($code -eq 1) "$case did not exit 1"
    $log = Join-Path $tree 'out/build-x64-debug.log'
    Assert-True (Test-Path -LiteralPath $log -PathType Leaf) "$case did not retain its transcript"
    $text = Get-Content -LiteralPath $log -Raw
    Assert-True ($text.Contains('https://github.com/matt439/ColourWars#if-something-goes-wrong')) "$case omitted the troubleshooting link"
    Assert-True ($text.Contains("Build log: $log")) "$case omitted the build log path"
    $expected = switch ($case) {
        'package-debug' { 'Only the x64-release preset can be packaged.' }
        'download-zip' { 'open PowerShell and enter:' }
        'missing-vs' { 'No Visual Studio 2022 or newer with C++ tools was found.' }
    }
    Assert-True ($text.Contains($expected)) "$case omitted its remedy"
    Assert-True (-not $text.Contains('Configuring x64-debug')) "$case reached CMake"
}
# Run the actual locator block in a fresh child shell. Stopping its native
# output with Select-Object -First 1 can leave LASTEXITCODE unset in pwsh,
# discarding a valid installation path. Both success and failure must finish.
$locator = Join-Path $fixtureRoot 'native-locator'
$null = New-Item -ItemType Directory -Path $locator -Force
$stub = Join-Path $locator 'vswhere.cmd'
[IO.File]::WriteAllText($stub, @'
@echo off
echo C:\fixture\visual-studio
exit /b %COLOURWARS_LOCATOR_EXIT%
'@)
$wrapper = Join-Path $locator 'invoke.ps1'
[IO.File]::WriteAllText($wrapper, @'
param([string]$BuildScript, [string]$Stub, [string]$Receipt, [int]$RequestedExit)
$ErrorActionPreference = 'Stop'
$tokens = $null; $errors = $null
$ast = [Management.Automation.Language.Parser]::ParseFile($BuildScript, [ref]$tokens, [ref]$errors)
$block = $ast.Find({ param($node)
    $node -is [Management.Automation.Language.IfStatementAst] -and
        $node.Clauses[0].Item1.Extent.Text -eq 'Test-Path -LiteralPath $vswhere'
}, $true)
if ($null -eq $block -or $errors.Count -ne 0) { throw 'Cannot read the Visual Studio locator block.' }
$env:COLOURWARS_LOCATOR_EXIT = [string]$RequestedExit
$vswhere = $Stub
$vs = $null
. ([scriptblock]::Create($block.Extent.Text))
[pscustomobject]@{ installation = $vs; native_exit = $LASTEXITCODE } |
    ConvertTo-Json | Set-Content -LiteralPath $Receipt -Encoding UTF8
'@)
foreach ($nativeExit in 0, 7) {
    $receipt = Join-Path $locator "result-$nativeExit.json"
    $output = & $shell -NoProfile -ExecutionPolicy Bypass -File $wrapper `
        -BuildScript (Join-Path $PSScriptRoot 'build.ps1') -Stub $stub `
        -Receipt $receipt -RequestedExit $nativeExit 2>&1 | Out-String
    $code = $LASTEXITCODE
    [IO.File]::WriteAllText((Join-Path $locator "child-output-$nativeExit.txt"), $output)
    Assert-True ($code -eq 0) "locator exit $nativeExit failed its fixture"
    $result = Get-Content -LiteralPath $receipt -Raw | ConvertFrom-Json
    Assert-True ($result.native_exit -eq $nativeExit) "locator exit $nativeExit lost its native exit code"
    if ($nativeExit -eq 0) {
        Assert-True ($result.installation -eq 'C:\fixture\visual-studio') 'locator discarded a valid Visual Studio installation'
    }
    else {
        Assert-True ($null -eq $result.installation) 'locator accepted a failed discovery command'
    }
}
# Exercise the actual command function with a native stub while its caller
# redirects output, which used to leave native output out of the transcript.
# The stub waits for an acknowledgement written when stdout reaches this
# outer pipeline, proving that output arrives before the command finishes.
$native = Join-Path $fixtureRoot 'native-output'
$null = New-Item -ItemType Directory -Path $native -Force
$stub = Join-Path $native 'native.cmd'
[IO.File]::WriteAllText($stub, @'
@echo off
echo native stdout marker
echo native stderr marker 1>&2
for /l %%n in (1,1,5) do (
    if exist "%~dp0ack-%1" goto acknowledged
    ping -n 2 -w 1000 127.0.0.1 >nul
)
echo native output was not streamed before exit 1>&2
exit /b 19
:acknowledged
echo native completed marker
exit /b %1
'@)
$wrapper = Join-Path $native 'invoke.ps1'
[IO.File]::WriteAllText($wrapper, @'
param([string]$BuildScript, [string]$Stub, [string]$Log, [int]$RequestedExit)
$ErrorActionPreference = 'Stop'
$tokens = $null; $errors = $null
$ast = [Management.Automation.Language.Parser]::ParseFile($BuildScript, [ref]$tokens, [ref]$errors)
$command = $ast.Find({ param($node)
    $node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq 'Invoke-BuildCommand'
}, $true)
if ($null -eq $command -or $errors.Count -ne 0) { throw 'Cannot read the build command function.' }
. ([scriptblock]::Create($command.Extent.Text))
$null = Start-Transcript -LiteralPath $Log -Force
$status = 0
try { Invoke-BuildCommand $Stub @([string]$RequestedExit) 'Native fixture failed' }
catch { Write-Host $_.Exception.Message; $status = 1 }
finally { $null = Stop-Transcript }
exit $status
'@)
foreach ($nativeExit in 0, 7) {
    $log = Join-Path $native "native-$nativeExit.log"
    $lines = [Collections.Generic.List[string]]::new()
    $previous = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        & $shell -NoProfile -ExecutionPolicy Bypass -File $wrapper `
            -BuildScript (Join-Path $PSScriptRoot 'build.ps1') -Stub $stub -Log $log -RequestedExit $nativeExit 2>&1 |
            ForEach-Object {
                $lines.Add([string]$_)
                if (([string]$_).Contains('native stdout marker')) {
                    [IO.File]::WriteAllText((Join-Path $native "ack-$nativeExit"), 'received before native exit')
                }
            }
        $code = $LASTEXITCODE
    }
    finally { $ErrorActionPreference = $previous }
    $output = $lines -join [Environment]::NewLine
    [IO.File]::WriteAllText((Join-Path $native "child-output-$nativeExit.txt"), $output)
    $text = Get-Content -LiteralPath $log -Raw
    foreach ($marker in 'native stdout marker', 'native stderr marker', 'native completed marker') {
        Assert-True ($text.Contains($marker) -and $output.Contains($marker)) "native exit $nativeExit lost $marker from the log or output"
    }
    Assert-True (-not $text.Contains('not streamed before exit')) "native exit $nativeExit buffered its output until completion"
    $expectedExit = if ($nativeExit -eq 0) { 0 } else { 1 }
    Assert-True ($code -eq $expectedExit) "native exit $nativeExit changed its success/failure outcome"
    if ($nativeExit -ne 0) {
        Assert-True ($text.Contains('Native fixture failed (exit 7).')) 'native failure lost its original exit code or contextual remedy'
    }
}
Write-Host "check-build: $checks checks passed ($($PSVersionTable.PSVersion)); evidence: $fixtureRoot"
