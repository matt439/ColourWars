[CmdletBinding()]
param(
    [ValidateSet('x64-debug', 'x64-release')]
    [string] $Preset = 'x64-debug',
    [switch] $SkipBuild,
    [string] $OutputDirectory,
    [string[]] $Cases,
    [switch] $CompileOnly
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$acceptanceCases = @('display_awake', 'action_bindings', 'input_bindings', 'keyboard_jump_capture', 'ammo', 'settings', 'match_records', 'remembered_setup', 'stick_settings', 'paint', 'camera_framing', 'view_shake', 'movement', 'movement_contact', 'combat_feedback', 'tool_motion', 'weapon_feedback', 'aim_marker', 'player_identity', 'blocked_shot', 'idle_drip', 'quiet_character', 'stage_selection', 'team_palette', 'team_selection', 'menu_navigation',
        'movement_course', 'cq_blockout', 'crossfeed', 'press_tower', 'resume_countdown', 'debug_console', 'story_audio', 'level_format', 'fatal_report', 'gameplay_tuning', 'menu_presentation', 'weapon_catalog', 'menu_text', 'menu_focus_motion', 'display_trial', 'tunnelling', 'text_wrap', 'terrain', 'lethal_provenance', 'skirmish_bot', 'paint_surfaces', 'balance_regions', 'paint_underfoot')
$availableCases = @($acceptanceCases) + @('balance_runner')
$selectedCases = if ($PSBoundParameters.ContainsKey('Cases')) {
    if (-not $Cases) { throw 'Cases must contain at least one case name.' }
    @($Cases | ForEach-Object {
        if ([string]::IsNullOrWhiteSpace($_)) { throw 'Cases must contain nonempty names.' }
        $_.ToLowerInvariant()
    })
} else {
    @($acceptanceCases) + @(if ($Preset -eq 'x64-debug') { 'balance_runner' })
}
foreach ($case in $selectedCases) {
    if ($case -notin $availableCases) { throw "Unknown acceptance case: $case" }
    if ($case -eq 'balance_runner' -and $Preset -ne 'x64-debug') {
        throw 'balance_runner is a Debug-only developer tool.'
    }
}
if (@($selectedCases | Select-Object -Unique).Count -ne @($selectedCases).Count) {
    throw 'Select each acceptance case once.'
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

$repo = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$build = Join-Path $repo "out/build/$Preset"
$evidence = if ($OutputDirectory) {
    [System.IO.Path]::GetFullPath((Join-Path $OutputDirectory $Preset))
} else {
    Join-Path $repo "out/acceptance/$Preset"
}
[System.IO.Directory]::CreateDirectory($evidence) | Out-Null

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

Push-Location -LiteralPath $repo
$previousPath = $env:PATH
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
    $objectDirectory = Join-Path $gameDirectory 'CMakeFiles/ColourWars.dir'
    $objects = @(Get-ChildItem -LiteralPath $objectDirectory -Filter '*.obj' -File -Recurse |
        Where-Object {
            if ($_.Name -eq 'main.cpp.obj') { return $false }
            if ($_.Name -eq 'cmake_pch.cxx.obj') { return $true }
            # CMake may leave an old object behind after its source is retired.
            $sourceRelative = $_.FullName.Substring($objectDirectory.Length + 1)
            $sourceRelative = $sourceRelative.Substring(0, $sourceRelative.Length - '.obj'.Length)
            Test-Path -LiteralPath (Join-Path (Join-Path $repo 'game') $sourceRelative) -PathType Leaf
        } |
        Sort-Object FullName)
    if ($objects.Count -eq 0) {
        throw 'No game objects exist. Run without -SkipBuild first.'
    }

    $installed = Join-Path $cache.VCPKG_INSTALLED_DIR $cache.VCPKG_TARGET_TRIPLET
    $dependencyLibraries = if ($debug) { Join-Path $installed 'debug/lib' } else {
        Join-Path $installed 'lib'
    }
    $libraries = @(
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
        'oleaut32.lib', 'uuid.lib', 'comdlg32.lib', 'advapi32.lib')
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
    foreach ($case in $selectedCases) {
        $sources = @(Join-Path $PSScriptRoot "acceptance/$case.cpp")
        if ($case -eq 'story_audio') {
            # The real game objects use the engine's audio seam unchanged.
            # Supply its recording backend before the static engine archive,
            # so this case can observe sound calls without opening hardware.
            $sources += Join-Path $repo 'external/labrador/engine/audio/null/audio_device.cpp'
        }
        if ($case -eq 'balance_runner') {
            # Reuse only the reviewed measurement/serializer, not its fixture or main.
            foreach ($shared in @('metrics.cpp','csv.cpp')) {
                $sources += Join-Path $PSScriptRoot ('acceptance/balance_regions/' + $shared)
            }
        }
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
        $arguments += @($objects | ForEach-Object { Quote-ResponsePath $_.FullName })
        $arguments += @($libraries | ForEach-Object { Quote-ResponsePath $_ })
        $arguments += $windowsLibraries
        # CL accepts UTF-16 response files, including checkout paths that are
        # not representable in the machine's active ANSI code page.
        $arguments | Out-File -LiteralPath $responseFile -Encoding unicode
        Write-Host "Compile acceptance: $case ($Preset)"
        Invoke-Logged -Command $cache.CMAKE_CXX_COMPILER -Arguments @("@$responseFile") `
            -LogPath (Join-Path $evidence "$case-build.log")
        if ($CompileOnly) { continue }
        $caseArguments = @($gameDirectory)
        if ($case -eq 'level_format') { $caseArguments += $repo }
        # The new console tool emits UTF-8 paths; scope native decoding only
        # to that executable and restore the caller even after a refusal.
        $previousNativeEncoding = [Console]::OutputEncoding
        try {
            if ($case -eq 'balance_runner') { [Console]::OutputEncoding = [Text.UTF8Encoding]::new($false) }
            Invoke-Logged -Command $executable -Arguments $caseArguments `
                -LogPath (Join-Path $evidence "$case-run.log") -ShowOutput
        } finally {
            if ($case -eq 'balance_runner') { [Console]::OutputEncoding = $previousNativeEncoding }
        }
    }
    $operation = if ($CompileOnly) { 'compilation' } else { 'acceptance' }
    Write-Host "PASS $operation ($Preset). Evidence: $evidence"
}
finally {
    $env:PATH = $previousPath
    Pop-Location
}
