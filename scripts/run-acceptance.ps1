[CmdletBinding()]
param(
    [ValidateSet('x64-debug', 'x64-release')]
    [string] $Preset = 'x64-debug',
    [switch] $SkipBuild,
    [string] $OutputDirectory,
    [string[]] $Cases,
    [string[]] $ExcludeCases,
    [switch] $OptIn,
    [switch] $SkipSlowDebug,
    [ValidateRange(1, 64)]
    [int] $Parallel = 1,
    [switch] $CompileOnly,
    [ValidateRange(1, 86400)]
    [int] $RunTimeoutSeconds = 3600
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

# Each case is declared once, in scripts/acceptance/cases.psd1.
$declaredCases = @((Import-PowerShellDataFile -LiteralPath (Join-Path $PSScriptRoot 'acceptance/cases.psd1')).Cases)
$caseKeys = @('Name', 'Group', 'Sources', 'Arguments', 'StartRank', 'SlowInDebug', 'Utf8Output')
$caseTable = @{}
foreach ($declared in $declaredCases) {
    foreach ($key in $declared.Keys) {
        if ($key -cnotin $caseKeys) { throw "acceptance/cases.psd1: unknown key $key." }
    }
    if ($declared.Name -cnotmatch '^[a-z0-9_]+$' -or $caseTable.ContainsKey($declared.Name)) {
        throw "acceptance/cases.psd1: case names must be unique lower-case identifiers ($($declared.Name))."
    }
    if (-not $declared.ContainsKey('Group')) { $declared.Group = 'default' }
    if ($declared.Group -cnotin @('default', 'debug-only', 'opt-in')) {
        throw "acceptance/cases.psd1: $($declared.Name) has an unknown group."
    }
    # Fill in what a declaration leaves out, so every key reads under strict mode.
    $defaults = @{ Arguments = @('game'); Sources = @(); StartRank = [int]::MaxValue; SlowInDebug = $false; Utf8Output = $false }
    foreach ($key in $defaults.Keys) {
        if (-not $declared.ContainsKey($key)) { $declared[$key] = $defaults[$key] }
    }
    foreach ($argument in $declared.Arguments) {
        if ($argument -cnotin @('game', 'repo')) { throw "acceptance/cases.psd1: $($declared.Name) has an unknown argument." }
    }
    $caseTable[$declared.Name] = $declared
}
$availableCases = @($declaredCases | ForEach-Object { $_.Name })
if ($OptIn -and $PSBoundParameters.ContainsKey('Cases')) { throw 'Choose -Cases or -OptIn, not both.' }
if ($SkipSlowDebug -and $Preset -ne 'x64-debug') { throw 'SkipSlowDebug applies only to x64-debug.' }
$selectedCases = if ($PSBoundParameters.ContainsKey('Cases')) {
    if (-not $Cases) { throw 'Cases must contain at least one case name.' }
    @($Cases | ForEach-Object {
        # '-File' binds comma lists as one string; arrays remain supported.
        foreach ($name in ($_ -split ',')) {
            if ([string]::IsNullOrWhiteSpace($name)) { throw 'Cases must contain nonempty names.' }
            $name.Trim().ToLowerInvariant()
        }
    })
} elseif ($OptIn) {
    @($declaredCases | Where-Object { $_.Group -ceq 'opt-in' } | ForEach-Object { $_.Name })
} else {
    @($declaredCases | Where-Object {
        $_.Group -ceq 'default' -or ($_.Group -ceq 'debug-only' -and $Preset -eq 'x64-debug')
    } | ForEach-Object { $_.Name })
}
foreach ($case in $selectedCases) {
    if ($case -notin $availableCases) { throw "Unknown acceptance case: $case" }
    if ($caseTable[$case].Group -ceq 'debug-only' -and $Preset -ne 'x64-debug') {
        throw "$case is a Debug-only developer tool."
    }
}
if (@($selectedCases | Select-Object -Unique).Count -ne @($selectedCases).Count) {
    throw 'Select each acceptance case once.'
}
if ($ExcludeCases) {
    # 'pwsh -File' passes a comma list as one string.
    $excluded = @($ExcludeCases | ForEach-Object { $_ -split ',' } |
        Where-Object { $_.Trim() } | ForEach-Object { $_.Trim().ToLowerInvariant() })
    foreach ($case in $excluded) {
        if ($case -notin $availableCases) { throw "Unknown excluded acceptance case: $case" }
    }
    $selectedCases = @($selectedCases | Where-Object { $_ -notin $excluded })
    if (-not $selectedCases) { throw 'Every selected acceptance case was excluded.' }
}
if ($SkipSlowDebug) {
    $selectedCases = @($selectedCases | Where-Object { -not $caseTable[$_].SlowInDebug })
    if (-not $selectedCases) { throw 'Every selected acceptance case was excluded.' }
}

function Invoke-Logged {
    param(
        [string] $Command,
        [string[]] $Arguments,
        [string] $LogPath,
        [switch] $ShowOutput
    )

    # Windows PowerShell represents native stderr as ErrorRecord instances.
    # The executable's exit code decides success; preserve both streams.
    $ErrorActionPreference = 'Continue'
    $output = & $Command @Arguments 2>&1
    $code = $LASTEXITCODE
    $output | Out-File -LiteralPath $LogPath -Encoding utf8
    if ($ShowOutput -or $code -ne 0) {
        $output | ForEach-Object { Write-Host $_ }
    }
    if ($code -ne 0) {
        throw "$Command exited with $code. See $LogPath"
    }
}

function Quote-ResponsePath {
    param([string] $Path)
    if ($Path.Contains('"') -or $Path.Contains("`r") -or $Path.Contains("`n")) {
        throw 'A compiler path contains an unsupported quote or newline.'
    }
    return '"' + [System.IO.Path]::GetFullPath($Path) + '"'
}

. (Join-Path $PSScriptRoot 'check-path.ps1')
$repo = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$build = Join-Path $repo "out/build/$Preset"
$evidence = if ($OutputDirectory) {
    Join-Path (Resolve-CheckFileSystemPath $OutputDirectory) $Preset
} else {
    Join-Path $repo "out/acceptance/$Preset"
}
[System.IO.Directory]::CreateDirectory($evidence) | Out-Null
# A timing summary never outlives the run that follows it, even one that stops
# in tool checks or the build; a fresh one is written once cases are scheduled.
$staleSummary = Join-Path $evidence 'acceptance-summary.json'
if (Test-Path -LiteralPath $staleSummary -PathType Leaf) { Remove-Item -LiteralPath $staleSummary -Force }

foreach ($command in 'cmake', 'cl') {
    if (-not (Get-Command $command -ErrorAction SilentlyContinue)) {
        throw "Run this script from an x64 Visual Studio Developer shell ($command is missing)."
    }
}
if (-not $env:INCLUDE -or -not $env:LIB) {
    throw 'Run this script from an x64 Visual Studio Developer shell (INCLUDE/LIB are missing).'
}
if ($env:VSCMD_ARG_TGT_ARCH -and $env:VSCMD_ARG_TGT_ARCH -ne 'x64') {
    throw 'The acceptance executables require the x64 Visual Studio environment.'
}

# Per-case compile and run seconds and outcome, longest first, for every
# selected case. A case the run never reached is 'not-run'.
function Write-AcceptanceSummary {
    $entries = @(foreach ($case in $selectedCases) {
        $result = $timedResults[$case]
        if ($null -ne $result) {
            $compile = [Math]::Round([double]$result.CompileSeconds, 1)
            $run = [Math]::Round([double]$result.RunSeconds, 1)
            $outcome = [string]$result.Outcome
        } else {
            $compile = 0.0
            $run = 0.0
            $outcome = if ($runnerFailures.ContainsKey($case)) { 'runner-failed' }
                elseif ($interruptedCases.ContainsKey($case)) { 'cancelled' } else { 'not-run' }
        }
        [pscustomobject][ordered]@{
            case = $case
            outcome = $outcome
            compile_seconds = $compile
            run_seconds = $run
            total_seconds = [Math]::Round($compile + $run, 1)
        }
    })
    $sorted = @($entries | Sort-Object -Property @{ Expression = 'total_seconds'; Descending = $true },
        @{ Expression = { [array]::IndexOf($selectedCases, $_.case) } })
    $passing = if ($CompileOnly) { 'compiled' } else { 'passed' }
    $summary = [ordered]@{
        schema_version = 1
        preset = $Preset
        compile_only = $CompileOnly.IsPresent
        parallel = $Parallel
        outcome = if (@($sorted | Where-Object { $_.outcome -ne $passing }).Count -eq 0) { 'passed' } else { 'failed' }
        started_utc = $summaryStarted.ToString('o')
        completed_utc = [DateTime]::UtcNow.ToString('o')
        wall_seconds = [Math]::Round($summaryWatch.Elapsed.TotalSeconds, 1)
        shell_edition = $PSVersionTable.PSEdition
        shell_version = $PSVersionTable.PSVersion.ToString()
        cases = $sorted
    }
    [System.IO.File]::WriteAllText($summaryPath, ($summary | ConvertTo-Json -Depth 4),
        [System.Text.UTF8Encoding]::new($false))
    Write-Host "Acceptance summary: $summaryPath"
}

$summaryPath = $null
Push-Location -LiteralPath $repo
$previousPath = $env:PATH
$probeControl = $null
$caseCancellation = [Threading.CancellationTokenSource]::new()
try {
    if (-not $SkipBuild) {
        Write-Host "Configure and build $Preset; logs: $evidence"
        Invoke-Logged -Command 'cmake' -Arguments @('--preset', $Preset) `
            -LogPath (Join-Path $evidence 'configure.log')
        Invoke-Logged -Command 'cmake' -Arguments @('--build', '--preset', $Preset, '--parallel', '1') `
            -LogPath (Join-Path $evidence 'build.log')
    }

    $cachePath = Join-Path $build 'CMakeCache.txt'
    if (-not (Test-Path -LiteralPath $cachePath -PathType Leaf)) {
        throw "No configured build at $build. Run without -SkipBuild first."
    }
    $cache = @{}
    foreach ($line in Get-Content -LiteralPath $cachePath) {
        if ($line -match '^([^/#][^:]*):[^=]*=(.*)$') {
            $cache[$Matches[1]] = $Matches[2]
        }
    }
    foreach ($key in 'CMAKE_BUILD_TYPE', 'CMAKE_CXX_COMPILER', 'CMAKE_HOME_DIRECTORY',
        'LABRADOR_RENDER_BACKEND', 'LABRADOR_AUDIO_BACKEND', 'VCPKG_INSTALLED_DIR',
        'VCPKG_TARGET_TRIPLET') {
        if (-not $cache.ContainsKey($key)) {
            throw "The configured build is missing $key. Configure it again."
        }
    }
    if ([System.IO.Path]::GetFullPath($cache.CMAKE_HOME_DIRECTORY) -ne $repo) {
        throw 'The configured build belongs to another checkout.'
    }
    if ($cache.LABRADOR_RENDER_BACKEND -ne 'd3d11' -or
        $cache.LABRADOR_AUDIO_BACKEND -ne 'xaudio2' -or
        $cache.VCPKG_TARGET_TRIPLET -ne 'x64-windows') {
        throw 'These consumer acceptance cases use the d3d11/xaudio2 x64-windows game build.'
    }
    $debug = $Preset -eq 'x64-debug'
    $configuration = if ($debug) { 'Debug' } else { 'Release' }
    if ($cache.CMAKE_BUILD_TYPE -ne $configuration) {
        throw "$Preset must contain a $configuration build."
    }

    $gameDirectory = Join-Path $build 'game'
    $installed = Join-Path $cache.VCPKG_INSTALLED_DIR $cache.VCPKG_TARGET_TRIPLET
    $dependencyLibraries = if ($debug) { Join-Path $installed 'debug/lib' } else {
        Join-Path $installed 'lib'
    }
    # The game library is every game source but main.cpp; the cases also link
    # both platform adapters, as the executable does (game/CMakeLists.txt).
    $libraries = @(
        (Join-Path $gameDirectory 'colourwars_game.lib'),
        (Join-Path $gameDirectory 'colourwars_rumble_xinput.lib'),
        (Join-Path $gameDirectory 'colourwars_display_awake.lib'),
        (Join-Path $build 'external/labrador/engine/LabradorEngine.lib'),
        (Join-Path $build 'external/labrador/engine/math/MattMath.lib'),
        (Join-Path $dependencyLibraries 'DirectXTK.lib'),
        (Join-Path $dependencyLibraries 'xaudio2_9redist.lib')
    )
    foreach ($path in @($cache.CMAKE_CXX_COMPILER) + $libraries +
        @((Join-Path $gameDirectory 'ColourWars.exe'), (Join-Path $gameDirectory 'tuning.json'))) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "A required build output is missing: $path"
        }
    }
    $windowsLibraries = @('d3d11.lib', 'dxgi.lib', 'dxguid.lib', 'kernel32.lib',
        'user32.lib', 'gdi32.lib', 'winspool.lib', 'shell32.lib', 'ole32.lib',
        'oleaut32.lib', 'uuid.lib', 'comdlg32.lib', 'advapi32.lib', 'comctl32.lib', 'Xinput.lib')
    $common = @('/nologo', '/EHsc', '/std:c++20', '/W4', '/WX', '/permissive-',
        '/DDIRECTX_TOOLKIT_IMPORT', '/DUSING_XAUDIO2_REDIST', '/DUSING_XINPUT',
        '/DUNICODE', '/D_UNICODE', '/external:W0')
    if ($debug) { $common += '/MDd' } else { $common += @('/MD', '/DNDEBUG') }
    foreach ($include in @($repo, (Join-Path $repo 'external/labrador'),
        (Join-Path $build 'generated'))) {
        $common += '/I' + (Quote-ResponsePath $include)
    }
    foreach ($include in @((Join-Path $repo 'external/rapidjson/include'),
        (Join-Path $installed 'include/directxtk'),
        (Join-Path $installed 'include/xaudio2redist'), (Join-Path $installed 'include'))) {
        $common += '/external:I' + (Quote-ResponsePath $include)
    }

    # The probes use CPU-side game data and do not create a window or hardware device.
    # DLL imports still resolve from the same staged folder as the game.
    # A case may own a same-named folder of extra .cpp sources.
    $env:PATH = "$gameDirectory;$previousPath"
    if (-not $CompileOnly) {
        . (Join-Path $PSScriptRoot 'acceptance-process.ps1')
        Initialize-AcceptanceProcess
        $probeControl = [ColourWars.Acceptance.OwnedRunControl]::new()
    }
    $specs = @()
    foreach ($case in $selectedCases) {
        $declared = $caseTable[$case]
        $sources = @(Join-Path $PSScriptRoot "acceptance/$case.cpp")
        $sources += @($declared.Sources | ForEach-Object { Join-Path $repo $_ })
        $parts = Join-Path $PSScriptRoot "acceptance/$case"
        if (Test-Path -LiteralPath $parts -PathType Container) {
            $sources += @(Get-ChildItem -LiteralPath $parts -Filter '*.cpp' -File |
                Sort-Object Name | ForEach-Object { $_.FullName })
        }
        $executable = Join-Path $evidence "$case.exe"
        $responseFile = Join-Path $evidence "$case.rsp"
        $objectFolder = Join-Path $evidence "$case-obj"
        [System.IO.Directory]::CreateDirectory($objectFolder) | Out-Null
        $null = Quote-ResponsePath $objectFolder  # its quote and newline guard only
        $arguments = $common + @($sources | ForEach-Object { Quote-ResponsePath $_ }) + @(
            ('/Fe:' + (Quote-ResponsePath $executable)),
            # /Fo names a folder only with a trailing separator; '/' because a
            # '\' before the closing quote would escape it.
            ('/Fo:"' + [System.IO.Path]::GetFullPath($objectFolder) + '/"'),
            # Bound probe compiler fan-out while other local projects are active.
            '/MP2'
        )
        $arguments += @($libraries | ForEach-Object { Quote-ResponsePath $_ })
        $arguments += $windowsLibraries
        # CL accepts UTF-16 response files, including checkout paths that are
        # not representable in the machine's active ANSI code page.
        $arguments | Out-File -LiteralPath $responseFile -Encoding unicode
        $caseArguments = @($declared.Arguments | ForEach-Object {
            if ($_ -ceq 'repo') { $repo } else { $gameDirectory }
        })
        $specs += [pscustomobject]@{
            Case = $case
            Compiler = $cache.CMAKE_CXX_COMPILER
            ResponseFile = $responseFile
            Executable = $executable
            Arguments = $caseArguments
            Evidence = $evidence
            WorkingDirectory = $repo
            RunTimeoutSeconds = $RunTimeoutSeconds
            ProbeControl = $probeControl
            CancellationToken = $caseCancellation.Token
            # Native decoding is process-wide, so a UTF-8 case runs on its
            # own after any parallel batch.
            Utf8 = [bool]$declared.Utf8Output
            StartRank = [int]$declared.StartRank
        }
    }

    # acceptance-summary.json is written by every run that reaches this
    # point, even a failed or interrupted one.
    $summaryPath = $staleSummary
    $timedResults = @{}
    $runnerFailures = @{}
    $interruptedCases = @{}
    $summaryWatch = [System.Diagnostics.Stopwatch]::StartNew()
    $summaryStarted = [DateTime]::UtcNow

    # Compiles and runs one case. Self-contained so it can run in a parallel
    # runspace; output is returned rather than printed so cases never interleave.
    $runCase = {
        param($Spec, [bool] $CompileOnly)
        # Windows PowerShell represents native stderr as ErrorRecord instances.
        # The executable's exit code decides success; preserve both streams.
        $ErrorActionPreference = 'Continue'
        $watch = [System.Diagnostics.Stopwatch]::StartNew()
        $lines = @()
        $failure = $null
        $outcome = $null
        $runSeconds = 0.0
        $log = Join-Path $Spec.Evidence "$($Spec.Case)-build.log"
        $compileSkipped = $Spec.CancellationToken.IsCancellationRequested -or
            ($null -ne $Spec.ProbeControl -and $Spec.ProbeControl.IsCancellationRequested)
        if ($compileSkipped) {
            $output = @('Acceptance cancelled before compilation; no probe launched.')
            $code = 1
        } else {
            # Compiler execution remains uncontained: shared compiler-service
            # ownership has not been established by the probe-runtime wrapper.
            $output = & $Spec.Compiler "@$($Spec.ResponseFile)" 2>&1
            $code = $LASTEXITCODE
        }
        $compileSeconds = $watch.Elapsed.TotalSeconds
        $output | Out-File -LiteralPath $log -Encoding utf8
        if ($code -ne 0) {
            $lines = $output
            $outcome = if ($compileSkipped) { 'cancelled' } else { 'compile-failed' }
            $failure = if ($compileSkipped) { "Compilation cancelled before launch. See $log" }
                else { "$($Spec.Compiler) exited with $code. See $log" }
        } elseif ($CompileOnly) {
            $outcome = 'compiled'
        } else {
            $log = Join-Path $Spec.Evidence "$($Spec.Case)-run.log"
            $command = $null
            $runWatch = [System.Diagnostics.Stopwatch]::StartNew()
            try {
                $command = [ColourWars.Acceptance.OwnedCommand]::Start(
                    $Spec.Executable, [string[]]$Spec.Arguments, $Spec.WorkingDirectory,
                    $log, [Console]::OutputEncoding.CodePage, $Spec.RunTimeoutSeconds, $Spec.ProbeControl)
                while (-not $command.Poll(100)) { }
                $native = $command.Result
                $lines = @(Get-Content -LiteralPath $log -Encoding utf8 -ErrorAction Stop)
                if ($native.Outcome -ne 'exited' -or -not $native.CleanupCompleted) {
                    $failure = "$($Spec.Executable): $($native.Outcome): $($native.Message). See $log"
                } elseif ($null -eq $native.ExitCode) {
                    $failure = "$($Spec.Executable) has no confirmed exit code. See $log"
                } elseif ($native.ExitCode -ne 0) {
                    $failure = "$($Spec.Executable) exited with $($native.ExitCode). See $log"
                }
            } catch [System.Management.Automation.PipelineStoppedException] {
                throw
            } catch {
                $failure = "$($Spec.Executable): probe runner failed: $_. See $log"
            } finally {
                if ($null -ne $command) { $command.Dispose() }
                $runSeconds = $runWatch.Elapsed.TotalSeconds
            }
            $outcome = if ($failure) { 'run-failed' } else { 'passed' }
        }
        [pscustomobject]@{
            Case = $Spec.Case
            Lines = @($lines | ForEach-Object { "$_" })
            Failure = $failure
            Outcome = $outcome
            CompileSeconds = $compileSeconds
            RunSeconds = $runSeconds
            Seconds = [int]$watch.Elapsed.TotalSeconds
        }
    }
    $failures = [System.Collections.Generic.List[string]]::new()
    $report = {
        param($Result)
        $timedResults[$Result.Case] = $Result
        Write-Host ("Acceptance: $($Result.Case) ($Preset, $($Result.Seconds)s; compile " +
            "$([int]$Result.CompileSeconds)s, run $([int]$Result.RunSeconds)s)")
        $Result.Lines | ForEach-Object { Write-Host $_ }
        if ($Result.Failure) { $failures.Add("$($Result.Case): $($Result.Failure)") }
    }

    # Start the longest cases first so none of them is left running alone at
    # the end; StartRank in cases.psd1 is from CI timings, slowest first.
    $parallelSpecs = @($specs | Where-Object { -not $_.Utf8 } | Sort-Object { $_.StartRank },
        { [array]::IndexOf($selectedCases, $_.Case) })
    $serialSpecs = @($specs | Where-Object { $_.Utf8 })
    if ($Parallel -gt 1) {
        # A runspace pool rather than ForEach-Object -Parallel, so this also
        # runs under Windows PowerShell. Results print as each case finishes.
        Write-Host "Acceptance: $($parallelSpecs.Count) cases, $Parallel at a time ($Preset)"
        $pool = [runspacefactory]::CreateRunspacePool(1, $Parallel)
        $pool.Open()
        try {
            $pending = [System.Collections.Generic.List[object]]::new()
            foreach ($spec in $parallelSpecs) {
                $shell = [powershell]::Create()
                $shell.RunspacePool = $pool
                [void]$shell.AddScript($runCase.ToString()).AddArgument($spec).AddArgument($CompileOnly.IsPresent)
                $pending.Add([pscustomobject]@{ Case = $spec.Case; Shell = $shell; Handle = $shell.BeginInvoke() })
            }
            while ($pending.Count -gt 0) {
                $done = @($pending | Where-Object { $_.Handle.IsCompleted })
                if (-not $done) { Start-Sleep -Milliseconds 250; continue }
                foreach ($item in $done) {
                    [void]$pending.Remove($item)
                    try {
                        foreach ($result in $item.Shell.EndInvoke($item.Handle)) { & $report $result }
                    } catch {
                        $failures.Add("runner: $_")
                        $runnerFailures[$item.Case] = "$_"
                    } finally {
                        $item.Shell.Dispose()
                    }
                }
            }
        } finally {
            if ($pending.Count -gt 0) {
                $caseCancellation.Cancel()
                if ($null -ne $probeControl) { $probeControl.Cancel() }
                # Do not Stop compiling pipelines: compiler descendants may be
                # shared. Queued work observes cancellation before compiling;
                # active compilation finishes before its runspace is disposed.
                foreach ($item in $pending) {
                    while (-not $item.Handle.IsCompleted) { [Threading.Thread]::Sleep(100) }
                    # Keep each drained case's timing for the summary only.
                    try {
                        foreach ($result in $item.Shell.EndInvoke($item.Handle)) {
                            if ($null -ne $result.PSObject.Properties['Outcome']) { $timedResults[$result.Case] = $result }
                        }
                    }
                    catch { $runnerFailures[$item.Case] = "$_" }
                    finally { $item.Shell.Dispose() }
                }
            }
            $pool.Dispose()
        }
    } else {
        $serialSpecs = $parallelSpecs + $serialSpecs
    }
    foreach ($spec in $serialSpecs) {
        if ($failures.Count -gt 0) { break }
        Write-Host "Compile acceptance: $($spec.Case) ($Preset)"
        # Scope native decoding to that executable and restore the caller even
        # after a refusal.
        $previousNativeEncoding = [Console]::OutputEncoding
        try {
            if ($spec.Utf8) { [Console]::OutputEncoding = [Text.UTF8Encoding]::new($false) }
            & $report (& $runCase $spec $CompileOnly.IsPresent)
        } catch {
            $runnerFailures[$spec.Case] = "$_"
            throw
        } finally {
            [Console]::OutputEncoding = $previousNativeEncoding
            # Stopped mid-case (Ctrl+C runs no catch): reached, not completed.
            if (-not $timedResults.ContainsKey($spec.Case) -and
                -not $runnerFailures.ContainsKey($spec.Case)) { $interruptedCases[$spec.Case] = $true }
        }
    }
    if ($failures.Count -gt 0) {
        throw "Acceptance failed:`n  " + ($failures -join "`n  ")
    }
    $operation = if ($CompileOnly) { 'compilation' } else { 'acceptance' }
    Write-Host "PASS $operation ($Preset). Evidence: $evidence"
}
finally {
    try {
        if ($null -ne $summaryPath) {
            try {
                Write-AcceptanceSummary
            } catch {
                Write-Warning "Could not write $($summaryPath): $_"
            }
        }
    } finally {
        try {
            $caseCancellation.Cancel()
            if ($null -ne $probeControl) { $probeControl.Dispose() }
        } finally {
            $caseCancellation.Dispose()
            $env:PATH = $previousPath
            Pop-Location
        }
    }
}
