# Compile the hidden DPI baseline against prebuilt engine libraries; embeds its own manifest.
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
$dpiBaselineEngine=[IO.Path]::GetFullPath($EngineSourceDirectory)
$dpiBaselineBuild=[IO.Path]::GetFullPath($EngineBuildDirectory)
$dpiBaselineDependencies=[IO.Path]::GetFullPath($DependencyDirectory)
$dpiBaselineRuntime=[IO.Path]::GetFullPath($RuntimeDirectory)
$dpiBaselineOutput=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Path $dpiBaselineOutput -Force | Out-Null
$dpiBaselineSource=Join-Path $PSScriptRoot 'dpi_window_baseline.cpp'
$dpiBaselineExe=Join-Path $dpiBaselineOutput 'dpi_window_baseline.exe'
$dpiBaselineDependencyLib=Join-Path $dpiBaselineDependencies $(if ($Configuration -eq 'Debug') { 'debug/lib' } else { 'lib' })
$dpiBaselineArguments=@('/nologo','/EHsc','/std:c++20','/W4','/WX','/permissive-',
    '/DDIRECTX_TOOLKIT_IMPORT','/DUSING_XAUDIO2_REDIST','/DUSING_XINPUT','/DUNICODE','/D_UNICODE',
    '/D_WIN32_WINNT=0x0A00','/DWINVER=0x0A00',
    '/external:W0',('/I'+$dpiBaselineEngine),
    ('/external:I'+(Join-Path $dpiBaselineEngine 'external/rapidjson/include')),
    ('/external:I'+(Join-Path $dpiBaselineDependencies 'include/directxtk')),
    ('/external:I'+(Join-Path $dpiBaselineDependencies 'include/xaudio2redist')),
    ('/external:I'+(Join-Path $dpiBaselineDependencies 'include')),
    ('/Fe:'+ $dpiBaselineExe),('/Fo:'+ (Join-Path $dpiBaselineOutput 'dpi_window_baseline.obj')))
if ($Configuration -eq 'Debug') { $dpiBaselineArguments+=@('/MDd','/Od','/D_DEBUG') }
else { $dpiBaselineArguments+=@('/MD','/O2','/DNDEBUG') }
$dpiBaselineArguments+=@($dpiBaselineSource,
    (Join-Path $dpiBaselineBuild 'engine/LabradorEngine.lib'),
    (Join-Path $dpiBaselineBuild 'engine/math/MattMath.lib'),
    (Join-Path $dpiBaselineDependencyLib 'DirectXTK.lib'),
    (Join-Path $dpiBaselineDependencyLib 'xaudio2_9redist.lib'),
    'd3d11.lib','dxgi.lib','dxguid.lib','kernel32.lib','user32.lib','gdi32.lib',
    'winspool.lib','shell32.lib','ole32.lib','oleaut32.lib','uuid.lib','comdlg32.lib','advapi32.lib')
foreach ($dpiBaselineDll in @('DirectXTK.dll','xaudio2_9redist.dll')) {
    Copy-Item -LiteralPath (Join-Path $dpiBaselineRuntime $dpiBaselineDll) -Destination (Join-Path $dpiBaselineOutput $dpiBaselineDll)
}
$dpiBaselineArguments+=@('/link','/MANIFEST:EMBED',('/MANIFESTINPUT:'+(Join-Path $PSScriptRoot 'dpi_window_baseline.manifest')))
$dpiBaselineArguments | Set-Content -LiteralPath (Join-Path $dpiBaselineOutput 'compile-arguments.txt')
& cl.exe @dpiBaselineArguments *> (Join-Path $dpiBaselineOutput 'compile.log')
if ($LASTEXITCODE -ne 0) { throw "DPI baseline compile failed; see $dpiBaselineOutput/compile.log" }
Write-Output $dpiBaselineExe
