[CmdletBinding()]
param(
    [ValidateSet('x64-debug')][string] $Preset = 'x64-debug',
    [string[]] $Stage = @('crossfeed'),
    [uint32[]] $Seed = @(1),
    [ValidateRange(1,4)][int] $Players = 2,
    [ValidateRange(0.05,600.0)][double] $Duration = 60,
    [ValidateSet('sprayer','sniper','roller','mister','bucket')][string[]] $Tools,
    [ValidateSet('idle','walk','jump','chase','paint','skirmish')][string] $Bot = 'paint',
    [string] $Csv,
    [string] $OutputDirectory,
    [switch] $SkipBuild
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if ($null -eq $Stage -or @($Stage).Count -eq 0 -or $null -eq $Seed -or @($Seed).Count -eq 0 -or
    [double]::IsNaN($Duration) -or [double]::IsInfinity($Duration)) {
    throw 'Provide at least one stage and seed, and a finite duration.'
}
foreach ($id in $Stage) {
    if ($id -notmatch '^[A-Za-z0-9_-]+$') { throw "Invalid stage identifier: $id" }
}
if (-not $Tools) { $Tools = @('sprayer') * $Players }
if ($Tools.Count -ne $Players) { throw 'Tools must contain exactly one weapon for each player.' }
$run = if ($OutputDirectory) {
    [IO.Path]::GetFullPath($(if ([IO.Path]::IsPathRooted($OutputDirectory)) { $OutputDirectory } else { Join-Path $repo $OutputDirectory }))
} else {
    Join-Path $repo ('out/balance/' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss') + '-' + [guid]::NewGuid().ToString('N'))
}
if (Test-Path -LiteralPath $run) { throw 'Balance output directory must be new; preserve earlier evidence.' }
$csvPath = if ($Csv) {
    [IO.Path]::GetFullPath($(if ([IO.Path]::IsPathRooted($Csv)) { $Csv } else { Join-Path $repo $Csv }))
} else { Join-Path $run 'matches.csv' }
if (Test-Path -LiteralPath $csvPath) { throw 'CSV already exists; choose a new path.' }
[IO.Directory]::CreateDirectory($run) | Out-Null
[IO.Directory]::CreateDirectory((Split-Path -Parent $csvPath)) | Out-Null
$build = Join-Path $repo "out/build/$Preset"
$content = Join-Path $build 'game'
$compile = Join-Path $run 'compile'
$exe = Join-Path $compile "$Preset/balance_runner.exe"
$receipt = Join-Path $run 'run-info.json'
$result = [ordered]@{
    outcome='running'; started_utc=[DateTime]::UtcNow.ToString('o'); preset=$Preset
    stages=$Stage; seeds=$Seed; players=$Players; requested_duration=$Duration
    tools=$Tools; bot=$Bot; csv=$csvPath; executable=$exe
    device_free=$true; human_acceptance=$false; failure=$null
}
function Save-Result { $result | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $receipt -Encoding utf8 }
function Get-Pin([string]$Path) { [ordered]@{path=$Path;sha256=(Get-FileHash -LiteralPath $Path).Hash} }
Save-Result
$previousPath = $env:PATH
$previousNativeEncoding = [Console]::OutputEncoding
try {
    & (Join-Path $PSScriptRoot 'run-acceptance.ps1') -Preset $Preset -SkipBuild:$SkipBuild -Cases @('balance_runner') -CompileOnly -OutputDirectory $compile *> (Join-Path $run 'compile.log')
    if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) { throw 'Balance executable was not produced.' }
    $arguments = @($content)
    foreach ($id in $Stage) { $arguments += @('--stage',$id) }
    foreach ($value in $Seed) { $arguments += @('--seed',$value.ToString([Globalization.CultureInfo]::InvariantCulture)) }
    $arguments += @('--players',$Players.ToString([Globalization.CultureInfo]::InvariantCulture),
        '--duration',$Duration.ToString('R',[Globalization.CultureInfo]::InvariantCulture),
        '--tools',(($Tools | ForEach-Object { $_.ToLowerInvariant() }) -join ','),
        '--bot',$Bot.ToLowerInvariant(),'--csv',$csvPath)
    $result.arguments=$arguments
    $result.input_pins=@((Get-Pin (Join-Path $content 'tuning.json')),(Get-Pin (Join-Path $content 'presentation.json')),
        (Get-Pin (Join-Path $content 'manifest.json')),(Get-Pin (Join-Path $build 'generated/build-info.txt')),
        (Get-Pin (Join-Path $content 'ColourWars.exe')),(Get-Pin $exe))
    foreach ($id in @($Stage | Select-Object -Unique)) { $result.input_pins += Get-Pin (Join-Path $content "levels/$id.json") }
    $result.source_pins=@((Get-Pin $PSCommandPath),(Get-Pin (Join-Path $PSScriptRoot 'run-acceptance.ps1')),
        (Get-Pin (Join-Path $PSScriptRoot 'acceptance/balance_runner.cpp')))
    $parts=Join-Path $PSScriptRoot 'acceptance/balance_runner'
    if (Test-Path -LiteralPath $parts) { $result.source_pins += @(Get-ChildItem -LiteralPath $parts -File -Recurse | Sort-Object FullName | ForEach-Object { Get-Pin $_.FullName }) }
    if (Get-Command git -ErrorAction SilentlyContinue) {
        $result.revision=(& git -C $repo rev-parse HEAD).Trim()
        $result.working_changes=@(& git -C $repo status --porcelain)
    }
    Save-Result
    $env:PATH="$content;$previousPath"
    $result.native_stdout_encoding='utf-8'
    [Console]::OutputEncoding = [Text.UTF8Encoding]::new($false)
    $saved=$ErrorActionPreference; $ErrorActionPreference='Continue'
    try { $output=& $exe @arguments 2>&1; $exitCode=$LASTEXITCODE; $output | Out-File -LiteralPath (Join-Path $run 'run.log') -Encoding utf8 }
    finally { $ErrorActionPreference=$saved; [Console]::OutputEncoding=$previousNativeEncoding }
    $result.exit_code=$exitCode
    if ($exitCode -ne 0) { throw "Balance runner exited $exitCode; inspect run.log." }
    if (-not (Test-Path -LiteralPath $csvPath -PathType Leaf)) { throw 'Balance runner did not produce its CSV.' }
    foreach ($pin in @($result.input_pins)+@($result.source_pins)) {
        if ((Get-FileHash -LiteralPath $pin.path).Hash -ne $pin.sha256) { throw "Input changed during balance run: $($pin.path)" }
    }
    $result.csv_sha256=(Get-FileHash -LiteralPath $csvPath).Hash
    $result.outcome='passed'
} catch { $result.outcome='failed';$result.failure=$_.Exception.Message;throw }
finally { $env:PATH=$previousPath;[Console]::OutputEncoding=$previousNativeEncoding;$result.console_output_encoding_restored=([Console]::OutputEncoding.CodePage -eq $previousNativeEncoding.CodePage);$result.completed_utc=[DateTime]::UtcNow.ToString('o');Save-Result }
Write-Output "PASS device-free balance run. CSV: $csvPath; evidence: $receipt"
