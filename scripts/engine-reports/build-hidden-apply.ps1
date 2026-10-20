# Compile the minimal hidden-window Apply exhibit against prebuilt engine libraries.
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
$hiddenApplyEngine=[IO.Path]::GetFullPath($EngineSourceDirectory)
$hiddenApplyBuild=[IO.Path]::GetFullPath($EngineBuildDirectory)
$hiddenApplyDependencies=[IO.Path]::GetFullPath($DependencyDirectory)
$hiddenApplyRuntime=[IO.Path]::GetFullPath($RuntimeDirectory)
$hiddenApplyOutput=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Path $hiddenApplyOutput -Force | Out-Null
$hiddenApplySource=Join-Path $PSScriptRoot 'hidden_apply_exhibit.cpp'
$hiddenApplyExe=Join-Path $hiddenApplyOutput 'hidden_apply_exhibit.exe'
$hiddenApplyDependencyLib=Join-Path $hiddenApplyDependencies $(if ($Configuration -eq 'Debug') { 'debug/lib' } else { 'lib' })
$hiddenApplyArguments=@('/nologo','/EHsc','/std:c++20','/W4','/WX','/permissive-',
    '/DDIRECTX_TOOLKIT_IMPORT','/DUSING_XAUDIO2_REDIST','/DUSING_XINPUT','/DUNICODE','/D_UNICODE',
    '/external:W0',('/I'+$hiddenApplyEngine),
    ('/external:I'+(Join-Path $hiddenApplyEngine 'external/rapidjson/include')),
    ('/external:I'+(Join-Path $hiddenApplyDependencies 'include/directxtk')),
    ('/external:I'+(Join-Path $hiddenApplyDependencies 'include/xaudio2redist')),
    ('/external:I'+(Join-Path $hiddenApplyDependencies 'include')),
    ('/Fe:'+ $hiddenApplyExe),('/Fo:'+ (Join-Path $hiddenApplyOutput 'hidden_apply_exhibit.obj')))
if ($Configuration -eq 'Debug') { $hiddenApplyArguments+=@('/MDd','/Od','/D_DEBUG') }
else { $hiddenApplyArguments+=@('/MD','/O2','/DNDEBUG') }
$hiddenApplyArguments+=@($hiddenApplySource,
    (Join-Path $hiddenApplyBuild 'engine/LabradorEngine.lib'),
    (Join-Path $hiddenApplyBuild 'engine/math/MattMath.lib'),
    (Join-Path $hiddenApplyDependencyLib 'DirectXTK.lib'),
    (Join-Path $hiddenApplyDependencyLib 'xaudio2_9redist.lib'),
    'd3d11.lib','dxgi.lib','dxguid.lib','kernel32.lib','user32.lib','gdi32.lib',
    'winspool.lib','shell32.lib','ole32.lib','oleaut32.lib','uuid.lib','comdlg32.lib','advapi32.lib')
foreach ($hiddenApplyDll in @('DirectXTK.dll','xaudio2_9redist.dll')) {
    Copy-Item -LiteralPath (Join-Path $hiddenApplyRuntime $hiddenApplyDll) -Destination (Join-Path $hiddenApplyOutput $hiddenApplyDll)
}
$hiddenApplyArguments | Set-Content -LiteralPath (Join-Path $hiddenApplyOutput 'compile-arguments.txt')
& cl.exe @hiddenApplyArguments *> (Join-Path $hiddenApplyOutput 'compile.log')
if ($LASTEXITCODE -ne 0) { throw "Hidden Apply exhibit compile failed; see $hiddenApplyOutput/compile.log" }
Write-Output $hiddenApplyExe
