[CmdletBinding()]
param([string] $OutputDirectory)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot 'check-path.ps1')

$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$runner = Join-Path $PSScriptRoot 'run-acceptance.ps1'
$outputRoot = if ($OutputDirectory) { Resolve-CheckFileSystemPath $OutputDirectory } else {
    Join-Path $repo 'out/validation/acceptance-selection'
}
$run = [IO.Path]::GetFullPath((Join-Path $outputRoot ([Guid]::NewGuid().ToString('N'))))
$prefix = $outputRoot.TrimEnd([char[]]@('\', '/')) + [IO.Path]::DirectorySeparatorChar
if (-not $run.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase) -or (Test-Path -LiteralPath $run)) {
    throw 'Selection-check scratch must be a new directory beneath the selected output root.'
}
[IO.Directory]::CreateDirectory($run) | Out-Null
if (([IO.File]::GetAttributes($run) -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
    throw 'Selection-check scratch must be an ordinary directory.'
}
$working = Join-Path $run 'working'
[IO.Directory]::CreateDirectory($working) | Out-Null
$utf8 = [Text.UTF8Encoding]::new($false, $true)
$checks = [Collections.Generic.List[object]]::new()
$failures = [Collections.Generic.List[string]]::new()
$observations = @{}
$initialPath = $env:PATH
$initialProfile = $env:LOCALAPPDATA
$initialLocation = (Get-Location).Path
$initialProcessDirectory = [Environment]::CurrentDirectory
$missingCmake = 'Run this script from an x64 Visual Studio Developer shell (cmake is missing).'
$result = [ordered]@{
    schema_version = 1
    scope = 'actual-acceptance-selection-preflight'
    outcome = 'running'
    run = $run
    shell_version = $PSVersionTable.PSVersion.ToString()
    shell_edition = $PSVersionTable.PSEdition
    started_utc = [DateTime]::UtcNow.ToString('o')
    checks = $checks
    cmake_or_compiler_started = $false
    native_helper_loaded = $false
}

function Save-Result {
    [IO.File]::WriteAllText((Join-Path $run 'result.json'), ($result | ConvertTo-Json -Depth 12), $utf8)
}

function File-SHA256([string] $Path) {
    $stream = $null
    $algorithm = $null
    try {
        $stream = [IO.File]::OpenRead($Path)
        $algorithm = [Security.Cryptography.SHA256]::Create()
        return [BitConverter]::ToString($algorithm.ComputeHash($stream)).Replace('-', '')
    } finally {
        if ($null -ne $algorithm) { $algorithm.Dispose() }
        if ($null -ne $stream) { $stream.Dispose() }
    }
}

function Require([bool] $Condition, [string] $Message) {
    if (-not $Condition) { throw $Message }
}

function Require-Names([string[]] $Actual, [string[]] $Expected) {
    Require ($Actual.Count -eq $Expected.Count) 'The selected case count differs.'
    for ($index = 0; $index -lt $Expected.Count; ++$index) {
        Require ($Actual[$index] -ceq $Expected[$index]) "Selection order/name differs at index $index."
    }
}

function Helper-Loaded {
    return $null -ne ('ColourWars.Acceptance.OwnedCommand' -as [type])
}

function Invoke-Selection([string] $Name, [hashtable] $Options,
    [string] $OutputArgument = '', [string] $ExpectedOutput = '') {
    # Dot-sourcing inside this local function exposes the actual selectedCases
    # while containing the runner's variables, functions and strict-mode scope.
    $selectedCases = $null
    $evidence = $null
    $caught = $null
    $beforePath = $env:PATH
    $beforeProfile = $env:LOCALAPPDATA
    $beforeLocation = (Get-Location).Path
    $beforeProcessDirectory = [Environment]::CurrentDirectory
    $privateOutput = if ($ExpectedOutput) { $ExpectedOutput } else { Join-Path $run ('runner-' + $Name) }
    $Options.OutputDirectory = if ($OutputArgument) { $OutputArgument } else { $privateOutput }
    $Options.SkipBuild = $true
    try {
        $env:PATH = ''
        foreach ($tool in @('cmake', 'cl')) {
            Require (-not (Get-Command $tool -ErrorAction SilentlyContinue)) `
                "Run qualification in a fresh -NoProfile host without a preexisting $tool command."
        }
        try { . $runner @Options } catch { $caught = $_ }
        Require ($null -ne $caught) 'The actual runner did not stop during selection/tool preflight.'
        Require (-not (Helper-Loaded)) 'Selection preflight loaded the native-process helper.'
        $actualOutput = if ($null -ne $evidence) { [string]$evidence } else { $privateOutput }
        Require ($actualOutput.StartsWith($run + [IO.Path]::DirectorySeparatorChar,
            [StringComparison]::OrdinalIgnoreCase)) 'Acceptance preflight output escaped its private checker scratch.'
        $files = @(if ([IO.Directory]::Exists($actualOutput)) {
            Get-ChildItem -LiteralPath $actualOutput -Recurse -File
        })
        Require ($files.Count -eq 0) 'Selection preflight created build, response, executable or log files.'
        return [pscustomobject]@{
            selected = [string[]]@($selectedCases | Where-Object { $null -ne $_ })
            diagnostic = $caught.Exception.Message
            output_directory = $privateOutput
            output_created = [IO.Directory]::Exists($privateOutput)
            actual_evidence_directory = $evidence
            actual_output_created = [IO.Directory]::Exists($actualOutput)
            artifact_files = $files.Count
            helper_loaded = $false
        }
    } finally {
        $env:PATH = $beforePath
        $env:LOCALAPPDATA = $beforeProfile
        Set-Location -LiteralPath $beforeLocation
        [Environment]::CurrentDirectory = $beforeProcessDirectory
    }
}

function Check-Selection([string] $Name, [hashtable] $Options, [string] $Diagnostic,
    [string[]] $Expected = @(), [switch] $CheckNames) {
    $record = [ordered]@{ name = $Name; outcome = 'running' }
    $checks.Add($record)
    try {
        $observed = Invoke-Selection $Name $Options
        $observations[$Name] = $observed
        $record.observed = $observed
        Require ($observed.diagnostic -ceq $Diagnostic) "Expected '$Diagnostic'; got '$($observed.diagnostic)'."
        if ($CheckNames) { Require-Names $observed.selected $Expected }
        Require ([string]::Equals($env:PATH, $initialPath, [StringComparison]::Ordinal)) 'Selection preflight did not restore caller PATH.'
        Require ((Get-Location).Path -ceq $working) 'Selection preflight did not restore caller location.'
        $record.outcome = 'passed'
        Write-Host "PASS acceptance selection: $Name"
    } catch {
        $record.outcome = 'failed'
        $record.diagnostic = $_.Exception.Message
        $failures.Add("${Name}: $($_.Exception.Message)")
    } finally { Save-Result }
}

function Check-SelectionPaths([string] $Name, [bool] $AbsoluteOutput) {
    $pathRecord = [ordered]@{ name = $Name; outcome = 'running' }
    $checks.Add($pathRecord)
    $pathBeforeLocation = (Get-Location).Path
    $pathBeforeProcessDirectory = [Environment]::CurrentDirectory
    $pathBeforeEnvironment = $env:PATH
    $pathBeforeProfile = $env:LOCALAPPDATA
    try {
        $caseRoot = Join-Path $run $Name
        $providerDirectory = Join-Path $caseRoot ('provider space ' + [char]0x03A9)
        $processDirectory = Join-Path $caseRoot ('process space ' + [char]0x03A9)
        foreach ($directory in @($providerDirectory, $processDirectory)) {
            [IO.Directory]::CreateDirectory($directory) | Out-Null
        }
        $outputLeaf = 'evidence space ' + [char]0x03A9
        $expectedOutput = if ($AbsoluteOutput) { Join-Path $caseRoot $outputLeaf } else { Join-Path $providerDirectory $outputLeaf }
        $expectedEvidence = Join-Path $expectedOutput 'x64-debug'
        $wrongOutput = Join-Path $processDirectory $outputLeaf
        $outputArgument = if ($AbsoluteOutput) { $expectedOutput } else { $outputLeaf }
        Set-Location -LiteralPath $providerDirectory
        [Environment]::CurrentDirectory = $processDirectory
        $pathRecord.provider_directory = $providerDirectory
        $pathRecord.process_directory = $processDirectory
        $pathRecord.output_argument = $outputArgument
        $pathRecord.expected_evidence = $expectedEvidence
        $pathRecord.process_relative_output = $wrongOutput
        $observed = Invoke-Selection $Name @{ Cases = [string[]]@('ammo'); Preset = 'x64-debug' } `
            -OutputArgument $outputArgument -ExpectedOutput $expectedOutput
        $pathRecord.observed = $observed
        $pathRecord.process_relative_output_created = [IO.Directory]::Exists($wrongOutput)
        Require ($observed.diagnostic -ceq $missingCmake) `
            "Path preflight did not reach missing CMake: $($observed.diagnostic)"
        Require ($observed.actual_evidence_directory -ceq $expectedEvidence -and
            $observed.output_created -and -not $pathRecord.process_relative_output_created) `
            'HYG17 acceptance OutputDirectory resolved against process cwd instead of caller PowerShell location.'
        Require ($observed.artifact_files -eq 0 -and -not $observed.helper_loaded) `
            'Acceptance path preflight created build artifacts or loaded the native-process helper.'
        Require-Names $observed.selected @('ammo')
        Require ((Get-Location).Path -ceq $providerDirectory -and
            [Environment]::CurrentDirectory -ceq $processDirectory -and
            [string]::Equals($env:PATH, $pathBeforeEnvironment, [StringComparison]::Ordinal) -and
            [string]::Equals($env:LOCALAPPDATA, $pathBeforeProfile, [StringComparison]::Ordinal)) `
            'Acceptance path preflight changed caller environment or either current directory.'
        $pathRecord.outcome = 'passed'
        Write-Host "PASS acceptance selection: $Name"
    } catch {
        $pathRecord.outcome = 'failed'
        $pathRecord.diagnostic = $_.Exception.Message
        $failures.Add("${Name}: $($_.Exception.Message)")
    } finally {
        $env:PATH = $pathBeforeEnvironment
        $env:LOCALAPPDATA = $pathBeforeProfile
        Set-Location -LiteralPath $pathBeforeLocation
        [Environment]::CurrentDirectory = $pathBeforeProcessDirectory
        Save-Result
    }
}

function Quote-ChildArgument([string] $Value) {
    # These generated CLI arguments have no embedded quotes. Explicit trailing
    # slash escaping keeps ordinary Windows paths intact under both hosts.
    Require (-not ($Value.Contains('"') -or $Value.IndexOf([char]0) -ge 0)) 'Unsupported generated child argument.'
    $slashes = 0
    while ($slashes -lt $Value.Length -and $Value[$Value.Length - 1 - $slashes] -eq [char]92) { ++$slashes }
    return '"' + $Value + ('\' * $slashes) + '"'
}

Save-Result
Write-Host "Acceptance-selection evidence: $run"
try {
    Require (-not (Helper-Loaded)) 'Run selection qualification in a fresh -NoProfile host without the native helper loaded.'
    $caseFile = Join-Path $PSScriptRoot 'acceptance/cases.psd1'
    $sourcePaths = @($PSCommandPath, $runner, $caseFile)
    $pathHelper = Join-Path $PSScriptRoot 'check-path.ps1'
    if (Test-Path -LiteralPath $pathHelper -PathType Leaf) { $sourcePaths += $pathHelper }
    $result.source_hashes = @(foreach ($path in $sourcePaths) {
        [ordered]@{ path = $path; sha256 = File-SHA256 $path }
    })
    Set-Location -LiteralPath $working

    Check-Selection 'native-array' @{ Cases = [string[]]@('settings', 'ammo', 'text_wrap') } $missingCmake `
        @('settings', 'ammo', 'text_wrap') -CheckNames
    Check-Selection 'comma-and-whitespace' @{ Cases = [string[]]@(' ammo , TeXt_WrAp ') } $missingCmake `
        @('ammo', 'text_wrap') -CheckNames
    Check-Selection 'mixed-array-and-commas' @{ Cases = [string[]]@(' SETTINGS , ammo', ' TEXT_WRAP ') } $missingCmake `
        @('settings', 'ammo', 'text_wrap') -CheckNames
    Check-Selection 'trimmed-single' @{ Cases = [string[]]@(' AmMo ') } $missingCmake @('ammo') -CheckNames
    Check-Selection 'exclusions-preserve-order' @{
        Cases = [string[]]@('settings, ammo', 'text_wrap')
        ExcludeCases = [string[]]@(' , AMMO , ', 'ammo')
    } $missingCmake @('settings', 'text_wrap') -CheckNames
    Check-Selection 'default-debug' @{ Preset = 'x64-debug' } $missingCmake
    Check-Selection 'default-release' @{ Preset = 'x64-release' } $missingCmake

    $defaultRecord = [ordered]@{ name = 'default-preset-relationship'; outcome = 'running' }
    $checks.Add($defaultRecord)
    try {
        $debugNames = $observations['default-debug'].selected
        $releaseNames = $observations['default-release'].selected
        Require ($debugNames.Count -gt 1 -and $debugNames[-1] -ceq 'balance_runner') 'Debug defaults lost the final balance_runner case.'
        Require (@($debugNames | Where-Object { $_ -ceq 'balance_runner' }).Count -eq 1) 'Debug defaults repeat balance_runner.'
        Require (@($releaseNames | Where-Object { $_ -ceq 'balance_runner' }).Count -eq 0) 'Release defaults include balance_runner.'
        Require-Names ([string[]]@($debugNames | Where-Object { $_ -cne 'balance_runner' })) $releaseNames
        $defaultRecord.debug_cases = $debugNames.Count
        $defaultRecord.release_cases = $releaseNames.Count
        $defaultRecord.outcome = 'passed'
        Write-Host 'PASS acceptance selection: default-preset-relationship'
    } catch {
        $defaultRecord.outcome = 'failed'
        $defaultRecord.diagnostic = $_.Exception.Message
        $failures.Add("default-preset-relationship: $($_.Exception.Message)")
    } finally { Save-Result }

    # The opt-in group and the Debug pull-request exclusions come from the
    # case declarations, in declaration order.
    $declared = @((Import-PowerShellDataFile -LiteralPath $caseFile).Cases)
    $optInNames = [string[]]@($declared | Where-Object { $_.ContainsKey('Group') -and $_.Group -ceq 'opt-in' } |
        ForEach-Object { $_.Name })
    $slowNames = [string[]]@($declared | Where-Object { $_.ContainsKey('SlowInDebug') -and $_.SlowInDebug } |
        ForEach-Object { $_.Name })
    Require ($optInNames.Count -gt 0 -and $slowNames.Count -gt 0) 'The case declarations lost the opt-in group or the slow Debug cases.'
    $result.opt_in_cases = $optInNames.Count
    $result.slow_debug_cases = $slowNames
    Check-Selection 'opt-in-debug' @{ Preset = 'x64-debug'; OptIn = $true } $missingCmake $optInNames -CheckNames
    Check-Selection 'opt-in-release' @{ Preset = 'x64-release'; OptIn = $true } $missingCmake $optInNames -CheckNames
    Check-Selection 'skip-slow-debug' @{ Preset = 'x64-debug'; SkipSlowDebug = $true } $missingCmake `
        ([string[]]@($observations['default-debug'].selected | Where-Object { $_ -cnotin $slowNames })) -CheckNames
    Check-Selection 'skip-slow-release' @{ Preset = 'x64-release'; SkipSlowDebug = $true } 'SkipSlowDebug applies only to x64-debug.'
    Check-Selection 'opt-in-with-cases' @{ OptIn = $true; Cases = [string[]]@('ammo') } 'Choose -Cases or -OptIn, not both.'
    Check-Selection 'empty-explicit-array' @{ Cases = [string[]]@() } 'Cases must contain at least one case name.'
    Check-Selection 'whitespace-entry' @{ Cases = [string[]]@('ammo', ' ') } 'Cases must contain nonempty names.'
    foreach ($entry in @(
        @{ name = 'empty-middle-token'; value = 'ammo,,text_wrap' },
        @{ name = 'empty-leading-token'; value = ',ammo' },
        @{ name = 'empty-trailing-token'; value = 'ammo,' },
        @{ name = 'whitespace-token'; value = 'ammo, ,text_wrap' })) {
        Check-Selection $entry.name @{ Cases = [string[]]@($entry.value) } 'Cases must contain nonempty names.'
    }
    Check-Selection 'normalized-duplicate' @{ Cases = [string[]]@(' AMMO , ammo ') } 'Select each acceptance case once.'
    Check-Selection 'duplicate-across-array' @{ Cases = [string[]]@('ammo,text_wrap', 'AMMO') } 'Select each acceptance case once.'
    Check-Selection 'first-unknown-name' @{ Cases = [string[]]@('ammo, Ghost_First ,ghost_second') } 'Unknown acceptance case: ghost_first'
    Check-Selection 'unknown-before-duplicate' @{ Cases = [string[]]@('ammo,ammo,ghost') } 'Unknown acceptance case: ghost'
    Check-Selection 'debug-only-before-exclusion' @{
        Preset = 'x64-release'; Cases = [string[]]@(' ammo , BALANCE_RUNNER '); ExcludeCases = [string[]]@('balance_runner')
    } 'balance_runner is a Debug-only developer tool.'
    Check-Selection 'unknown-exclusion' @{
        Cases = [string[]]@('ammo', 'text_wrap'); ExcludeCases = [string[]]@('ghost_excluded')
    } 'Unknown excluded acceptance case: ghost_excluded'
    Check-Selection 'all-excluded' @{
        Cases = [string[]]@('ammo,text_wrap'); ExcludeCases = [string[]]@('text_wrap,ammo')
    } 'Every selected acceptance case was excluded.'

    $childRecord = [ordered]@{ name = 'current-host-file-comma-binding'; outcome = 'running' }
    $checks.Add($childRecord)
    $child = $null
    $childStarted = $false
    try {
        $hostName = if ($PSVersionTable.PSEdition -eq 'Desktop') { 'powershell.exe' } else { 'pwsh.exe' }
        $hostPath = Join-Path $PSHOME $hostName
        Require ([IO.File]::Exists($hostPath)) 'The current PowerShell family executable is missing.'
        $childOutput = Join-Path $run 'runner-file-child'
        $arguments = @('-NoProfile', '-NonInteractive', '-File', $runner, '-SkipBuild',
            '-OutputDirectory', $childOutput, '-Cases', ' ammo , TeXt_WrAp ')
        $start = [Diagnostics.ProcessStartInfo]::new()
        $start.FileName = $hostPath
        $start.Arguments = (@($arguments | ForEach-Object { Quote-ChildArgument $_ }) -join ' ')
        $start.WorkingDirectory = $working
        $start.UseShellExecute = $false
        $start.CreateNoWindow = $true
        $start.RedirectStandardOutput = $true
        $start.RedirectStandardError = $true
        $start.EnvironmentVariables['PATH'] = ''
        $child = [Diagnostics.Process]::new()
        $child.StartInfo = $start
        $childStarted = $child.Start()
        Require $childStarted 'The owned current-host CLI process did not start.'
        $null = $child.Handle
        $stdout = $child.StandardOutput.ReadToEndAsync()
        $stderr = $child.StandardError.ReadToEndAsync()
        $watch = [Diagnostics.Stopwatch]::StartNew()
        while (-not $child.WaitForExit(100)) {
            Require ($watch.ElapsedMilliseconds -lt 15000) 'The owned CLI preflight exceeded its wait budget.'
        }
        Require ($stdout.Wait(2000) -and $stderr.Wait(2000)) 'Owned CLI output did not finish.'
        $text = $stdout.Result + "`n" + $stderr.Result
        $childLog = Join-Path $run 'file-child.log'
        [IO.File]::WriteAllText($childLog, $text, $utf8)
        Require ($child.ExitCode -ne 0) 'The CLI child unexpectedly completed acceptance.'
        Require ($text.Contains('(cmake is missing).')) 'The actual -File comma list did not reach the missing-cmake preflight.'
        Require (@(Get-ChildItem -LiteralPath $childOutput -Recurse -File).Count -eq 0) 'The CLI child created build artifacts.'
        $childRecord.outcome = 'passed'
        $childRecord.process_id = $child.Id
        $childRecord.exit_code = $child.ExitCode
        $childRecord.log = $childLog
        Write-Host 'PASS acceptance selection: current-host-file-comma-binding'
    } catch {
        $childRecord.outcome = 'failed'
        $childRecord.diagnostic = $_.Exception.Message
        $failures.Add("current-host-file-comma-binding: $($_.Exception.Message)")
    } finally {
        if ($null -ne $child) {
            try {
                if ($childStarted -and -not $child.HasExited) {
                    # Only this known preflight host, with no compiler on its PATH.
                    $child.Kill()
                    Require ($child.WaitForExit(3000)) 'Owned CLI preflight cleanup could not be confirmed.'
                }
            } finally { $child.Dispose() }
        }
        Save-Result
    }

    Check-SelectionPaths 'relative-output-provider-location' $false
    Check-SelectionPaths 'absolute-output-independent-location' $true
    Require ($failures.Count -eq 0) ("Acceptance selection failed:`n  " + ($failures -join "`n  "))
    $result.outcome = 'passed'
} catch {
    $result.outcome = 'failed'
    $result.diagnostic = $_.Exception.Message
    throw
} finally {
    $env:PATH = $initialPath
    $env:LOCALAPPDATA = $initialProfile
    Set-Location -LiteralPath $initialLocation
    [Environment]::CurrentDirectory = $initialProcessDirectory
    $result.caller_path_restored = [string]::Equals($env:PATH, $initialPath, [StringComparison]::Ordinal)
    $result.caller_profile_restored = [string]::Equals($env:LOCALAPPDATA, $initialProfile, [StringComparison]::Ordinal)
    $result.caller_location_restored = (Get-Location).Path -ceq $initialLocation
    $result.caller_process_directory_restored = [Environment]::CurrentDirectory -ceq $initialProcessDirectory
    $result.completed_utc = [DateTime]::UtcNow.ToString('o')
    Save-Result
    Write-Host "Acceptance-selection evidence: $run"
}
Write-Host 'PASS acceptance selection without CMake, compiler or native helper startup.'
