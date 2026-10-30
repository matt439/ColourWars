[CmdletBinding()]
param(
    [ValidateSet('x64-debug')][string] $Preset = 'x64-debug',
    [string[]] $Stage = @('crossfeed'),
    [uint32[]] $Seed = @(1),
    [ValidateRange(1,4)][int] $Players = 2,
    [ValidateRange(0.05,600.0)][double] $Duration = 60,
    [ValidateSet('sprayer','sniper','roller','mister','bucket')][string[]] $Tools,
    [ValidateSet('idle','walk','jump','chase','paint','skirmish','match')][string] $Bot = 'paint',
    # match only: the shipping bot's difficulty per seat (default Normal for every seat).
    [ValidateSet('easy','normal','hard')][string[]] $Difficulty,
    [string] $Csv,
    [string] $PaintCsv,
    [string] $OutputDirectory,
    [switch] $SkipBuild
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
function Get-BalanceAttributes([string] $Path) {
    try { return [IO.File]::GetAttributes($Path) } catch {
        $cause=$_.Exception
        while ($null -ne $cause.InnerException) { $cause=$cause.InnerException }
        if ($cause -is [IO.FileNotFoundException] -or $cause -is [IO.DirectoryNotFoundException]) { return $null }
        throw
    }
}
function Assert-NewBalanceComponent([string] $Path) {
    $leaf=[IO.Path]::GetFileName($Path)
    if ([string]::IsNullOrEmpty($leaf) -or $leaf -eq '.' -or $leaf -eq '..' -or
        $leaf.EndsWith('.') -or $leaf.EndsWith(' ') -or $leaf -match '[<>:"/\\|?*~\x00-\x1F]') {
        throw "Balance output needs an ordinary unambiguous new Windows component: $Path"
    }
    $device=$leaf.Split('.')[0].TrimEnd([char[]]@(' ','.'))
    $superscripts='^(COM|LPT)['+[char]0xB9+[char]0xB2+[char]0xB3+']$'
    if ($device -match '^(CON|PRN|AUX|NUL|CONIN\$|CONOUT\$|COM[1-9]|LPT[1-9])$' -or $device -match $superscripts) {
        throw 'Balance outputs cannot use Windows device names.'
    }
}
function Resolve-BalancePath([string] $Value) {
    if ([string]::IsNullOrEmpty($Value) -or $Value.IndexOf([char]0) -ge 0) { throw 'Balance paths must be nonempty without embedded NULs.' }
    $separator=[string][char]92
    $preferred=$Value.Replace('/',$separator)
    if ($preferred.StartsWith($separator+$separator+'.'+$separator,[StringComparison]::Ordinal) -or
        $preferred.StartsWith($separator+'??'+$separator,[StringComparison]::Ordinal)) { throw 'Balance paths cannot name Windows device namespaces.' }
    $extended=$separator+$separator+'?'+$separator
    if ($preferred.StartsWith($extended,[StringComparison]::Ordinal)) {
        $tail=$preferred.Substring(4)
        if ($tail -notmatch '^[A-Za-z]:\\' -and -not $tail.StartsWith('UNC'+$separator,[StringComparison]::OrdinalIgnoreCase)) {
            throw 'Extended balance paths must name a drive or UNC filesystem.'
        }
    }
    Assert-NewBalanceComponent $preferred
    $raw=if ([IO.Path]::IsPathRooted($preferred)) { $preferred } else { [IO.Path]::Combine($repo,$preferred) }
    # Inspect raw absent parent components before GetFullPath can trim aliases.
    $parent=[IO.Path]::GetDirectoryName($raw)
    while (-not [string]::IsNullOrEmpty($parent)) {
        $attributes=Get-BalanceAttributes $parent
        if ($null -ne $attributes) {
            if (($attributes -band [IO.FileAttributes]::Directory) -eq 0) { throw "Balance parent is not a directory: $parent" }
            break
        }
        Assert-NewBalanceComponent $parent
        $next=[IO.Path]::GetDirectoryName($parent)
        if ($next -eq $parent) { break }
        $parent=$next
    }
    return [IO.Path]::GetFullPath($raw)
}
function Get-BalanceFuturePath([string] $Path) {
    # This read-only helper exists only for the two CSV/evidence preflight.
    if (-not ('ColourWarsBalancePathV1' -as [type])) {
        $native=@"
using System;
using System.ComponentModel;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using Microsoft.Win32.SafeHandles;
public static class ColourWarsBalancePathV1 {
    [DllImport("kernel32.dll", CharSet=CharSet.Unicode, SetLastError=true, ExactSpelling=true)]
    private static extern SafeFileHandle CreateFileW(string name, uint access, uint share, IntPtr security, uint creation, uint flags, IntPtr template);
    [DllImport("kernel32.dll", CharSet=CharSet.Unicode, SetLastError=true, ExactSpelling=true)]
    private static extern uint GetFinalPathNameByHandleW(SafeFileHandle file, StringBuilder text, uint count, uint flags);
    public static string ExistingDirectory(string path) {
        using (SafeFileHandle file=CreateFileW(path,0x80,7,IntPtr.Zero,3,0x02000000,IntPtr.Zero)) {
            if (file.IsInvalid) throw new Win32Exception(Marshal.GetLastWin32Error(),"Cannot open existing balance parent: "+path);
            uint count=GetFinalPathNameByHandleW(file,null,0,0);
            if (count==0 || count>Int32.MaxValue-1) throw new Win32Exception(Marshal.GetLastWin32Error(),"Cannot size resolved balance parent: "+path);
            StringBuilder text=new StringBuilder((int)count+1);
            uint written=GetFinalPathNameByHandleW(file,text,(uint)text.Capacity,0);
            if (written==0 || written>=text.Capacity) throw new Win32Exception(Marshal.GetLastWin32Error(),"Cannot resolve balance parent: "+path);
            return text.ToString();
        }
    }
}
"@
        Add-Type -TypeDefinition $native
    }
    $suffix=[Collections.Generic.List[string]]::new()
    $current=$Path
    while (-not [string]::IsNullOrEmpty($current)) {
        $attributes=Get-BalanceAttributes $current
        if ($null -ne $attributes) {
            if (($attributes -band [IO.FileAttributes]::Directory) -eq 0) { throw "Balance existing prefix is not a directory: $current" }
            $resolved=[ColourWarsBalancePathV1]::ExistingDirectory($current)
            foreach ($component in $suffix) { $resolved=[IO.Path]::Combine($resolved,$component) }
            return $resolved
        }
        Assert-NewBalanceComponent $current
        $suffix.Insert(0,[IO.Path]::GetFileName($current))
        $next=[IO.Path]::GetDirectoryName($current)
        if ($next -eq $current) { break }
        $current=$next
    }
    throw 'Balance path has no accessible existing directory prefix.'
}
function Test-BalanceAncestor([string] $Ancestor,[string] $Descendant) {
    $separator=[string][char]92
    return [string]::Equals($Ancestor,$Descendant,[StringComparison]::OrdinalIgnoreCase) -or
        $Descendant.StartsWith($Ancestor.TrimEnd([char]92)+$separator,[StringComparison]::OrdinalIgnoreCase)
}
if ($null -eq $Stage -or @($Stage).Count -eq 0 -or $null -eq $Seed -or @($Seed).Count -eq 0 -or
    [double]::IsNaN($Duration) -or [double]::IsInfinity($Duration)) {
    throw 'Provide at least one stage and seed, and a finite duration.'
}
foreach ($id in $Stage) {
    if ($id -notmatch '^[A-Za-z0-9_-]+$') { throw "Invalid stage identifier: $id" }
}
if (-not $Tools) { $Tools = @('sprayer') * $Players }
if ($Tools.Count -ne $Players) { throw 'Tools must contain exactly one weapon for each player.' }
if ($Difficulty -and $Bot -ne 'match') { throw 'Difficulty applies only to the match bot.' }
if ($Bot -eq 'match' -and -not $Difficulty) { $Difficulty = @('normal') * $Players }
if ($Bot -eq 'match' -and $Difficulty.Count -ne $Players) { throw 'Difficulty must name exactly one level for each player.' }
$run = if ($PSBoundParameters.ContainsKey('OutputDirectory')) {
    Resolve-BalancePath $OutputDirectory
} else {
    Resolve-BalancePath (Join-Path $repo ('out/balance/' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss') + '-' + [guid]::NewGuid().ToString('N')))
}
if (Test-Path -LiteralPath $run) { throw 'Balance output directory must be new; preserve earlier evidence.' }
$csvPath = if ($PSBoundParameters.ContainsKey('Csv')) { Resolve-BalancePath $Csv } else { Resolve-BalancePath (Join-Path $run 'matches.csv') }
$paintPath = if ($PSBoundParameters.ContainsKey('PaintCsv')) { Resolve-BalancePath $PaintCsv } else {
    $name = [IO.Path]::GetFileName($csvPath)
    $extension = $name.LastIndexOf('.')
    $stem = if ($extension -gt 0) { $name.Substring(0,$extension) } else { $name }
    Resolve-BalancePath (Join-Path ([IO.Path]::GetDirectoryName($csvPath)) ($stem + '.paint.csv'))
}
if ([string]::Equals($csvPath,$paintPath,[StringComparison]::OrdinalIgnoreCase)) { throw 'Main and paint CSV destinations must be distinct.' }
foreach ($target in @($csvPath,$paintPath)) {
    if (Test-Path -LiteralPath $target) { throw "CSV already exists; choose a new path: $target" }
}
# All relationships are checked before any evidence/CSV directory is created.
$mainFuture=Get-BalanceFuturePath $csvPath
$paintFuture=Get-BalanceFuturePath $paintPath
$runFuture=Get-BalanceFuturePath $run
if ((Test-BalanceAncestor $mainFuture $paintFuture) -or (Test-BalanceAncestor $paintFuture $mainFuture)) {
    throw 'CSV destinations must not alias or be ancestors of one another.'
}
$compileFuture=[IO.Path]::Combine($runFuture,'compile')
foreach ($target in @($mainFuture,$paintFuture)) {
    if (Test-BalanceAncestor $target $runFuture) { throw 'A CSV final cannot contain or equal the evidence directory.' }
    foreach ($reserved in @($compileFuture,[IO.Path]::Combine($runFuture,'run-info.json'),
        [IO.Path]::Combine($runFuture,'run.log'),[IO.Path]::Combine($runFuture,'compile.log'))) {
        if ((Test-BalanceAncestor $target $reserved) -or (Test-BalanceAncestor $reserved $target)) {
            throw 'CSV destinations overlap the reserved compile/receipt/log paths.'
        }
    }
}
[IO.Directory]::CreateDirectory($run) | Out-Null
# CsvOutputs exclusively owns CSV parent creation after its native preflight.
$build = Join-Path $repo "out/build/$Preset"
$content = Join-Path $build 'game'
$compile = Join-Path $run 'compile'
$exe = Join-Path $compile "$Preset/balance_runner.exe"
$receipt = Join-Path $run 'run-info.json'
$result = [ordered]@{
    outcome='running'; started_utc=[DateTime]::UtcNow.ToString('o'); preset=$Preset
    stages=$Stage; seeds=$Seed; players=$Players; requested_duration=$Duration
    tools=$Tools; bot=$Bot; difficulty=$Difficulty; csv=$csvPath; paint_csv=$paintPath; executable=$exe
    main_schema=2; main_columns=81; paint_schema=1; paint_columns=29
    publication_convention='companion-first-main-last-completion-marker'; joint_physical_atomicity=$false
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
        '--bot',$Bot.ToLowerInvariant(),'--csv',$csvPath,'--paint-csv',$paintPath)
    if ($Bot -eq 'match') { $arguments += @('--difficulty',(($Difficulty | ForEach-Object { $_.ToLowerInvariant() }) -join ',')) }
    $result.arguments=$arguments
    $result.input_pins=@((Get-Pin (Join-Path $content 'tuning.json')),(Get-Pin (Join-Path $content 'presentation.json')),
        (Get-Pin (Join-Path $content 'manifest.json')),(Get-Pin (Join-Path $build 'generated/build-info.txt')),
        (Get-Pin (Join-Path $content 'ColourWars.exe')),(Get-Pin $exe))
    foreach ($id in @($Stage | Select-Object -Unique)) { $result.input_pins += Get-Pin (Join-Path $content "levels/$id.json") }
    $result.source_pins=@((Get-Pin $PSCommandPath),(Get-Pin (Join-Path $PSScriptRoot 'run-acceptance.ps1')),
        (Get-Pin (Join-Path $PSScriptRoot 'acceptance/balance_runner.cpp')))
    $parts=Join-Path $PSScriptRoot 'acceptance/balance_runner'
    if (Test-Path -LiteralPath $parts) { $result.source_pins += @(Get-ChildItem -LiteralPath $parts -File -Recurse | Sort-Object FullName | ForEach-Object { Get-Pin $_.FullName }) }
    foreach ($shared in @('metrics.h','metrics.cpp','csv.cpp')) {
        $result.source_pins += Get-Pin (Join-Path $PSScriptRoot ('acceptance/balance_regions/' + $shared))
    }
    foreach ($producer in @('game/objects/paintable_object.h','game/objects/structure_paintable.h',
        'game/objects/structure_paintable.cpp','game/objects/level.h','game/objects/level.cpp')) {
        $result.source_pins += Get-Pin (Join-Path $repo $producer)
    }
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
    foreach ($target in @($csvPath,$paintPath)) {
        if (-not (Test-Path -LiteralPath $target -PathType Leaf)) { throw "Balance runner did not produce its paired CSV: $target" }
    }
    foreach ($pin in @($result.input_pins)+@($result.source_pins)) {
        if ((Get-FileHash -LiteralPath $pin.path).Hash -ne $pin.sha256) { throw "Input changed during balance run: $($pin.path)" }
    }
    $result.csv_sha256=(Get-FileHash -LiteralPath $csvPath).Hash
    $result.paint_csv_sha256=(Get-FileHash -LiteralPath $paintPath).Hash
    $result.outcome='passed'
} catch { $result.outcome='failed';$result.failure=$_.Exception.Message;throw }
finally { $env:PATH=$previousPath;[Console]::OutputEncoding=$previousNativeEncoding;$result.console_output_encoding_restored=([Console]::OutputEncoding.CodePage -eq $previousNativeEncoding.CodePage);$result.completed_utc=[DateTime]::UtcNow.ToString('o');$result.observed_outputs=[ordered]@{main=(Test-Path -LiteralPath $csvPath -PathType Leaf);paint=(Test-Path -LiteralPath $paintPath -PathType Leaf)};Save-Result }
Write-Output "PASS device-free balance run. Main CSV: $csvPath; paint CSV: $paintPath; evidence: $receipt"
