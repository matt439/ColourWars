[CmdletBinding()]
param(
    [string] $OutputDirectory,
    [string] $Compiler = 'cl'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$outputRoot = if ($OutputDirectory) { [IO.Path]::GetFullPath($OutputDirectory) } else {
    Join-Path $repo 'out/validation/acceptance-process'
}
$run = [IO.Path]::GetFullPath((Join-Path $outputRoot ([Guid]::NewGuid().ToString('N'))))
$rootPrefix = $outputRoot.TrimEnd([char[]]@('\', '/')) + [IO.Path]::DirectorySeparatorChar
if (-not $run.StartsWith($rootPrefix, [StringComparison]::OrdinalIgnoreCase) -or
    (Test-Path -LiteralPath $run)) {
    throw 'The tooling scratch directory must be new and beneath its selected output directory.'
}
[IO.Directory]::CreateDirectory($run) | Out-Null
if (([IO.File]::GetAttributes($run) -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
    throw 'The tooling scratch directory must be an ordinary directory.'
}
$working = Join-Path $run ('native space ' + [char]0x03A9)
$logs = Join-Path $run 'logs'
foreach ($directory in @($working, $logs)) { [IO.Directory]::CreateDirectory($directory) | Out-Null }
$executable = Join-Path $working 'owned probe.exe'
$utf8 = [Text.UTF8Encoding]::new($false, $true)
$commands = [Collections.Generic.List[object]]::new()
$controls = [Collections.Generic.List[object]]::new()
$knownProcesses = [Collections.Generic.List[object]]::new()
$checks = [Collections.Generic.List[object]]::new()
$guards = [Collections.Generic.List[object]]::new()
$result = [ordered]@{
    schema_version = 1
    scope = 'owned-acceptance-probe-runtime-only'
    outcome = 'running'
    run = $run
    shell_version = $PSVersionTable.PSVersion.ToString()
    shell_edition = $PSVersionTable.PSEdition
    started_utc = [DateTime]::UtcNow.ToString('o')
    checks = $checks
    guard_checks = $guards
    process_inventory = $false
    compiler_containment = $false
    game_devices = $false
    player_data = $false
}

function Save-Result {
    [IO.File]::WriteAllText((Join-Path $run 'result.json'), ($result | ConvertTo-Json -Depth 12), $utf8)
}

function Get-FileSHA256([string] $Path) {
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

function Assert-True([bool] $Condition, [string] $Message) {
    if (-not $Condition) { throw $Message }
}

function Assert-Contains([string] $Text, [string] $Expected, [string] $Message) {
    Assert-True ($Text.IndexOf($Expected, [StringComparison]::Ordinal) -ge 0) $Message
}

function Invoke-Check([string] $Name, [scriptblock] $Body) {
    $watch = [Diagnostics.Stopwatch]::StartNew()
    $record = [ordered]@{ name = $Name; outcome = 'running' }
    $checks.Add($record)
    try {
        $record.facts = & $Body
        $record.outcome = 'passed'
        Write-Host "PASS acceptance process: $Name"
    } catch {
        $record.outcome = 'failed'
        $record.diagnostic = $_.Exception.Message
        throw
    } finally {
        $record.elapsed_ms = $watch.ElapsedMilliseconds
        Save-Result
    }
}

function New-Control {
    $control = [ColourWars.Acceptance.OwnedRunControl]::new()
    $controls.Add($control)
    return $control
}

function Start-Owned([string] $Name, [string[]] $Arguments, [int] $Seconds, $Control,
    [int] $CodePage = 65001, [string] $File = $executable) {
    $command = [ColourWars.Acceptance.OwnedCommand]::Start($File, $Arguments, $working,
        (Join-Path $logs ($Name + '.log')), $CodePage, $Seconds, $Control)
    $commands.Add($command)
    return $command
}

function Wait-Owned($Command, [int] $BudgetMilliseconds = 20000) {
    $watch = [Diagnostics.Stopwatch]::StartNew()
    while (-not $Command.Poll(50)) {
        Release-ExitedKnownProcesses
        if ($watch.ElapsedMilliseconds -gt $BudgetMilliseconds) {
            throw 'The owned-command helper exceeded the tooling harness wait budget.'
        }
    }
    Release-ExitedKnownProcesses
    Assert-True ($null -ne $Command.Result) 'A terminal command did not return a result.'
    return $Command.Result
}

function Assert-Outcome($Actual, [string] $Outcome) {
    Assert-True ($Actual.Outcome -ceq $Outcome) "Expected $Outcome; got $($Actual.Outcome): $($Actual.Message)"
    Assert-True ([bool]$Actual.CleanupCompleted) "Owned cleanup was not confirmed: $($Actual.Message)"
}

function Read-Log($Actual) {
    Assert-True ([IO.File]::Exists($Actual.LogPath)) 'The final UTF-8 log is missing.'
    return [IO.File]::ReadAllText($Actual.LogPath, $utf8)
}

function Read-LiveRaw([string] $Path) {
    $stream = [IO.FileStream]::new($Path, [IO.FileMode]::Open, [IO.FileAccess]::Read,
        ([IO.FileShare]::ReadWrite -bor [IO.FileShare]::Delete))
    $reader = $null
    try {
        $reader = [IO.StreamReader]::new($stream, $utf8)
        return $reader.ReadToEnd()
    } finally {
        if ($null -ne $reader) { $reader.Dispose() } else { $stream.Dispose() }
    }
}

function Wait-Ready([string] $Path, [int] $BudgetMilliseconds = 10000) {
    $watch = [Diagnostics.Stopwatch]::StartNew()
    while ($watch.ElapsedMilliseconds -le $BudgetMilliseconds) {
        if ([IO.File]::Exists($Path)) {
            try {
                $text = [IO.File]::ReadAllText($Path)
                if ($text -match '(?m)^ready=1\r?$') {
                    $values = @{}
                    foreach ($line in ($text -split "`n")) {
                        if ($line -match '^(pid|child)=([0-9]+)\r?$') { $values[$Matches[1]] = [int]$Matches[2] }
                    }
                    if ($values.ContainsKey('pid') -and $values.ContainsKey('child')) { return $values }
                }
            } catch [IO.IOException] {
                # The fixture publishes this small marker before closing its write handle.
            }
        }
        Start-Sleep -Milliseconds 50
    }
    throw "Owned fixture readiness was not observed: $Path"
}

function Retain-KnownProcess([int] $KnownId) {
    # This ID came from an owned fixture's private readiness file, not inventory.
    $process = [Diagnostics.Process]::GetProcessById($KnownId)
    $null = $process.Handle
    $known = [pscustomobject]@{ process = $process; id = $KnownId; exit_observed = $false; handle_released = $false }
    $knownProcesses.Add($known)
    return $known
}

function Observe-KnownExit($Known) {
    if (-not $Known.handle_released -and $Known.process.WaitForExit(0)) {
        $Known.exit_observed = $true
        # Job accounting can retain a terminated process until its last reference
        # closes. Record the exit proof before releasing this test's known handle.
        $Known.process.Dispose()
        $Known.handle_released = $true
    }
    return $Known.exit_observed
}

function Release-ExitedKnownProcesses {
    foreach ($known in $knownProcesses) { $null = Observe-KnownExit $known }
}

function Assert-Exited($Known, [string] $Message) {
    $watch = [Diagnostics.Stopwatch]::StartNew()
    while (-not (Observe-KnownExit $Known)) {
        if ($watch.ElapsedMilliseconds -gt 5000) { throw $Message }
        Start-Sleep -Milliseconds 50
    }
}

function Wait-Invocation($Handle, [int] $BudgetMilliseconds = 20000) {
    $watch = [Diagnostics.Stopwatch]::StartNew()
    while (-not $Handle.IsCompleted) {
        Release-ExitedKnownProcesses
        if ($watch.ElapsedMilliseconds -gt $BudgetMilliseconds) { throw 'Owned test pipeline did not finish.' }
        Start-Sleep -Milliseconds 50
    }
}

# Only probe code runs in these workers. No compiler is stopped by the tests.
$worker = {
    param($File, [string[]] $NativeArguments, $Working, $Log, $Control, $FinallyMarker)
    $command = $null
    try {
        $command = [ColourWars.Acceptance.OwnedCommand]::Start($File, $NativeArguments,
            $Working, $Log, 65001, 60, $Control)
        while (-not $command.Poll(50)) { Start-Sleep -Milliseconds 1 }
        $command.Result
    } finally {
        if ($null -ne $command) { $command.Dispose() }
        if ($FinallyMarker) { [IO.File]::WriteAllText($FinallyMarker, 'disposed') }
    }
}

$failure = $null
Save-Result
Write-Host "Acceptance-process evidence: $run"
try {
    $compilerCommand = Get-Command $Compiler -ErrorAction Stop
    if (-not $env:INCLUDE -or -not $env:LIB -or
        ($env:VSCMD_ARG_TGT_ARCH -and $env:VSCMD_ARG_TGT_ARCH -ne 'x64')) {
        throw 'Use an x64 Visual Studio Developer shell for the isolated native fixture.'
    }
    $compilerPath = $compilerCommand.Source
    Assert-True ([IO.File]::Exists($compilerPath)) 'The fixture compiler must resolve to an executable.'
    $source = Join-Path $PSScriptRoot 'acceptance-process/probe.cpp'
    $response = Join-Path $run 'probe.rsp'
    $object = Join-Path $run 'probe.obj'
    foreach ($path in @($source, $executable, $object)) {
        Assert-True (-not ($path.Contains('"') -or $path.Contains("`r") -or $path.Contains("`n"))) `
            'The isolated compiler path contains a quote or newline.'
    }
    # Do not job-wrap CL. No /MP or /Zi, including inherited CL/_CL_ options.
    @('/nologo', '/EHsc', '/std:c++20', '/W4', '/WX', '/permissive-', '/MT', '/utf-8',
        ('"' + $source + '"'), ('/Fe:"' + $executable + '"'), ('/Fo:"' + $object + '"')) |
        Out-File -LiteralPath $response -Encoding unicode
    $savedCL = $env:CL
    $savedUnderscoreCL = $env:_CL_
    $savedPreference = $ErrorActionPreference
    Push-Location -LiteralPath $run
    try {
        $env:CL = $null
        $env:_CL_ = $null
        $ErrorActionPreference = 'Continue'
        $compileOutput = & $compilerPath "@$response" 2>&1
        $compileCode = $LASTEXITCODE
        $compileOutput | Out-File -LiteralPath (Join-Path $run 'compile.log') -Encoding utf8
    } finally {
        $ErrorActionPreference = $savedPreference
        $env:CL = $savedCL
        $env:_CL_ = $savedUnderscoreCL
        Pop-Location
    }
    Assert-True ($compileCode -eq 0) "Isolated fixture compilation failed; see $run/compile.log"
    $result.fixture_sha256 = Get-FileSHA256 $executable
    $result.source_hashes = @(foreach ($relative in @('scripts/check-acceptance-process.ps1',
        'scripts/acceptance-process/probe.cpp', 'scripts/acceptance-process.ps1',
        'scripts/acceptance-process/owned-command.cs', 'scripts/run-acceptance.ps1')) {
        [ordered]@{ path = $relative; sha256 = Get-FileSHA256 (Join-Path $repo $relative) }
    })

    # Exercise the real runner's queued CompileOnly guard without executing its
    # top-level environment/build setup or loading the native-process helper.
    $guard = [ordered]@{ name = 'compile-only-queued-cancellation'; scope = 'actual-runCase-guard'; outcome = 'running' }
    $guards.Add($guard)
    $guardWatch = [Diagnostics.Stopwatch]::StartNew()
    $guardCancellation = [Threading.CancellationTokenSource]::new()
    try {
        $runnerPath = Join-Path $PSScriptRoot 'run-acceptance.ps1'
        $runnerTokens = $null
        $runnerErrors = $null
        $runnerAst = [Management.Automation.Language.Parser]::ParseFile($runnerPath,
            [ref]$runnerTokens, [ref]$runnerErrors)
        Assert-True ($runnerErrors.Count -eq 0) 'The actual acceptance runner did not parse.'
        $assignments = @($runnerAst.FindAll({
            param($node)
            $node -is [Management.Automation.Language.AssignmentStatementAst] -and
                $node.Left -is [Management.Automation.Language.VariableExpressionAst] -and
                $node.Left.VariablePath.UserPath -ceq 'runCase'
        }, $true))
        Assert-True ($assignments.Count -eq 1) 'Expected one actual runCase assignment in the acceptance runner.'
        $right = $assignments[0].Right
        Assert-True ($right -is [Management.Automation.Language.CommandExpressionAst] -and
            $right.Expression -is [Management.Automation.Language.ScriptBlockExpressionAst]) `
            'The actual runCase assignment must hold a script-block literal.'
        $actualRunCase = $right.Expression.ScriptBlock.GetScriptBlock()
        $compilerMarker = Join-Path $run 'guard-compiler-must-not-launch.marker'
        $compilerDelegate = {
            param($ResponseFile)
            [IO.File]::WriteAllText($compilerMarker, [string]$ResponseFile)
            throw 'The queued CompileOnly guard invoked its compiler delegate.'
        }.GetNewClosure()
        $guardCase = 'compile-only-cancelled-guard'
        $guardCancellation.Cancel()
        $guardSpec = [pscustomobject]@{
            Case = $guardCase
            Evidence = $logs
            Compiler = $compilerDelegate
            ResponseFile = Join-Path $run 'guard-unused.rsp'
            ProbeControl = $null
            CancellationToken = $guardCancellation.Token
        }
        $helperLoadedBefore = $null -ne ('ColourWars.Acceptance.OwnedCommand' -as [type])
        $guardResults = @(& $actualRunCase $guardSpec $true)
        Assert-True (-not [IO.File]::Exists($compilerMarker)) 'Canceled CompileOnly work invoked compilation.'
        Assert-True ($guardResults.Count -eq 1 -and $guardResults[0].Case -ceq $guardCase) `
            'Canceled CompileOnly work did not return its named case result.'
        $guardResult = $guardResults[0]
        $guardLog = Join-Path $logs ($guardCase + '-build.log')
        Assert-Contains $guardResult.Failure 'Compilation cancelled before launch.' 'The case failure lost its pre-launch cancellation reason.'
        Assert-Contains $guardResult.Failure $guardLog 'The case failure does not identify its cancellation log.'
        $guardText = [IO.File]::ReadAllText($guardLog, $utf8)
        Assert-Contains $guardText 'Acceptance cancelled before compilation; no probe launched.' `
            'The actual runner log lost its pre-compilation cancellation reason.'
        $helperLoadedAfter = $null -ne ('ColourWars.Acceptance.OwnedCommand' -as [type])
        Assert-True ($helperLoadedBefore -or -not $helperLoadedAfter) 'The CompileOnly guard loaded the native-process helper.'
        $guard.outcome = 'passed'
        $guard.facts = [ordered]@{
            case = $guardResult.Case
            failure = $guardResult.Failure
            log = $guardLog
            compiler_marker_absent = $true
            native_launches = 0
            probe_control_is_null = $true
            core_token_cancelled = $guardSpec.CancellationToken.IsCancellationRequested
            helper_loaded_before = $helperLoadedBefore
            helper_loaded_after = $helperLoadedAfter
            runner_script_block_line = $right.Extent.StartLineNumber
        }
        Write-Host 'PASS acceptance runner guard: queued CompileOnly cancellation'
    } catch {
        $guard.outcome = 'failed'
        $guard.diagnostic = $_.Exception.Message
        throw
    } finally {
        $guardCancellation.Dispose()
        $guard.elapsed_ms = $guardWatch.ElapsedMilliseconds
        Save-Result
    }
    . (Join-Path $PSScriptRoot 'acceptance-process.ps1')
    Initialize-AcceptanceProcess

    Invoke-Check 'success-native-argv-and-merged-utf8' {
        $payload = [string[]]@('', 'ordinary', 'two words', ('caf' + [char]0x00E9 + ' ' + [char]0x03A9),
            'say "hello"', 'C:\directory with spaces\', 'two\\before"quote', 'tail\\')
        $control = New-Control
        $command = Start-Owned 'success' (@('emit', '0', 'success', '-') + $payload) 20 $control
        try { $actual = Wait-Owned $command } finally { $command.Dispose() }
        Assert-Outcome $actual 'exited'
        Assert-True ($actual.ExitCode -eq 0) 'Success exit code changed.'
        $text = Read-Log $actual
        foreach ($expected in @('OUT begin success', 'ERR begin success', 'OUT end success', 'ERR end success',
            ('UTF8 caf' + [char]0x00E9 + ' ' + [char]0x03A9), ('ARGC ' + $payload.Count))) {
            Assert-Contains $text $expected 'Merged UTF-8 output is incomplete.'
        }
        for ($index = 0; $index -lt $payload.Count; ++$index) {
            $bytes = [Text.Encoding]::UTF8.GetBytes($payload[$index])
            $hex = [BitConverter]::ToString($bytes).Replace('-', '')
            $line = 'ARG {0} {1} {2}' -f $index, $bytes.Length, $hex
            Assert-True (@($text -split "`r?`n" | Where-Object { $_ -ceq $line }).Count -eq 1) `
                "Native argv round trip differs at argument $index."
        }
        [ordered]@{ exit_code = $actual.ExitCode; native_arguments = $payload.Count; log = $actual.LogPath }
    }

    Invoke-Check 'nonzero-exit' {
        $command = Start-Owned 'failure' @('emit', '37', 'failure', '-') 20 (New-Control)
        try { $actual = Wait-Owned $command } finally { $command.Dispose() }
        Assert-Outcome $actual 'exited'
        Assert-True ($actual.ExitCode -eq 37) 'The native failure exit code was lost.'
        Assert-Contains (Read-Log $actual) 'ERR end failure' 'Failure stderr was lost.'
        [ordered]@{ exit_code = $actual.ExitCode; log = $actual.LogPath }
    }

    Invoke-Check 'missing-executable' {
        $command = Start-Owned 'missing' @() 20 (New-Control) 65001 (Join-Path $working 'absent.exe')
        try { $actual = Wait-Owned $command } finally { $command.Dispose() }
        Assert-Outcome $actual 'launch_failed'
        Assert-True ($null -eq $actual.ExitCode -and $actual.ProcessId -eq 0) 'A missing executable claimed a process exit.'
        Assert-True (-not [string]::IsNullOrWhiteSpace($actual.Message)) 'Launch failure lacks a diagnostic.'
        [ordered]@{ outcome = $actual.Outcome; process_id = $actual.ProcessId; message = $actual.Message }
    }

    Invoke-Check 'legacy-native-decoding-to-utf8' {
        $command = Start-Owned 'legacy' @('legacy') 20 (New-Control) 1252
        try { $actual = Wait-Owned $command } finally { $command.Dispose() }
        Assert-Outcome $actual 'exited'
        $text = Read-Log $actual
        foreach ($prefix in @('OUT', 'ERR')) {
            Assert-Contains $text ($prefix + ' legacy caf' + [char]0x00E9) 'Captured native code-page decoding differs.'
        }
        [ordered]@{ native_code_page = 1252; log = $actual.LogPath }
    }

    Invoke-Check 'large-merged-output' {
        $command = Start-Owned 'burst' @('burst', 'burst', '1024') 20 (New-Control)
        try { $actual = Wait-Owned $command } finally { $command.Dispose() }
        Assert-Outcome $actual 'exited'
        Assert-True ($actual.ExitCode -eq 0) 'Large-output fixture failed.'
        $rows = @((Read-Log $actual) -split "`r?`n" | Where-Object { $_ })
        Assert-True ($rows.Count -eq 2049) 'Merged large-output row count differs.'
        $counts = [Collections.Generic.Dictionary[string, int]]::new([StringComparer]::Ordinal)
        foreach ($row in $rows) {
            if ($counts.ContainsKey($row)) { ++$counts[$row] } else { $counts.Add($row, 1) }
        }
        foreach ($prefix in @('OUT', 'ERR')) {
            for ($index = 0; $index -lt 1024; ++$index) {
                $expected = "$prefix burst $index " + ('x' * 96)
                Assert-True ($counts.ContainsKey($expected) -and $counts[$expected] -eq 1) 'Large output was lost or duplicated.'
            }
        }
        Assert-True ($rows[-1] -ceq 'DONE burst') 'Large-output completion marker is missing.'
        [ordered]@{ rows = $rows.Count; bytes = ([IO.FileInfo]$actual.LogPath).Length; log = $actual.LogPath }
    }

    Invoke-Check 'partial-output-before-timeout' {
        $marker = Join-Path $run 'timeout.ready'
        $command = Start-Owned 'timeout' @('sleep', 'timeout', $marker) 5 (New-Control)
        try {
            $ready = Wait-Ready $marker
            $process = Retain-KnownProcess $ready.pid
            $raw = Join-Path $logs 'timeout.log.raw'
            $watch = [Diagnostics.Stopwatch]::StartNew()
            while ($true) {
                $text = if ([IO.File]::Exists($raw)) { Read-LiveRaw $raw } else { '' }
                if ($text.Contains('OUT ready timeout') -and $text.Contains('ERR ready timeout')) { break }
                Assert-True (-not $command.Poll(50)) 'The command ended before partial output was observable.'
                Assert-True ($watch.ElapsedMilliseconds -lt 4000) 'Partial output was not published while the probe ran.'
            }
            Assert-True (-not (Observe-KnownExit $process)) 'Partial-output observation happened after process exit.'
            $actual = Wait-Owned $command
        } finally { $command.Dispose() }
        Assert-Outcome $actual 'timeout'
        Assert-Exited $process 'Timed-out fixture remained alive.'
        Assert-Contains (Read-Log $actual) 'ERR ready timeout' 'Timeout final log lost partial output.'
        [ordered]@{ outcome = $actual.Outcome; partial_output_before_exit = $true; process_id = $ready.pid; log = $actual.LogPath }
    }

    Invoke-Check 'owned-child-cleanup-and-unrelated-sentinel' {
        $sentinelMarker = Join-Path $run 'sentinel.ready'
        $sentinelControl = New-Control
        $sentinel = Start-Owned 'sentinel' @('sleep', 'sentinel', $sentinelMarker) 60 $sentinelControl
        try {
            $sentinelReady = Wait-Ready $sentinelMarker
            $sentinelProcess = Retain-KnownProcess $sentinelReady.pid
            $parentMarker = Join-Path $run 'tree-parent.ready'
            $childMarker = Join-Path $run 'tree-child.ready'
            $tree = Start-Owned 'tree' @('tree', 'tree', $parentMarker, $childMarker) 5 (New-Control)
            try {
                $parentReady = Wait-Ready $parentMarker
                $childReady = Wait-Ready $childMarker
                Assert-True ($parentReady.child -eq $childReady.pid) 'Owned parent and child readiness identities differ.'
                $parentProcess = Retain-KnownProcess $parentReady.pid
                $childProcess = Retain-KnownProcess $childReady.pid
                $actual = Wait-Owned $tree
            } finally { $tree.Dispose() }
            Assert-Outcome $actual 'timeout'
            Assert-Exited $parentProcess 'Timed-out fixture parent remained alive.'
            Assert-Exited $childProcess 'Timed-out fixture child remained alive.'
            Assert-True (-not $sentinel.Poll(0) -and -not (Observe-KnownExit $sentinelProcess)) `
                'Cleaning the private fixture job affected an unrelated owned sentinel.'
            $sentinelControl.Cancel()
            $sentinelResult = Wait-Owned $sentinel
            Assert-Outcome $sentinelResult 'cancelled'
            Assert-Exited $sentinelProcess 'Sentinel did not exit after its own cancellation.'
        } finally { $sentinel.Dispose() }
        [ordered]@{ parent_id = $parentReady.pid; child_id = $childReady.pid; sentinel_id = $sentinelReady.pid; sentinel_survived_other_cleanup = $true }
    }

    Invoke-Check 'normal-exit-cleans-sleeping-child' {
        $parentMarker = Join-Path $run 'normal-parent.ready'
        $childMarker = Join-Path $run 'normal-child.ready'
        $command = Start-Owned 'normal-tree' @('tree-exit', 'normal-tree', $parentMarker, $childMarker) 20 (New-Control)
        try {
            $parentReady = Wait-Ready $parentMarker
            $childReady = Wait-Ready $childMarker
            Assert-True ($parentReady.child -eq $childReady.pid) 'Normal-exit child identity differs.'
            $childProcess = Retain-KnownProcess $childReady.pid
            Assert-True (-not (Observe-KnownExit $childProcess)) 'The straggler fixture was not alive before normal-exit cleanup.'
            $actual = Wait-Owned $command
        } finally { $command.Dispose() }
        Assert-Outcome $actual 'exited'
        Assert-True ($actual.ExitCode -eq 0 -and $actual.ProcessId -eq $parentReady.pid) 'Normal parent exit code or identity changed.'
        Assert-Exited $childProcess 'Normal parent completion left its sleeping child alive.'
        [ordered]@{ parent_id = $parentReady.pid; parent_exit_code = $actual.ExitCode; child_id = $childReady.pid; child_exit_observed = $childProcess.exit_observed }
    }

    Invoke-Check 'concurrent-log-isolation' {
        $leftMarker = Join-Path $run 'concurrent-left.ready'
        $rightMarker = Join-Path $run 'concurrent-right.ready'
        $release = Join-Path $run 'concurrent.release'
        $left = Start-Owned 'concurrent-left' @('burst-wait', 'left-only', '512', $leftMarker, $release) 20 (New-Control)
        $right = $null
        try {
            $right = Start-Owned 'concurrent-right' @('burst-wait', 'right-only', '512', $rightMarker, $release) 20 (New-Control)
            $leftReady = Wait-Ready $leftMarker
            $rightReady = Wait-Ready $rightMarker
            $leftProcess = Retain-KnownProcess $leftReady.pid
            $rightProcess = Retain-KnownProcess $rightReady.pid
            Assert-True (-not (Observe-KnownExit $leftProcess) -and -not (Observe-KnownExit $rightProcess)) `
                'The concurrent fixture lifetimes did not overlap at their private release barrier.'
            [IO.File]::WriteAllText($release, 'go')
            $leftResult = Wait-Owned $left
            $rightResult = Wait-Owned $right
        } finally {
            $left.Dispose()
            if ($null -ne $right) { $right.Dispose() }
        }
        Assert-Outcome $leftResult 'exited'
        Assert-Outcome $rightResult 'exited'
        Assert-True ($leftResult.ExitCode -eq 0 -and $rightResult.ExitCode -eq 0) 'A concurrent fixture failed.'
        Assert-Exited $leftProcess 'Left concurrent fixture remained alive.'
        Assert-Exited $rightProcess 'Right concurrent fixture remained alive.'
        $leftText = Read-Log $leftResult
        $rightText = Read-Log $rightResult
        Assert-Contains $leftText 'DONE left-only' 'Left concurrent log is incomplete.'
        Assert-Contains $rightText 'DONE right-only' 'Right concurrent log is incomplete.'
        Assert-True (-not $leftText.Contains('right-only') -and -not $rightText.Contains('left-only')) 'Concurrent logs were mixed.'
        [ordered]@{ overlapping_lifetimes = $true; left_id = $leftReady.pid; right_id = $rightReady.pid; left_log = $leftResult.LogPath; right_log = $rightResult.LogPath }
    }

    Invoke-Check 'queued-cancellation-prevents-launch' {
        $control = New-Control
        $pool = [runspacefactory]::CreateRunspacePool(1, 1)
        $pool.Open()
        $workers = [Collections.Generic.List[object]]::new()
        $firstMarker = Join-Path $run 'queue-first.ready'
        $queuedMarker = Join-Path $run 'queue-never-launched.ready'
        try {
            foreach ($spec in @(
                @{ arguments = [string[]]@('sleep', 'queued-first', $firstMarker); log = 'queue-first.log' },
                @{ arguments = [string[]]@('emit', '0', 'queued-second', $queuedMarker); log = 'queue-second.log' })) {
                $shell = [powershell]::Create()
                $shell.RunspacePool = $pool
                [void]$shell.AddScript($worker.ToString()).AddArgument($executable).AddArgument($spec.arguments).
                    AddArgument($working).AddArgument((Join-Path $logs $spec.log)).AddArgument($control).AddArgument($null)
                $workers.Add([pscustomobject]@{ shell = $shell; handle = $shell.BeginInvoke() })
            }
            $firstReady = Wait-Ready $firstMarker
            $firstProcess = Retain-KnownProcess $firstReady.pid
            Assert-True (-not [IO.File]::Exists($queuedMarker)) 'Queued fixture launched before its slot was available.'
            $control.Cancel()
            $outcomes = @()
            foreach ($item in $workers) {
                Wait-Invocation $item.handle
                $returned = @($item.shell.EndInvoke($item.handle))
                Assert-True ($returned.Count -eq 1) 'A queued worker did not return exactly one terminal result.'
                Assert-Outcome $returned[0] 'cancelled'
                $outcomes += $returned[0]
            }
            Assert-Exited $firstProcess 'Active queued fixture did not exit on shared cancellation.'
            Assert-True ($outcomes[1].ProcessId -eq 0 -and -not [IO.File]::Exists($queuedMarker)) 'Cancellation launched the queued native probe.'
        } finally {
            $control.Cancel()
            foreach ($item in $workers) {
                if (-not $item.handle.IsCompleted) { $item.shell.Stop() }
                $item.shell.Dispose()
            }
            $pool.Dispose()
        }
        [ordered]@{ active_id = $firstReady.pid; queued_process_id = $outcomes[1].ProcessId; queued_marker_absent = $true }
    }

    Invoke-Check 'interruptible-serial-pipeline-finally' {
        $control = New-Control
        $marker = Join-Path $run 'serial.ready'
        $finallyMarker = Join-Path $run 'serial.finally'
        $shell = [powershell]::Create()
        $handle = $null
        try {
            [void]$shell.AddScript($worker.ToString()).AddArgument($executable).
                AddArgument([string[]]@('sleep', 'serial', $marker)).AddArgument($working).
                AddArgument((Join-Path $logs 'serial.log')).AddArgument($control).AddArgument($finallyMarker)
            $handle = $shell.BeginInvoke()
            $ready = Wait-Ready $marker
            $process = Retain-KnownProcess $ready.pid
            # This pipeline contains only a disposable probe and short polling,
            # so Stop exercises the same finally path needed by serial Ctrl+C.
            $stop = $shell.BeginStop($null, $null)
            Wait-Invocation $stop 10000
            $shell.EndStop($stop)
            Wait-Invocation $handle 10000
            try { $null = $shell.EndInvoke($handle) } catch {
                Assert-True ($shell.InvocationStateInfo.State -eq 'Stopped') 'The serial pipeline failed for a reason other than stopping.'
            }
            Assert-True ($shell.InvocationStateInfo.State -eq 'Stopped') 'Serial pipeline was not interruptible.'
            Assert-True ([IO.File]::Exists($finallyMarker)) 'Serial pipeline Stop skipped its finally block.'
            Assert-True ([IO.File]::ReadAllText($finallyMarker) -ceq 'disposed') 'Serial finally completion marker differs.'
            Assert-Exited $process 'Serial pipeline finally did not dispose its owned probe.'
        } finally {
            $control.Cancel()
            if ($null -ne $handle -and -not $handle.IsCompleted) { $shell.Stop() }
            $shell.Dispose()
        }
        [ordered]@{ process_id = $ready.pid; pipeline_state = 'Stopped'; finally_disposed = $true; ctrl_c_scope = 'pipeline cancellation path; no injected console input' }
    }

    $result.outcome = 'passed'
} catch {
    $failure = $_
    $result.outcome = 'failed'
    $result.diagnostic = $_.Exception.Message
} finally {
    foreach ($control in $controls) { $control.Cancel() }
    foreach ($known in $knownProcesses) {
        if (-not $known.handle_released) {
            $null = Observe-KnownExit $known
            if (-not $known.handle_released) {
                $known.process.Dispose()
                $known.handle_released = $true
            }
        }
    }
    foreach ($command in $commands) {
        try { $command.Dispose() } catch {
            $result.outcome = 'failed'
            $result.cleanup_diagnostic = $_.Exception.Message
            if ($null -eq $failure) { $failure = $_ }
        }
    }
    foreach ($control in $controls) { $control.Dispose() }
    $result.known_processes = @($knownProcesses | ForEach-Object {
        [ordered]@{ process_id = $_.id; exit_observed = $_.exit_observed; handle_released = $_.handle_released }
    })
    $result.completed_utc = [DateTime]::UtcNow.ToString('o')
    Save-Result
    Write-Host "Acceptance-process evidence: $run"
}
if ($null -ne $failure) { throw $failure }
Write-Host 'PASS isolated acceptance-process qualification.'
