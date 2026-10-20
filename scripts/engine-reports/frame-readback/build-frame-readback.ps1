# Compile the asset-free frame-readback positive control against prebuilt engine libraries.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$EngineSourceDirectory,
    [Parameter(Mandatory=$true)][string]$EngineBuildDirectory,
    [Parameter(Mandatory=$true)][string]$DependencyDirectory,
    [Parameter(Mandatory=$true)][string]$RuntimeDirectory,
    [ValidateSet('Debug','Release')][string]$Configuration='Debug',
    [Parameter(Mandatory=$true)][string]$OutputDirectory
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$frameEngine=[IO.Path]::GetFullPath($EngineSourceDirectory)
$frameBuild=[IO.Path]::GetFullPath($EngineBuildDirectory)
$frameDependencies=[IO.Path]::GetFullPath($DependencyDirectory)
$frameRuntime=[IO.Path]::GetFullPath($RuntimeDirectory)
$frameOutput=[IO.Path]::GetFullPath($OutputDirectory)
$frameSource=Join-Path $PSScriptRoot 'frame_readback.cpp'
$frameSourceHash='5FA71174A19F9DF75871BBC4A22951088C75B60C9626237028DBA0C6ED2E33EF'
if ((Get-FileHash -LiteralPath $frameSource).Hash -ne $frameSourceHash) {
    throw 'Frame-readback source differs from the frozen source audit.'
}
$frameDependencyLib=Join-Path $frameDependencies $(if ($Configuration -eq 'Debug') { 'debug/lib' } else { 'lib' })
$frameLibraries=@(
    (Join-Path $frameBuild 'engine/LabradorEngine.lib'),
    (Join-Path $frameBuild 'engine/math/MattMath.lib'),
    (Join-Path $frameDependencyLib 'DirectXTK.lib'),
    (Join-Path $frameDependencyLib 'xaudio2_9redist.lib'))
foreach ($frameInput in @($frameSource)+$frameLibraries+
    @((Join-Path $frameRuntime 'DirectXTK.dll'),(Join-Path $frameRuntime 'xaudio2_9redist.dll'))) {
    if (-not (Test-Path -LiteralPath $frameInput -PathType Leaf)) {
        throw "Missing frame-readback build input: $frameInput"
    }
}
New-Item -ItemType Directory -Path $frameOutput -Force | Out-Null
$frameExe=Join-Path $frameOutput 'frame_readback.exe'
$frameArguments=@('/nologo','/EHsc','/std:c++20','/W4','/WX','/permissive-',
    '/DDIRECTX_TOOLKIT_IMPORT','/DUSING_XAUDIO2_REDIST','/DUSING_XINPUT','/DUNICODE','/D_UNICODE',
    '/external:W0',('/I'+$frameEngine),
    ('/external:I'+(Join-Path $frameEngine 'external/rapidjson/include')),
    ('/external:I'+(Join-Path $frameDependencies 'include/directxtk')),
    ('/external:I'+(Join-Path $frameDependencies 'include/xaudio2redist')),
    ('/external:I'+(Join-Path $frameDependencies 'include')),
    ('/Fe:'+ $frameExe),('/Fo:'+ (Join-Path $frameOutput 'frame_readback.obj')))
if ($Configuration -eq 'Debug') { $frameArguments+=@('/MDd','/Od','/D_DEBUG') }
else { $frameArguments+=@('/MD','/O2','/DNDEBUG') }
$frameArguments+=@($frameSource)+$frameLibraries+@(
    'd3d11.lib','dxgi.lib','dxguid.lib','kernel32.lib','user32.lib','gdi32.lib',
    'winspool.lib','shell32.lib','ole32.lib','oleaut32.lib','uuid.lib','comdlg32.lib','advapi32.lib')
foreach ($frameDll in @('DirectXTK.dll','xaudio2_9redist.dll')) {
    Copy-Item -LiteralPath (Join-Path $frameRuntime $frameDll) -Destination (Join-Path $frameOutput $frameDll)
}
$frameArguments | Set-Content -LiteralPath (Join-Path $frameOutput 'compile-arguments.txt')
& cl.exe @frameArguments *> (Join-Path $frameOutput 'compile.log')
if ($LASTEXITCODE -ne 0) { throw "Frame-readback compile failed; see $frameOutput/compile.log" }
Write-Output $frameExe
