# CI#6: run vcvars64.bat once per job and hand the variables it sets to every
# later step through GITHUB_ENV, so no step has to call it again. Locally,
# -EnvironmentFile <file> writes the same NAME=value lines to a scratch file.
[CmdletBinding()]
param(
    [string]$EnvironmentFile = $env:GITHUB_ENV
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if (-not $EnvironmentFile) { throw 'export-msvc-environment: GITHUB_ENV is not set; pass -EnvironmentFile.' }

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$install = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $install) { throw 'export-msvc-environment: vswhere found no Visual Studio with the x64 C++ tools.' }
$vcvars = Join-Path $install 'VC\Auxiliary\Build\vcvars64.bat'

# A batch file sidesteps cmd's quoting of the spaced vcvars path.
$batch = Join-Path ([IO.Path]::GetTempPath()) "export-msvc-environment-$PID.cmd"
Set-Content -LiteralPath $batch -Encoding ascii -Value "@call `"$vcvars`" >nul`r`n@if errorlevel 1 exit /b 1`r`n@set"
try {
    $lines = @(& cmd.exe /d /c $batch)
    if ($LASTEXITCODE -ne 0) { throw "export-msvc-environment: vcvars64.bat failed with exit code $LASTEXITCODE." }
}
finally {
    Remove-Item -LiteralPath $batch -ErrorAction SilentlyContinue
}

# Only what vcvars64 added or changed; PowerShell hashtables ignore case, as
# Windows variable names do.
$before = @{}
foreach ($variable in Get-ChildItem env:) { $before[$variable.Name] = $variable.Value }
$exported = @()
foreach ($line in $lines) {
    $at = $line.IndexOf('=')
    if ($at -le 0) { continue }
    $name = $line.Substring(0, $at)
    $value = $line.Substring($at + 1)
    if ($before.ContainsKey($name) -and $before[$name] -ceq $value) { continue }
    $exported += "$name=$value"
}
if (-not ($exported | Where-Object { $_ -like 'INCLUDE=*' })) {
    throw 'export-msvc-environment: vcvars64.bat set no INCLUDE; the MSVC environment is incomplete.'
}
Add-Content -LiteralPath $EnvironmentFile -Encoding utf8 -Value $exported
Write-Host "export-msvc-environment: $($exported.Count) variables from $vcvars"
