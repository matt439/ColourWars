[CmdletBinding()]
param(
    [ValidateSet('x64-debug', 'x64-release')]
    [string] $Preset = 'x64-debug',
    [switch] $SkipBuild,
    [string] $OutputDirectory
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if (-not $OutputDirectory) {
    $OutputDirectory = Join-Path $PSScriptRoot '../out/validation/failure-reporting'
}

function Invoke-Logged {
    param([string] $Command, [string[]] $Arguments, [string] $LogPath)
    $ErrorActionPreference = 'Continue'
    $output = & $Command @Arguments 2>&1
    $code = $LASTEXITCODE
    $output | Out-File -LiteralPath $LogPath -Encoding utf8
    if ($code -ne 0) {
        $output | ForEach-Object { Write-Host $_ }
        throw "$Command exited with $code. See $LogPath"
    }
}

if (-not ('ColourWars.FatalWindowProbe' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace ColourWars
{
    public static class FatalWindowProbe
    {
        private delegate bool EnumWindowProc(IntPtr window, IntPtr parameter);
        [DllImport("user32.dll")]
        private static extern bool EnumWindows(EnumWindowProc callback, IntPtr parameter);
        [DllImport("user32.dll")]
        private static extern bool EnumChildWindows(IntPtr parent, EnumWindowProc callback, IntPtr parameter);
        [DllImport("user32.dll")]
        private static extern uint GetWindowThreadProcessId(IntPtr window, out uint processId);
        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        private static extern int GetClassName(IntPtr window, StringBuilder name, int maximum);
        [DllImport("user32.dll", SetLastError = true)]
        private static extern bool PostMessage(IntPtr window, uint message, IntPtr wParam, IntPtr lParam);
        [DllImport("user32.dll", EntryPoint = "SendMessageTimeoutW", SetLastError = true)]
        private static extern IntPtr SendLength(IntPtr window, uint message, IntPtr wParam,
            IntPtr lParam, uint flags, uint timeout, out IntPtr result);
        [DllImport("user32.dll", EntryPoint = "SendMessageTimeoutW", CharSet = CharSet.Unicode, SetLastError = true)]
        private static extern IntPtr SendText(IntPtr window, uint message, IntPtr wParam,
            StringBuilder text, uint flags, uint timeout, out IntPtr result);

        private static string ClassName(IntPtr window)
        {
            var name = new StringBuilder(256);
            GetClassName(window, name, name.Capacity);
            return name.ToString();
        }

        public static IntPtr FindDialog(int processId)
        {
            IntPtr result = IntPtr.Zero;
            EnumWindows((window, parameter) => {
                uint owner;
                GetWindowThreadProcessId(window, out owner);
                if (owner == processId && ClassName(window) == "#32770") {
                    result = window;
                    return false;
                }
                return true;
            }, IntPtr.Zero);
            return result;
        }

        public static bool HasGameWindow(int processId)
        {
            bool found = false;
            EnumWindows((window, parameter) => {
                uint owner;
                GetWindowThreadProcessId(window, out owner);
                if (owner == processId && ClassName(window) == "ColourWarsWindowClass") {
                    found = true;
                    return false;
                }
                return true;
            }, IntPtr.Zero);
            return found;
        }

        public static string Text(IntPtr window)
        {
            IntPtr length;
            if (SendLength(window, 0x000E, IntPtr.Zero, IntPtr.Zero, 2, 2000, out length) == IntPtr.Zero)
                throw new InvalidOperationException("Owned dialog text-length request timed out");
            long count = length.ToInt64();
            if (count < 0 || count > 131072)
                throw new InvalidOperationException("Owned dialog text exceeds the probe bound");
            if (count == 0) return "";
            var text = new StringBuilder((int)count + 1);
            IntPtr copied;
            if (SendText(window, 0x000D, new IntPtr(text.Capacity), text, 2, 2000, out copied) == IntPtr.Zero)
                throw new InvalidOperationException("Owned dialog text request timed out");
            return text.ToString();
        }

        public static string[] StaticTexts(IntPtr dialog)
        {
            var controls = new List<IntPtr>();
            EnumChildWindows(dialog, (window, parameter) => {
                if (ClassName(window) == "Static") controls.Add(window);
                return true;
            }, IntPtr.Zero);
            var result = new List<string>();
            foreach (IntPtr control in controls) {
                string text = Text(control);
                if (!String.IsNullOrEmpty(text)) result.Add(text);
            }
            return result.ToArray();
        }

        public static void CloseOwnedWindows(int processId)
        {
            EnumWindows((window, parameter) => {
                uint owner;
                GetWindowThreadProcessId(window, out owner);
                if (owner == processId) PostMessage(window, 0x0010, IntPtr.Zero, IntPtr.Zero);
                return true;
            }, IntPtr.Zero);
        }
    }
}
'@
}

function Test-FailureCase {
    param(
        [string] $CaseDirectory,
        [string] $SourceExecutable,
        [IO.FileInfo[]] $Libraries,
        [Collections.IDictionary] $ExpectedMetadata,
        [Collections.IDictionary] $CaseResult,
        [switch] $BlockedLog
    )
    $privateId = [Guid]::NewGuid().ToString('N')
    $binaryMarker = "PRIVATE_BINARY_$privateId"
    $profileMarker = "PRIVATE_PROFILE_$privateId"
    $workingMarker = "PRIVATE_WORKING_$privateId"
    $argumentMarker = "PRIVATE_ARGUMENT_$privateId"
    $environmentMarker = "PRIVATE_ENVIRONMENT_$privateId"
    $saveMarker = "PRIVATE_SAVE_$privateId"
    $binaryDirectory = Join-Path $CaseDirectory $binaryMarker
    $profileDirectory = Join-Path $CaseDirectory $profileMarker
    $workingDirectory = Join-Path $CaseDirectory $workingMarker
    $userDirectory = Join-Path $profileDirectory 'ColourWars'
    $null = New-Item -ItemType Directory -Path $binaryDirectory, $userDirectory, $workingDirectory -Force
    # Avoid first-launch default-save stdout and prove reporting leaves an
    # existing player save intact. This fixture never reads the real profile.
    $savePath = Join-Path $userDirectory 'save_data.json'
    $saveText = '{"resolution":{"x":1280,"y":720},"fullscreen":false,"failure_probe_private":"' + $saveMarker + '"}'
    [IO.File]::WriteAllText($savePath, $saveText, [Text.UTF8Encoding]::new($false))
    $executable = Join-Path $binaryDirectory 'ColourWars.exe'
    Copy-Item -LiteralPath $SourceExecutable -Destination $executable
    foreach ($library in $Libraries) { Copy-Item -LiteralPath $library.FullName -Destination $binaryDirectory }
    # This malformed definition fails before Application construction, so a
    # successful test needs neither a graphics/audio device nor staged assets.
    $malformed = '{"failure_probe":'
    [IO.File]::WriteAllText((Join-Path $binaryDirectory 'tuning.json'), $malformed, [Text.UTF8Encoding]::new($false))
    $reportPath = Join-Path $userDirectory 'last-error.log'
    $sentinel = 'retain the blocked report directory'
    if ($BlockedLog) {
        $null = New-Item -ItemType Directory -Path $reportPath
        [IO.File]::WriteAllText((Join-Path $reportPath 'sentinel.txt'), $sentinel)
    }
    else {
        [IO.File]::WriteAllText($reportPath, 'previous report must be replaced completely')
    }
    $stderrPath = Join-Path $CaseDirectory 'startup.err'
    $stdoutPath = Join-Path $CaseDirectory 'startup.out'
    # executable_directory() keeps its trailing native separator and the
    # startup constant is ./tuning.json; preserve that full original spelling.
    $sourcePath = $binaryDirectory + [IO.Path]::DirectorySeparatorChar + './tuning.json'
    $expectedCause = "read_json_file - '$sourcePath' is not valid JSON: Invalid value. (offset $($malformed.Length))"
    $process = $null
    $previousLocalAppData = $env:LOCALAPPDATA
    $previousPrivateEnvironment = [Environment]::GetEnvironmentVariable('COLOURWARS_FAILURE_PROBE_PRIVATE', 'Process')
    try {
        try {
            $env:LOCALAPPDATA = $profileDirectory
            $env:COLOURWARS_FAILURE_PROBE_PRIVATE = $environmentMarker
            $process = Start-Process -FilePath $executable -WindowStyle Hidden -PassThru `
                -WorkingDirectory $workingDirectory -ArgumentList @('+private_failure_probe', $argumentMarker) `
                -RedirectStandardError $stderrPath -RedirectStandardOutput $stdoutPath
            $null = $process.Handle
        }
        finally {
            $env:LOCALAPPDATA = $previousLocalAppData
            [Environment]::SetEnvironmentVariable('COLOURWARS_FAILURE_PROBE_PRIVATE', $previousPrivateEnvironment, 'Process')
        }
        $timer = [Diagnostics.Stopwatch]::StartNew()
        $dialog = [IntPtr]::Zero
        $caption = ''
        $texts = @()
        while ($timer.Elapsed.TotalSeconds -lt 15) {
            $process.Refresh()
            $dialog = [ColourWars.FatalWindowProbe]::FindDialog($process.Id)
            if ($dialog -ne [IntPtr]::Zero) {
                $caption = [ColourWars.FatalWindowProbe]::Text($dialog)
                $texts = @([ColourWars.FatalWindowProbe]::StaticTexts($dialog) | Sort-Object Length -Descending)
                if ($caption.Length -gt 0 -and $texts.Count -gt 0) { break }
            }
            if ($process.HasExited) { throw "Child exited before the expected failure dialog (code $($process.ExitCode))." }
            Start-Sleep -Milliseconds 100
        }
        if ($dialog -eq [IntPtr]::Zero) { throw 'The actual entry point did not create its failure dialog within 15 seconds.' }
        if ($texts.Count -eq 0) { throw 'The owned failure dialog has no readable message body.' }
        $body = ([string]$texts[0]).Replace("`r`n", "`n").Replace("`r", "`n")
        [IO.File]::WriteAllText((Join-Path $CaseDirectory 'dialog-caption.txt'), $caption, [Text.UTF8Encoding]::new($false))
        [IO.File]::WriteAllText((Join-Path $CaseDirectory 'dialog-body.txt'), $body, [Text.UTF8Encoding]::new($false))
        if ($caption -cne 'ColourWars - startup failure') { throw "Unexpected owned failure caption: $caption" }
        if ($body -notmatch '^Failure while [^\r\n]+\.\n\n' -or
            -not $body.Contains("`n`n$expectedCause`n`n")) {
            throw 'The failure dialog did not preserve the complete original file/parser/offset cause.'
        }
        $status = if ($BlockedLog) { 'Diagnostic metadata could not be saved.' } else {
            'Diagnostic metadata saved to %LOCALAPPDATA%\ColourWars\last-error.log.'
        }
        if (-not $body.EndsWith($status, [StringComparison]::Ordinal)) {
            throw 'The failure dialog did not report the actual metadata-write outcome.'
        }
        if ([ColourWars.FatalWindowProbe]::HasGameWindow($process.Id)) {
            throw 'Malformed early tuning unexpectedly reached game-window/device initialization.'
        }
        [ColourWars.FatalWindowProbe]::CloseOwnedWindows($process.Id)
        if (-not $process.WaitForExit(10000)) { throw 'The owned failure dialog did not close normally.' }
        $process.WaitForExit()
        $CaseResult.exit_code = $process.ExitCode
        if ($process.ExitCode -ne 1) { throw "Caught startup failure exited with $($process.ExitCode), expected 1." }
        $stderr = [IO.File]::ReadAllText($stderrPath).Replace("`r`n", "`n")
        if ($stderr -cne "startup failure: $expectedCause`n") {
            throw 'Stderr changed or truncated the original startup-failure diagnostic, or added report-status text.'
        }
        if (-not [string]::IsNullOrWhiteSpace([IO.File]::ReadAllText($stdoutPath))) {
            throw 'Unexpected stdout accompanied the early content-definition failure.'
        }
        if ([IO.File]::ReadAllText($savePath) -cne $saveText) {
            throw 'Failure reporting changed the existing isolated player save.'
        }
        if ($BlockedLog) {
            if (-not (Test-Path -LiteralPath $reportPath -PathType Container) -or
                [IO.File]::ReadAllText((Join-Path $reportPath 'sentinel.txt')) -cne $sentinel) {
                throw 'Failed report writing damaged the existing blocking directory.'
            }
        }
        else {
            if (-not (Test-Path -LiteralPath $reportPath -PathType Leaf)) { throw 'The per-user last-error.log was not written.' }
            $record = [IO.File]::ReadAllText($reportPath)
            [IO.File]::WriteAllText((Join-Path $CaseDirectory 'metadata.log'), $record, [Text.UTF8Encoding]::new($false))
            $metadata = @{}
            foreach ($line in ($record -split '\r?\n')) {
                if ($line.Length -eq 0) { continue }
                if ($line -notmatch '^([a-z0-9_]+)=(.*)$') { throw 'Fatal metadata contains a malformed line or raw exception text.' }
                $key = $Matches[1]
                $value = $Matches[2]
                if ($metadata.ContainsKey($key)) { throw "Fatal metadata repeats $key." }
                if (-not $ExpectedMetadata.Contains($key) -and $key -ne 'utc') {
                    throw "Unexpected fatal metadata field $key; this simple runtime_error has no numeric code or literal reason."
                }
                $metadata[$key] = $value
            }
            if ($metadata.Count -ne $ExpectedMetadata.Count + 1 -or -not $metadata.ContainsKey('utc')) {
                throw 'The per-user fatal report omitted required metadata.'
            }
            foreach ($key in $ExpectedMetadata.Keys) {
                if (-not $metadata.ContainsKey($key) -or $metadata[$key] -cne $ExpectedMetadata[$key]) {
                    throw "Unexpected fatal metadata value for $key."
                }
            }
            $timestamp = [DateTime]::MinValue
            if (-not [DateTime]::TryParseExact($metadata.utc, "yyyy-MM-dd'T'HH:mm:ss'Z'",
                [Globalization.CultureInfo]::InvariantCulture,
                [Globalization.DateTimeStyles]::AssumeUniversal, [ref] $timestamp)) {
                throw 'The fatal report UTC timestamp is malformed.'
            }
            foreach ($private in @($binaryMarker, $profileMarker, $workingMarker, $argumentMarker,
                $environmentMarker, $saveMarker, $binaryDirectory, $profileDirectory, $workingDirectory,
                $expectedCause, 'read_json_file', 'failure_probe', 'tuning.json')) {
                if ($record.IndexOf($private, [StringComparison]::OrdinalIgnoreCase) -ge 0) {
                    throw 'The automatic metadata report leaked raw cause, path, argv or environment details.'
                }
            }
            $CaseResult.metadata = $metadata
        }
        if (@(Get-ChildItem -LiteralPath $userDirectory -Filter 'last-error.log.*.tmp' -File).Count -ne 0) {
            throw 'Report writing left a temporary file behind.'
        }
        $CaseResult.caption = $caption
        $CaseResult.phase = 'content_definitions'
        $CaseResult.complete_cause_in_dialog_and_stderr = $true
        $CaseResult.existing_save_preserved = $true
        $CaseResult.metadata_saved = -not $BlockedLog.IsPresent
        $CaseResult.metadata_privacy = if ($BlockedLog) { 'not-written' } else { 'allowlisted-fields-only' }
        $CaseResult.outcome = 'passed'
        Write-Host "PASS actual wWinMain content failure ($($CaseResult.name)): full cause/dialog/stderr, exit1, metadata outcome/privacy"
    }
    catch {
        $CaseResult.diagnostic = $_.Exception.Message
        throw
    }
    finally {
        if ($null -ne $process) {
            $process.Refresh()
            if (-not $process.HasExited) {
                [ColourWars.FatalWindowProbe]::CloseOwnedWindows($process.Id)
                if (-not $process.WaitForExit(3000)) {
                    # Only the process created above; no foreground state or
                    # other game/tool processes are altered.
                    $process.Kill()
                    $null = $process.WaitForExit(3000)
                }
            }
            $process.Dispose()
        }
        $CaseResult | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $CaseDirectory 'result.json') -Encoding utf8
    }
}

$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$build = Join-Path $repo "out/build/$Preset"
$outputRoot = [IO.Path]::GetFullPath($OutputDirectory)
$run = Join-Path (Join-Path $outputRoot $Preset) ([Guid]::NewGuid().ToString('N'))
$null = New-Item -ItemType Directory -Path $run -Force
$cases = [Collections.Generic.List[object]]::new()
$result = [ordered]@{
    preset = $Preset
    outcome = 'failed'
    scope = 'actual-wWinMain-early-content-failure'
    graphics_audio_initialization = 'not-reached; no hardware skip is accepted'
    privacy_scope = 'automatic last-error.log only; isolated stderr/dialog evidence intentionally retains the full original cause'
    cases = @()
    diagnostic = ''
}
Push-Location -LiteralPath $repo
try {
    if (-not $SkipBuild) {
        foreach ($command in 'cmake', 'cl') {
            if (-not (Get-Command $command -ErrorAction SilentlyContinue)) {
                throw "Run from an x64 Visual Studio Developer shell ($command is missing)."
            }
        }
        if (-not $env:INCLUDE -or -not $env:LIB -or
            ($env:VSCMD_ARG_TGT_ARCH -and $env:VSCMD_ARG_TGT_ARCH -ne 'x64')) {
            throw 'Run from an x64 Visual Studio Developer shell.'
        }
        Invoke-Logged -Command 'cmake' -Arguments @('--preset', $Preset) -LogPath (Join-Path $run 'configure.log')
        Invoke-Logged -Command 'cmake' -Arguments @('--build', '--preset', $Preset, '--target', 'ColourWars', '--parallel', '1') `
            -LogPath (Join-Path $run 'build.log')
    }
    $cachePath = Join-Path $build 'CMakeCache.txt'
    if (-not (Test-Path -LiteralPath $cachePath -PathType Leaf)) { throw 'Configured build is missing; run without -SkipBuild.' }
    $cache = @{}
    foreach ($line in Get-Content -LiteralPath $cachePath) {
        if ($line -match '^([^/#][^:]*):[^=]*=(.*)$') { $cache[$Matches[1]] = $Matches[2] }
    }
    $configuration = if ($Preset -eq 'x64-debug') { 'Debug' } else { 'Release' }
    foreach ($key in 'CMAKE_HOME_DIRECTORY', 'CMAKE_BUILD_TYPE', 'LABRADOR_RENDER_BACKEND', 'LABRADOR_AUDIO_BACKEND') {
        if (-not $cache.ContainsKey($key)) { throw "Configured build is missing $key." }
    }
    if ([IO.Path]::GetFullPath($cache.CMAKE_HOME_DIRECTORY) -ne $repo -or $cache.CMAKE_BUILD_TYPE -ne $configuration) {
        throw 'The selected build does not match this checkout and configuration.'
    }
    $gameDirectory = Join-Path $build 'game'
    $sourceExecutable = Join-Path $gameDirectory 'ColourWars.exe'
    $buildDetails = Join-Path $build 'generated/build-info.txt'
    foreach ($file in $sourceExecutable, $buildDetails) {
        if (-not (Test-Path -LiteralPath $file -PathType Leaf)) { throw "Missing build output: $file" }
    }
    $details = @{}
    foreach ($line in Get-Content -LiteralPath $buildDetails) {
        if ($line -match '^([^:]+): (.*)$') { $details[$Matches[1]] = $Matches[2] }
    }
    foreach ($key in 'ColourWars', 'Labrador', 'Configuration', 'Render', 'Audio') {
        if (-not $details.ContainsKey($key)) { throw "Build details omit $key." }
    }
    if ($details.Configuration -cne $configuration -or $details.Render -cne $cache.LABRADOR_RENDER_BACKEND -or
        $details.Audio -cne $cache.LABRADOR_AUDIO_BACKEND) { throw 'Build details disagree with the selected configured build.' }
    $expectedMetadata = [ordered]@{
        report_format = '1'
        phase = 'content_definitions'
        category = 'startup'
        game_revision = $details.ColourWars
        engine_revision = $details.Labrador
        configuration = $details.Configuration
        render_backend = $details.Render
        audio_backend = $details.Audio
        cause_0 = 'runtime_error'
        causes_truncated = 'false'
        message_truncated = 'false'
    }
    $libraries = @(Get-ChildItem -LiteralPath $gameDirectory -Filter '*.dll' -File)
    foreach ($name in 'writable-log', 'blocked-log') {
        $caseDirectory = Join-Path $run $name
        $null = New-Item -ItemType Directory -Path $caseDirectory
        $caseResult = [ordered]@{ name = $name; outcome = 'failed'; diagnostic = '' }
        $cases.Add($caseResult)
        Test-FailureCase -CaseDirectory $caseDirectory -SourceExecutable $sourceExecutable `
            -Libraries $libraries -ExpectedMetadata $expectedMetadata -CaseResult $caseResult -BlockedLog:($name -eq 'blocked-log')
    }
    $result.outcome = 'passed'
    $result.diagnostic = 'Both actual-entry content failures retained the complete original dialog/stderr cause and exit1; allowlisted per-user metadata and blocked-write recovery passed without reaching graphics/audio initialization.'
}
catch {
    $result.diagnostic = $_.Exception.Message
    throw
}
finally {
    $result.cases = @($cases.ToArray())
    $result | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $run 'result.json') -Encoding utf8
    Write-Host "Failure-reporting evidence: $run"
    Pop-Location
}
