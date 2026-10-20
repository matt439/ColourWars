# Compile the asset-free hidden-creation positive control against prebuilt engine libraries.
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
$startupEngine=[IO.Path]::GetFullPath($EngineSourceDirectory)
$startupBuild=[IO.Path]::GetFullPath($EngineBuildDirectory)
$startupDependencies=[IO.Path]::GetFullPath($DependencyDirectory)
$startupRuntime=[IO.Path]::GetFullPath($RuntimeDirectory)
$startupOutput=[IO.Path]::GetFullPath($OutputDirectory)
$startupSource=Join-Path $PSScriptRoot 'startup_hidden_creation.cpp'
$startupSourceHash='E4CE532B903884BC2B6A06D3CF228554E042251D1FDCD93E7F2BBB9C48DC6A1E'
if ((Get-FileHash -LiteralPath $startupSource).Hash -ne $startupSourceHash) {
    throw 'Hidden-creation source differs from the frozen source audit.'
}
$startupDependencyLib=Join-Path $startupDependencies $(if ($Configuration -eq 'Debug') { 'debug/lib' } else { 'lib' })
$startupLibraries=@(
    (Join-Path $startupBuild 'engine/LabradorEngine.lib'),
    (Join-Path $startupBuild 'engine/math/MattMath.lib'),
    (Join-Path $startupDependencyLib 'DirectXTK.lib'),
    (Join-Path $startupDependencyLib 'xaudio2_9redist.lib'))
foreach ($startupInput in @($startupSource)+$startupLibraries+
    @((Join-Path $startupRuntime 'DirectXTK.dll'),(Join-Path $startupRuntime 'xaudio2_9redist.dll'))) {
    if (-not (Test-Path -LiteralPath $startupInput -PathType Leaf)) {
        throw "Missing hidden-creation build input: $startupInput"
    }
}
New-Item -ItemType Directory -Path $startupOutput -Force | Out-Null
$startupExe=Join-Path $startupOutput 'startup_hidden_creation.exe'
$startupArguments=@('/nologo','/EHsc','/std:c++20','/W4','/WX','/permissive-',
    '/DDIRECTX_TOOLKIT_IMPORT','/DUSING_XAUDIO2_REDIST','/DUSING_XINPUT','/DUNICODE','/D_UNICODE',
    '/external:W0',('/I'+$startupEngine),
    ('/external:I'+(Join-Path $startupEngine 'external/rapidjson/include')),
    ('/external:I'+(Join-Path $startupDependencies 'include/directxtk')),
    ('/external:I'+(Join-Path $startupDependencies 'include/xaudio2redist')),
    ('/external:I'+(Join-Path $startupDependencies 'include')),
    ('/Fe:'+ $startupExe),('/Fo:'+ (Join-Path $startupOutput 'startup_hidden_creation.obj')))
if ($Configuration -eq 'Debug') { $startupArguments+=@('/MDd','/Od','/D_DEBUG') }
else { $startupArguments+=@('/MD','/O2','/DNDEBUG') }
$startupArguments+=@($startupSource)+$startupLibraries+@(
    'd3d11.lib','dxgi.lib','dxguid.lib','kernel32.lib','user32.lib','gdi32.lib',
    'winspool.lib','shell32.lib','ole32.lib','oleaut32.lib','uuid.lib','comdlg32.lib','advapi32.lib')
foreach ($startupDll in @('DirectXTK.dll','xaudio2_9redist.dll')) {
    Copy-Item -LiteralPath (Join-Path $startupRuntime $startupDll) -Destination (Join-Path $startupOutput $startupDll)
}
$startupArguments | Set-Content -LiteralPath (Join-Path $startupOutput 'compile-arguments.txt')
& cl.exe @startupArguments *> (Join-Path $startupOutput 'compile.log')
if ($LASTEXITCODE -ne 0) { throw "Hidden-creation compile failed; see $startupOutput/compile.log" }
Write-Output $startupExe
