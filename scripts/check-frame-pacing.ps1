[CmdletBinding()]
param(
    [ValidateSet('x64-debug')]
    [string] $Preset = 'x64-debug',
    [switch] $SkipBuild,
    [switch] $CompileOnly,
    [ValidateSet(1, 2, 4)]
    [int[]] $Players = @(1, 2, 4),
    [ValidateRange(1, 36000)]
    [int] $Frames = 600,
    [ValidateRange(1, 36000)]
    [int] $WarmupUpdates = 120,
    [ValidateSet('1280x720', '1920x1080', '2560x1440')]
    [string] $Resolution = '1280x720',
    # Zero retains ApplicationOptions' machine-dependent default.
    [ValidateRange(0, 64)]
    [int] $MaxThreads = 0,
    [ValidateRange(5, 1800)]
    [int] $TimeoutSeconds = 180,
    [ValidateSet('shared', 'uncontended', 'unknown')]
    [string] $LoadCondition = 'shared',
    [string] $LoadNote,
    [switch] $Visible,
    [string] $Python = 'python',
    [string] $OutputDirectory
)

# Debug diagnostic only. Hidden/occluded Present can return rapidly; these
# intervals cannot establish visible-display 60 Hz or a Release hardware floor.
# The runner never changes native focus, priorities or unrelated processes.
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $PSScriptRoot '../out/validation/frame-pacing' }
if ([string]::IsNullOrWhiteSpace($LoadNote)) {
    if (-not $CompileOnly) { throw 'Record the concurrent workload or quiet-run conditions explicitly with -LoadNote.' }
    $LoadNote = 'Compile only; no workload or load-condition claim.'
}
if ($Players.Count -eq 0 -or @($Players | Select-Object -Unique).Count -ne $Players.Count) {
    throw 'Choose each player count at most once, with at least one capture.'
}

function Write-Json {
    param([string] $Path, [object] $Value)
    [IO.File]::WriteAllText($Path, ($Value | ConvertTo-Json -Depth 12), [Text.UTF8Encoding]::new($false))
}

function Quote-Path {
    param([string] $Path)
    if ($Path.Contains('"') -or $Path.Contains("`r") -or $Path.Contains("`n")) {
        throw 'A path contains an unsupported quote or newline.'
    }
    return '"' + [IO.Path]::GetFullPath($Path) + '"'
}

function Invoke-Logged {
    param([string] $Command, [string[]] $Arguments, [string] $LogPath)
    $ErrorActionPreference = 'Continue'
    $lines = & $Command @Arguments 2>&1
    $code = $LASTEXITCODE
    $lines | Out-File -LiteralPath $LogPath -Encoding utf8
    if ($code -ne 0) {
        $lines | ForEach-Object { Write-Host $_ }
        throw "$Command exited with $code. See $LogPath"
    }
}

function Get-MachineMetadata {
    $machine = [ordered]@{
        hostname = [Environment]::MachineName
        os = $null
        cpu = @()
        ram_bytes = $null
        video_controllers = @()
        metadata_warnings = @()
    }
    try {
        $os = Get-CimInstance -ClassName Win32_OperatingSystem
        $machine.os = [ordered]@{ caption = $os.Caption; version = $os.Version; build = $os.BuildNumber }
    } catch { $machine.metadata_warnings += 'OS metadata unavailable: ' + $_.Exception.Message }
    try {
        $machine.cpu = @(Get-CimInstance -ClassName Win32_Processor |
            Select-Object Name, Manufacturer, NumberOfCores, NumberOfLogicalProcessors, MaxClockSpeed)
    } catch { $machine.metadata_warnings += 'CPU metadata unavailable: ' + $_.Exception.Message }
    try {
        $machine.ram_bytes = (Get-CimInstance -ClassName Win32_ComputerSystem).TotalPhysicalMemory
    } catch { $machine.metadata_warnings += 'RAM metadata unavailable: ' + $_.Exception.Message }
    try {
        # Enumeration supplies driver context; device-metadata.json records
        # the adapter actually selected by Renderer, including its numeric IDs.
        $machine.video_controllers = @(Get-CimInstance -ClassName Win32_VideoController |
            Select-Object Name, PNPDeviceID, DriverVersion, DriverDate, AdapterRAM,
                CurrentHorizontalResolution, CurrentVerticalResolution, CurrentRefreshRate)
    } catch { $machine.metadata_warnings += 'Driver metadata unavailable: ' + $_.Exception.Message }
    return $machine
}

$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$build = Join-Path $repo "out/build/$Preset"
$run = Join-Path ([IO.Path]::GetFullPath($OutputDirectory)) ([Guid]::NewGuid().ToString('N'))
$null = New-Item -ItemType Directory -Path $run -Force
$result = [ordered]@{
    outcome = 'failed'
    scope = 'debug-real-application-frame-pacing-diagnostic'
    baseline_accepted = $false
    load_condition = $LoadCondition
    load_note = $LoadNote
    shared_load = $LoadCondition -eq 'shared'
    hidden = -not $Visible.IsPresent
    requested_player_counts = $Players
    runs = @()
    diagnostic = ''
}
$previousPath = $env:PATH
$previousLocalAppData = $env:LOCALAPPDATA
$process = $null
$failure = $null
Push-Location -LiteralPath $repo
try {
    foreach ($command in 'cmake', 'cl', 'git') {
        if (-not (Get-Command $command -ErrorAction SilentlyContinue)) {
            throw "Run from an x64 Visual Studio Developer shell ($command is missing)."
        }
    }
    if (-not $env:INCLUDE -or -not $env:LIB -or
        ($env:VSCMD_ARG_TGT_ARCH -and $env:VSCMD_ARG_TGT_ARCH -ne 'x64')) {
        throw 'This probe requires the x64 Visual Studio Developer environment.'
    }
    if (-not $SkipBuild) {
        Invoke-Logged 'cmake' @('--preset', $Preset) (Join-Path $run 'configure.log')
        Invoke-Logged 'cmake' @('--build', '--preset', $Preset, '--parallel', '1') (Join-Path $run 'build.log')
    }
    $cachePath = Join-Path $build 'CMakeCache.txt'
    if (-not (Test-Path -LiteralPath $cachePath -PathType Leaf)) { throw "No configured build: $build" }
    $cache = @{}
    foreach ($line in Get-Content -LiteralPath $cachePath) {
        if ($line -match '^([^/#][^:]*):[^=]*=(.*)$') { $cache[$Matches[1]] = $Matches[2] }
    }
    foreach ($key in 'CMAKE_BUILD_TYPE', 'CMAKE_CXX_COMPILER', 'CMAKE_HOME_DIRECTORY',
        'LABRADOR_RENDER_BACKEND', 'LABRADOR_AUDIO_BACKEND', 'VCPKG_INSTALLED_DIR', 'VCPKG_TARGET_TRIPLET') {
        if (-not $cache.ContainsKey($key)) { throw "Configured build is missing $key." }
    }
    if ([IO.Path]::GetFullPath($cache.CMAKE_HOME_DIRECTORY) -ne $repo -or
        $cache.CMAKE_BUILD_TYPE -ne 'Debug' -or $cache.LABRADOR_RENDER_BACKEND -ne 'd3d11' -or
        $cache.LABRADOR_AUDIO_BACKEND -ne 'xaudio2' -or $cache.VCPKG_TARGET_TRIPLET -ne 'x64-windows') {
        throw 'The capture requires this checkout and its Debug d3d11/xaudio2 x64-windows build.'
    }
    $gameDirectory = Join-Path $build 'game'
    $objectDirectory = Join-Path $gameDirectory 'CMakeFiles/ColourWars.dir'
    $objects = @(Get-ChildItem -LiteralPath $objectDirectory -Filter '*.obj' -File -Recurse |
        Where-Object {
            # GameMenu can construct GameLevel indirectly; its test factory
            # must never resolve to the physical motor adapter.
            if ($_.Name -in @('main.cpp.obj', 'rumble_xinput.cpp.obj')) { return $false }
            if ($_.Name -eq 'cmake_pch.cxx.obj') { return $true }
            $sourceRelative = $_.FullName.Substring($objectDirectory.Length + 1)
            $sourceRelative = $sourceRelative.Substring(0, $sourceRelative.Length - '.obj'.Length)
            Test-Path -LiteralPath (Join-Path (Join-Path $repo 'game') $sourceRelative) -PathType Leaf
        } | Sort-Object FullName)
    if ($objects.Count -eq 0) { throw 'Build the Debug game before compiling its frame-pacing harness.' }
    $installed = Join-Path $cache.VCPKG_INSTALLED_DIR $cache.VCPKG_TARGET_TRIPLET
    $libraries = @(
        (Join-Path $build 'external/labrador/engine/LabradorEngine.lib'),
        (Join-Path $build 'external/labrador/engine/math/MattMath.lib'),
        (Join-Path $installed 'debug/lib/DirectXTK.lib'),
        (Join-Path $installed 'debug/lib/xaudio2_9redist.lib')
    )
    foreach ($path in @($cache.CMAKE_CXX_COMPILER) + $libraries + @((Join-Path $gameDirectory 'ColourWars.exe'))) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing build output: $path" }
    }
    $source = Join-Path $PSScriptRoot 'acceptance/frame_pacing.cpp'
    $executable = Join-Path $run 'frame_pacing.exe'
    $responseFile = Join-Path $run 'compile.rsp'
    $arguments = @('/nologo', '/EHsc', '/std:c++20', '/W4', '/WX', '/permissive-', '/MDd',
        '/DDIRECTX_TOOLKIT_IMPORT', '/DUSING_XAUDIO2_REDIST', '/DUSING_XINPUT',
        '/DUNICODE', '/D_UNICODE', '/external:W0')
    foreach ($include in @($repo, (Join-Path $repo 'external/labrador'), (Join-Path $build 'generated'))) {
        $arguments += '/I' + (Quote-Path $include)
    }
    foreach ($include in @((Join-Path $repo 'external/rapidjson/include'), (Join-Path $installed 'include/directxtk'),
        (Join-Path $installed 'include/xaudio2redist'), (Join-Path $installed 'include'))) {
        $arguments += '/external:I' + (Quote-Path $include)
    }
    $arguments += @((Quote-Path $source), ('/Fe:' + (Quote-Path $executable)),
        ('/Fo:' + (Quote-Path (Join-Path $run 'frame_pacing.obj'))))
    $arguments += @($objects | ForEach-Object { Quote-Path $_.FullName })
    $arguments += @($libraries | ForEach-Object { Quote-Path $_ })
    $arguments += @('d3d11.lib', 'dxgi.lib', 'dxguid.lib', 'kernel32.lib', 'user32.lib', 'gdi32.lib',
        'winspool.lib', 'shell32.lib', 'ole32.lib', 'oleaut32.lib', 'uuid.lib', 'comdlg32.lib', 'advapi32.lib', 'comctl32.lib', 'Xinput.lib')
    $arguments | Out-File -LiteralPath $responseFile -Encoding unicode
    Invoke-Logged $cache.CMAKE_CXX_COMPILER @("@$responseFile") (Join-Path $run 'compile.log')
    $result.executable = $executable
    $result.executable_sha256 = (Get-FileHash -LiteralPath $executable).Hash
    if ($CompileOnly) {
        $result.outcome = 'compiled'
        $result.diagnostic = 'Harness compiled; no workload launched and no performance claim.'
    }
    else {
        if (-not (Get-Command $Python -ErrorAction SilentlyContinue)) { throw "Python runtime not found: $Python" }
        $analyzer = Join-Path $PSScriptRoot 'perf/analyze_frame_pacing.py'
        if (-not (Test-Path -LiteralPath $analyzer -PathType Leaf)) { throw "Missing analyzer: $analyzer" }
        $machine = Get-MachineMetadata
        $gameRevision = (& git rev-parse HEAD)
        if ($LASTEXITCODE -ne 0) { throw 'Cannot record the game revision.' }
        $engineRevision = (& git -C external/labrador rev-parse HEAD)
        if ($LASTEXITCODE -ne 0) { throw 'Cannot record the engine revision.' }
        $worktreeStatus = @(& git status --short --ignore-submodules=all)
        if ($LASTEXITCODE -ne 0) { throw 'Cannot record the worktree status.' }
        # Match the build's runtime mirror roots, including any future asset
        # extension, without entering CMakeFiles or hashing compiler outputs.
        $contentFiles = @('manifest.json', 'tuning.json', 'presentation.json' |
            ForEach-Object { Get-Item -LiteralPath (Join-Path $gameDirectory $_) })
        foreach ($directory in 'fonts', 'levels', 'sounds', 'textures') {
            $contentFiles += @(Get-ChildItem -LiteralPath (Join-Path $gameDirectory $directory) -File -Recurse)
        }
        $contentHashes = @($contentFiles |
            Sort-Object FullName | ForEach-Object {
                [ordered]@{ path = $_.FullName.Substring($gameDirectory.Length + 1).Replace('\', '/');
                    sha256 = (Get-FileHash -LiteralPath $_.FullName).Hash }
            })
        $runtimeBinaryHashes = @(Get-ChildItem -LiteralPath $gameDirectory -File -Filter '*.dll' |
            Sort-Object Name | ForEach-Object {
                [ordered]@{ path = $_.Name; sha256 = (Get-FileHash -LiteralPath $_.FullName).Hash }
            })
        foreach ($count in $Players) {
            $captureDirectory = Join-Path $run ($count.ToString() + 'p')
            $profile = Join-Path $captureDirectory 'profile'
            $working = Join-Path $captureDirectory 'working-directory'
            $saveDirectory = Join-Path $profile 'ColourWars'
            $null = New-Item -ItemType Directory -Path $captureDirectory, $working, $saveDirectory -Force
            $savePath = Join-Path $saveDirectory 'save_data.json'
            [IO.File]::WriteAllText($savePath,
                '{"fullscreen":false,"team_palette":"blue_orange","music_volume":0,"sfx_volume":0,"aim_marker":true,"reduce_effects":false,"view_shake":true}',
                [Text.UTF8Encoding]::new($false))
            $saveHash = (Get-FileHash -LiteralPath $savePath).Hash
            $metadataPath = Join-Path $captureDirectory 'run-metadata.json'
            $metadata = [ordered]@{
                schema_version = 1
                outcome = 'running'
                baseline_accepted = $false
                measurement_basis = if ($Visible) { 'visible-debug-real-application-loop' } else { 'hidden-debug-real-application-loop' }
                configuration = 'Debug'
                instrumented = $true
                hidden = -not $Visible.IsPresent
                load_condition = $LoadCondition
                load_note = $LoadNote
                shared_load = $LoadCondition -eq 'shared'
                started_utc = [DateTime]::UtcNow.ToString('o')
                finished_utc = $null
                game_revision = $gameRevision
                engine_revision = $engineRevision
                worktree_status = $worktreeStatus
                executable_sha256 = $result.executable_sha256
                game_executable_sha256 = (Get-FileHash -LiteralPath (Join-Path $gameDirectory 'ColourWars.exe')).Hash
                harness_source_sha256 = (Get-FileHash -LiteralPath $source).Hash
                staged_content_hashes = $contentHashes
                runtime_binary_hashes = $runtimeBinaryHashes
                machine = $machine
                device = $null
                workload = [ordered]@{
                    players = $count; stage = 'close_quarters'; seed = 439; bot = 'paint'; weapon = 'sprayer';
                    teams = @(0..($count - 1) | ForEach-Object { if ($_ % 2 -eq 0) { 'a' } else { 'b' } });
                    timescale = 1; god = $true; infinite_ammo = $true; music_volume = 0; sfx_volume = 0;
                    aim_marker = $true; reduce_effects = $false; view_shake = $true;
                    warmup_updates = $WarmupUpdates; requested_frames = $Frames; resolution = $Resolution;
                    max_threads_requested = $MaxThreads; max_threads = $null; physical_input = $false
                }
                limitation = 'Debug diagnostics only. Hidden/occluded Present may return rapidly; no visible-display 60 Hz, GPU/scan-out, Release or uncontended hardware-baseline acceptance.'
                diagnostic = ''
            }
            $entry = [ordered]@{ players = $count; metadata = $metadataPath; outcome = 'running' }
            $result.runs += $entry
            Write-Json $metadataPath $metadata
            try {
                $env:PATH = "$gameDirectory;$previousPath"
                $env:LOCALAPPDATA = $profile
                $probeArguments = @((Quote-Path $gameDirectory), (Quote-Path $captureDirectory),
                    $count.ToString(), $Frames.ToString(), $WarmupUpdates.ToString(), $Resolution,
                    $MaxThreads.ToString(), $(if ($Visible) { '1' } else { '0' }))
                $process = Start-Process -FilePath $executable -WindowStyle Hidden -PassThru `
                    -WorkingDirectory $working -ArgumentList $probeArguments `
                    -RedirectStandardOutput (Join-Path $captureDirectory 'run.log') `
                    -RedirectStandardError (Join-Path $captureDirectory 'run.err')
                $null = $process.Handle
                $env:PATH = $previousPath
                $env:LOCALAPPDATA = $previousLocalAppData
                $timer = [Diagnostics.Stopwatch]::StartNew()
                while (-not $process.WaitForExit(1000)) {
                    if ($timer.Elapsed.TotalSeconds -ge $TimeoutSeconds) {
                        throw "Owned $($count)P capture exceeded its $TimeoutSeconds-second runtime limit."
                    }
                }
                $process.WaitForExit()
                if ($process.ExitCode -ne 0) { throw "Owned $($count)P capture exited with $($process.ExitCode). See run.err." }
                if ((Get-FileHash -LiteralPath $savePath).Hash -ne $saveHash) { throw 'Capture changed its isolated save fixture.' }
                $metadata.device = Get-Content -LiteralPath (Join-Path $captureDirectory 'device-metadata.json') -Raw | ConvertFrom-Json
                $metadata.workload.max_threads = $metadata.device.max_threads
                $metadata.finished_utc = [DateTime]::UtcNow.ToString('o')
                $metadata.outcome = 'captured'
                Write-Json $metadataPath $metadata
                $capturePath = Join-Path $captureDirectory 'debug/perf/sample.json'
                $framesPath = Join-Path $captureDirectory 'debug/perf/sample.csv'
                $summaryPath = Join-Path $captureDirectory 'summary.json'
                Invoke-Logged $Python @($analyzer, '--capture', $capturePath, '--frames', $framesPath,
                    '--metadata', $metadataPath, '--output', $summaryPath) (Join-Path $captureDirectory 'analysis.log')
                $summary = Get-Content -LiteralPath $summaryPath -Raw | ConvertFrom-Json
                foreach ($inputFile in @(@('capture', $capturePath), @('frames', $framesPath), @('metadata', $metadataPath))) {
                    $actualHash = (Get-FileHash -LiteralPath $inputFile[1]).Hash.ToLowerInvariant()
                    if ($summary.input_sha256.($inputFile[0]) -cne $actualHash) {
                        throw "Analyzer fingerprint mismatch for $($inputFile[0])."
                    }
                }
                if ($summary.baseline_accepted -ne $false -or $summary.capture_valid -ne $true) {
                    throw 'Analyzer must retain diagnostic-only baseline and valid-capture status.'
                }
                $entry.outcome = 'analyzed'
                $entry.summary = $summaryPath
                Write-Host "$($count)P diagnostic captured and analyzed ($LoadCondition load); baseline remains unaccepted."
            }
            catch {
                $metadata.outcome = 'failed'
                $metadata.diagnostic = $_.Exception.Message
                $entry.outcome = 'failed'
                throw
            }
            finally {
                $env:PATH = $previousPath
                $env:LOCALAPPDATA = $previousLocalAppData
                if ($null -ne $process) {
                    if (-not $process.HasExited) {
                        # Retained handle only: never enumerate or alter another task's processes.
                        $process.Kill()
                        $null = $process.WaitForExit(3000)
                    }
                    $process.Dispose()
                    $process = $null
                }
                if ($entry.outcome -ne 'analyzed') {
                    $metadata.finished_utc = [DateTime]::UtcNow.ToString('o')
                    Write-Json $metadataPath $metadata
                }
            }
        }
        $result.outcome = 'diagnostic-complete'
        $result.diagnostic = 'Capture/analyzer workflow completed. Shared/unknown load and hidden Debug runs cannot establish an uncontended visible Release hardware baseline.'
    }
}
catch {
    $failure = $_
    $result.diagnostic = $_.Exception.Message
}
finally {
    $env:PATH = $previousPath
    $env:LOCALAPPDATA = $previousLocalAppData
    Write-Json (Join-Path $run 'result.json') $result
    Write-Host "Frame-pacing evidence: $run"
    Pop-Location
}
if ($null -ne $failure) { throw $failure }
