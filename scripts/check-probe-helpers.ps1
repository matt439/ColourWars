# Shared probe preflights and diagnostics, without CMake, MSVC or a GPU.
[CmdletBinding()]
param([string] $OutputDirectory)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot 'check-path.ps1')
. (Join-Path $PSScriptRoot 'probe-build.ps1')
. (Join-Path $PSScriptRoot 'check-diagnostics.ps1')
. (Join-Path $PSScriptRoot 'check-windows.ps1')
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $PSScriptRoot '../out/validation/probe-helpers' }
$run = Join-Path (Resolve-CheckFileSystemPath $OutputDirectory) ([Guid]::NewGuid().ToString('N'))
$repo = Join-Path $run 'checkout with spaces'
$null = New-Item -ItemType Directory -Path $repo -Force
$checks = 0

function Assert-Helper([bool] $Condition, [string] $Message) {
    if (-not $Condition) { throw "check-probe-helpers: $Message" }
    $script:checks++
}
function Assert-Refused([scriptblock] $Action, [string] $Message) {
    $refused = $false
    try { & $Action | Out-Null } catch { $refused = $_.Exception.Message.Contains($Message) }
    Assert-Helper $refused "expected refusal containing '$Message'"
}
function Write-Cache($Cache, [string] $Path) {
    @($Cache.Keys | Sort-Object | ForEach-Object { "${_}:STRING=$($Cache[$_])" }) |
        Set-Content -LiteralPath $Path -Encoding utf8
}

foreach ($preset in 'x64-debug', 'x64-release', 'x64-asan') {
    $debug = $preset -eq 'x64-debug'
    $asan = $preset -eq 'x64-asan'
    $build = Join-Path $repo "out/build/$preset"
    $cachePath = Join-Path $build 'CMakeCache.txt'
    $installed = Join-Path $repo 'vcpkg_installed'
    $cache = @{
        CMAKE_HOME_DIRECTORY = $repo; CMAKE_BUILD_TYPE = $(if ($debug) { 'Debug' } elseif ($asan) { 'RelWithDebInfo' } else { 'Release' })
        CMAKE_CXX_COMPILER = (Join-Path $repo 'cl.exe'); VCPKG_INSTALLED_DIR = $installed
        LABRADOR_RENDER_BACKEND = 'd3d11'; LABRADOR_AUDIO_BACKEND = 'xaudio2'
        LABRADOR_INPUT_BACKEND = 'xinput'; VCPKG_TARGET_TRIPLET = 'x64-windows'
    }
    $sanitizerRuntime = Join-Path $repo 'clang_rt.asan_dynamic-x86_64.dll'
    if ($asan) {
        $cache.CMAKE_CXX_FLAGS = '/EHsc /fsanitize=address'
        [IO.File]::WriteAllText($sanitizerRuntime, 'preflight fixture, not a runtime')
    }
    $dependency = Join-Path $installed ('x64-windows/' + $(if ($debug) { 'debug/lib' } else { 'lib' }))
    $files = @($cache.CMAKE_CXX_COMPILER,
        (Join-Path $build 'game/colourwars_game.lib'),
        (Join-Path $build 'game/colourwars_rumble_gamepads.lib'),
        (Join-Path $build 'game/colourwars_display_awake.lib'),
        (Join-Path $build 'game/ColourWars.exe'), (Join-Path $build 'game/presentation.json'),
        (Join-Path $build 'external/labrador/engine/LabradorEngine.lib'),
        (Join-Path $build 'external/labrador/engine/math/LabradorMath.lib'),
        (Join-Path $dependency 'DirectXTK.lib'), (Join-Path $dependency 'xaudio2_9redist.lib'))
    foreach ($file in $files) {
        $null = New-Item -ItemType Directory -Path (Split-Path -Parent $file) -Force
        [IO.File]::WriteAllText($file, 'preflight fixture, not a compiler or library')
    }
    Write-Cache $cache $cachePath
    $contract = Get-ProbeBuild -Repo $repo -Preset $preset -RequiredContent 'presentation.json'
    Assert-Helper ($contract.Debug -eq $debug -and $contract.Libraries.Count -eq 7) "$preset build and adapters"
    Assert-Helper ($contract.WindowsLibraries -contains 'Xinput.lib') "$preset XInput link library"
    Assert-Helper ($contract.CompileArguments -contains '/DUSING_XINPUT') "$preset XInput definition"
    Assert-Helper ($contract.CompileArguments -contains ('/external:I' + (ConvertTo-ProbeResponsePath (Join-Path $repo 'external/rapidjson/include')))) "$preset client RapidJSON include"
    Assert-Helper ($contract.CompileArguments -contains $(if ($debug) { '/MDd' } else { '/DNDEBUG' })) "$preset runtime flags"
    Assert-Helper ($contract.Asan -eq $asan -and ($contract.CompileArguments -contains '/fsanitize=address') -eq $asan -and
        ($contract.CompileArguments -contains '/Z7') -eq $asan) "$preset sanitizer flags"
    Assert-Helper ($contract.SanitizerRuntimeDirectory -eq $(if ($asan) { $repo } else { $null })) "$preset sanitizer runtime folder"
    if ($asan) {
        foreach ($flags in '/EHsc', '/EHsc /fsanitize=address,fuzzer') {
            $cache.CMAKE_CXX_FLAGS = $flags
            Write-Cache $cache $cachePath
            Assert-Refused { Get-ProbeBuild -Repo $repo -Preset $preset } 'not configured with /fsanitize=address'
        }
        $cache.CMAKE_CXX_FLAGS = '/EHsc /fsanitize=address'
        Write-Cache $cache $cachePath
        Remove-Item -LiteralPath $sanitizerRuntime -Force
        Assert-Refused { Get-ProbeBuild -Repo $repo -Preset $preset } "Install Visual Studio's 'C++ AddressSanitizer' component"
        [IO.File]::WriteAllText($sanitizerRuntime, 'preflight fixture, not a runtime')
    }
    $fakeAdapters = Get-ProbeBuild -Repo $repo -Preset $preset -Adapters @()
    Assert-Helper ($fakeAdapters.Libraries.Count -eq 5 -and -not ($fakeAdapters.Libraries -match 'colourwars_(rumble_gamepads|display_awake)\.lib$')) "$preset fake adapter omissions"
    Assert-Refused { Get-ProbeBuild -Repo $repo -Preset $preset -RequiredContent 'missing.json' } 'Missing build output'
    foreach ($entry in @(@('LABRADOR_INPUT_BACKEND', 'web'), @('CMAKE_BUILD_TYPE', 'Wrong'),
                          @('VCPKG_TARGET_TRIPLET', 'x86-windows'), @('CMAKE_HOME_DIRECTORY', $run))) {
        $original = $cache[$entry[0]]
        $cache[$entry[0]] = $entry[1]
        Write-Cache $cache $cachePath
        Assert-Refused { Get-ProbeBuild -Repo $repo -Preset $preset } 'requires this checkout'
        $cache[$entry[0]] = $original
    }
    Write-Cache $cache $cachePath
}
Assert-Refused { ConvertTo-ProbeResponsePath "bad`npath" } 'unsupported quote or newline'
$captureHeader = Join-Path $repo 'game/debug/capture/debug_perf.h'
$null = New-Item -ItemType Directory -Path (Split-Path -Parent $captureHeader) -Force
$declaration = 'static constexpr std::size_t max_capture_frames = 42;'
[IO.File]::WriteAllText($captureHeader, $declaration)
Assert-Helper ((Get-ProbeCaptureLimit $repo) -eq 42) 'capture limit comes from the recorder declaration'
[IO.File]::WriteAllText($captureHeader, $declaration + $declaration)
Assert-Refused { Get-ProbeCaptureLimit $repo } 'Cannot read DebugPerf::max_capture_frames'

$fallback = "no audio: fixture. The manifest marks this bank optional, so it plays nothing. See the repository's README, 'Audio'."
Assert-CheckDiagnostics -Lines @('', '   ')
Assert-CheckDiagnostics -Lines @($fallback) -RequireNoAudio
Assert-CheckDiagnostics -Lines @('fixture: expected refusal') -AllowedPattern '^fixture: expected refusal$'
$checks += 3
Assert-Refused { Assert-CheckDiagnostics -Lines @($fallback) -RequireAudio } 'unexpectedly used a silent bank'
Assert-Refused { Assert-CheckDiagnostics -RequireNoAudio } 'fallback was not reported'
Assert-Refused { Assert-CheckDiagnostics -Lines @($fallback + ' trailing garbage') } 'Unexpected probe diagnostic'
Assert-Refused { Assert-CheckDiagnostics -Lines @('no audio: fixture. The manifest marks this bank optional, so it plays nothing.') } 'Unexpected probe diagnostic'
Assert-Refused { Assert-CheckDiagnostics -Lines @($fallback.ToUpperInvariant()) } 'Unexpected probe diagnostic'
Assert-Refused { Assert-CheckDiagnostics -Lines @('startup failure: fixture') } 'Unexpected probe diagnostic'

# Create only invisible native windows; destroy exactly those handles afterwards.
if (-not ('ColourWars.CheckOwnedWindowsFixture' -as [type])) {
    Add-Type -TypeDefinition (@'
using System;
using System.Runtime.InteropServices;
namespace ColourWars {
    public static class CheckOwnedWindowsFixture {
        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        private static extern IntPtr CreateWindowEx(uint extended, string kind, string title, uint style,
            int x, int y, int width, int height, IntPtr parent, IntPtr menu, IntPtr instance, IntPtr parameter);
        [DllImport("user32.dll")] private static extern bool DestroyWindow(IntPtr window);
        public static bool Check(int processId) {
            IntPtr window = CreateWindowEx(0, "STATIC", "ColourWars helper fixture", 0,
                0, 0, 1, 1, IntPtr.Zero, IntPtr.Zero, IntPtr.Zero, IntPtr.Zero);
            if (window == IntPtr.Zero) throw new InvalidOperationException("Cannot create the hidden fixture window");
            try {
                string kind = FixtureOwnedWindows.ClassName(window);
                return Array.IndexOf(FixtureOwnedWindows.ForProcess(processId), window) >= 0 &&
                    FixtureOwnedWindows.ForProcess(-1).Length == 0 &&
                    FixtureOwnedWindows.Find(processId, kind) != IntPtr.Zero &&
                    FixtureOwnedWindows.Find(processId, "ColourWarsNonexistentFixtureClass") == IntPtr.Zero;
            }
            finally { DestroyWindow(window); }
        }
    }
}
'@ + (Get-CheckWindowSource 'FixtureOwnedWindows'))
}
Assert-Helper ([ColourWars.CheckOwnedWindowsFixture]::Check($PID)) 'owned window enumeration and class filtering'
Assert-Refused { Get-CheckWindowSource 'bad-name' } 'Invalid window helper class name'
[ordered]@{ outcome = 'passed'; checks = $checks; GPU = $false; cmake_or_msvc = $false } |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path $run 'result.json') -Encoding utf8
Write-Output "PASS shared probe helpers: $checks checks; $run"
