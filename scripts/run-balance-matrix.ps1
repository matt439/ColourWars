[CmdletBinding()]
param(
    [string] $Design,
    [Parameter(Mandatory=$true)][string] $OutputDirectory,
    [ValidateRange(1,16)][int] $Parallel = 4,
    [ValidateRange(1,86400)][int] $RunTimeoutSeconds = 3600,
    [string] $RunnerExecutable,
    [switch] $SkipBuild,
    [switch] $PlanOnly
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = Split-Path -Parent $PSScriptRoot
if (-not $Design) { $Design = Join-Path $PSScriptRoot 'balance/shipped.psd1' }
$designPath = (Resolve-Path -LiteralPath $Design).ProviderPath
if ([IO.Path]::GetExtension($designPath) -ne '.psd1') { throw 'Design must be a PowerShell data file (.psd1).' }
$definition = Import-PowerShellDataFile -LiteralPath $designPath
$json = $definition | ConvertTo-Json -Depth 12
$output = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputDirectory)
$driver = Join-Path $PSScriptRoot 'balance/matrix.py'
$args = @($driver, '--design-stdin', '--design-source', $designPath, '--out', $output,
    '--parallel', $Parallel.ToString(), '--timeout', $RunTimeoutSeconds.ToString())
# Validate the complete design and output scope before configuring or compiling.
$json | & python @args --plan-only
if ($LASTEXITCODE -ne 0) { throw 'Invalid balance matrix plan.' }
if ($PlanOnly) { return }
if (-not $RunnerExecutable) {
    $compile = Join-Path $repo ('out/balance-compile/' + [guid]::NewGuid().ToString('N'))
    & (Join-Path $PSScriptRoot 'run-acceptance.ps1') -Preset x64-debug -Cases balance_runner -CompileOnly -SkipBuild:$SkipBuild -OutputDirectory $compile
    $RunnerExecutable = Join-Path $compile 'x64-debug/balance_runner.exe'
}
$exe = (Resolve-Path -LiteralPath $RunnerExecutable).ProviderPath
$json | & python @args --exe $exe
if ($LASTEXITCODE -ne 0) { throw 'Balance matrix failed; retained job logs and receipts identify the failure.' }
