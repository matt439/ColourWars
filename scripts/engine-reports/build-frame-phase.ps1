# Optional ENG14 manual public-call illustration; no Application phase measurements.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$EngineSourceDirectory,
    [Parameter(Mandatory=$true)][string]$EngineBuildDirectory,
    [Parameter(Mandatory=$true)][string]$DependencyDirectory,
    [Parameter(Mandatory=$true)][string]$RuntimeDirectory,
    [ValidateSet('Debug','Release')][string]$Configuration='Debug',
    [Parameter(Mandatory=$true)][string]$OutputDirectory
)
$ErrorActionPreference='Stop'
$framePhaseEngine=[IO.Path]::GetFullPath($EngineSourceDirectory)
$framePhaseBuild=[IO.Path]::GetFullPath($EngineBuildDirectory)
$framePhaseDependencies=[IO.Path]::GetFullPath($DependencyDirectory)
$framePhaseRuntime=[IO.Path]::GetFullPath($RuntimeDirectory)
$framePhaseOutput=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Path $framePhaseOutput -Force | Out-Null
$framePhaseSource=Join-Path $PSScriptRoot 'frame_phase_boundaries.cpp'
$framePhaseExe=Join-Path $framePhaseOutput 'frame_phase_boundaries.exe'
$framePhaseDependencyLib=Join-Path $framePhaseDependencies $(if ($Configuration -eq 'Debug') { 'debug/lib' } else { 'lib' })
$framePhaseArguments=@('/nologo','/EHsc','/std:c++20','/W4','/WX','/permissive-',
    '/DDIRECTX_TOOLKIT_IMPORT','/DUSING_XAUDIO2_REDIST','/DUSING_XINPUT','/DUNICODE','/D_UNICODE',
    '/external:W0',('/I'+$framePhaseEngine),
    ('/external:I'+(Join-Path $framePhaseEngine 'external/rapidjson/include')),
    ('/external:I'+(Join-Path $framePhaseDependencies 'include/directxtk')),
    ('/external:I'+(Join-Path $framePhaseDependencies 'include/xaudio2redist')),
    ('/external:I'+(Join-Path $framePhaseDependencies 'include')),
    ('/Fe:'+ $framePhaseExe),('/Fo:'+ (Join-Path $framePhaseOutput 'frame_phase_boundaries.obj')))
if ($Configuration -eq 'Debug') { $framePhaseArguments+=@('/MDd','/Od','/D_DEBUG') }
else { $framePhaseArguments+=@('/MD','/O2','/DNDEBUG') }
$framePhaseArguments+=@($framePhaseSource,
    (Join-Path $framePhaseBuild 'engine/LabradorEngine.lib'),
    (Join-Path $framePhaseBuild 'engine/math/MattMath.lib'),
    (Join-Path $framePhaseDependencyLib 'DirectXTK.lib'),
    (Join-Path $framePhaseDependencyLib 'xaudio2_9redist.lib'),
    'd3d11.lib','dxgi.lib','dxguid.lib','kernel32.lib','user32.lib','gdi32.lib',
    'winspool.lib','shell32.lib','ole32.lib','oleaut32.lib','uuid.lib','comdlg32.lib','advapi32.lib')
foreach ($framePhaseDll in @('DirectXTK.dll','xaudio2_9redist.dll')) {
    Copy-Item -LiteralPath (Join-Path $framePhaseRuntime $framePhaseDll) -Destination (Join-Path $framePhaseOutput $framePhaseDll)
}
$framePhaseArguments | Set-Content -LiteralPath (Join-Path $framePhaseOutput 'compile-arguments.txt')
& cl.exe @framePhaseArguments *> (Join-Path $framePhaseOutput 'compile.log')
if ($LASTEXITCODE -ne 0) { throw "Frame phase illustration compile failed; see $framePhaseOutput/compile.log" }
Write-Output $framePhaseExe
