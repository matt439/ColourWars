# Compile the standalone exhibit using existing engine libraries; does not launch it.
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
$systemKeysEngine=[IO.Path]::GetFullPath($EngineSourceDirectory)
$systemKeysBuild=[IO.Path]::GetFullPath($EngineBuildDirectory)
$systemKeysDependencies=[IO.Path]::GetFullPath($DependencyDirectory)
$systemKeysRuntime=[IO.Path]::GetFullPath($RuntimeDirectory)
$systemKeysOutput=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Path $systemKeysOutput -Force | Out-Null
$systemKeysSource=Join-Path $PSScriptRoot 'system_keys_exhibit.cpp'
$systemKeysExe=Join-Path $systemKeysOutput 'system_keys_exhibit.exe'
$systemKeysDependencyLib=Join-Path $systemKeysDependencies $(if ($Configuration -eq 'Debug') { 'debug/lib' } else { 'lib' })
$systemKeysArguments=@('/nologo','/EHsc','/std:c++20','/W4','/WX','/permissive-',
    '/DDIRECTX_TOOLKIT_IMPORT','/DUSING_XAUDIO2_REDIST','/DUSING_XINPUT','/DUNICODE','/D_UNICODE',
    '/external:W0',('/I'+$systemKeysEngine),
    ('/external:I'+(Join-Path $systemKeysEngine 'external/rapidjson/include')),
    ('/external:I'+(Join-Path $systemKeysDependencies 'include/directxtk')),
    ('/external:I'+(Join-Path $systemKeysDependencies 'include/xaudio2redist')),
    ('/external:I'+(Join-Path $systemKeysDependencies 'include')),
    ('/Fe:'+ $systemKeysExe),('/Fo:'+ (Join-Path $systemKeysOutput 'system_keys_exhibit.obj')))
if ($Configuration -eq 'Debug') { $systemKeysArguments+=@('/MDd','/Od','/D_DEBUG') }
else { $systemKeysArguments+=@('/MD','/O2','/DNDEBUG') }
$systemKeysArguments+=@($systemKeysSource,
    (Join-Path $systemKeysBuild 'engine/LabradorEngine.lib'),
    (Join-Path $systemKeysBuild 'engine/math/MattMath.lib'),
    (Join-Path $systemKeysDependencyLib 'DirectXTK.lib'),
    (Join-Path $systemKeysDependencyLib 'xaudio2_9redist.lib'),
    'd3d11.lib','dxgi.lib','dxguid.lib','kernel32.lib','user32.lib','gdi32.lib',
    'winspool.lib','shell32.lib','ole32.lib','oleaut32.lib','uuid.lib','comdlg32.lib','advapi32.lib')
foreach ($systemKeysDll in @('DirectXTK.dll','xaudio2_9redist.dll')) {
    Copy-Item -LiteralPath (Join-Path $systemKeysRuntime $systemKeysDll) -Destination (Join-Path $systemKeysOutput $systemKeysDll)
}
$systemKeysArguments | Set-Content -LiteralPath (Join-Path $systemKeysOutput 'compile-arguments.txt')
& cl.exe @systemKeysArguments *> (Join-Path $systemKeysOutput 'compile.log')
if ($LASTEXITCODE -ne 0) { throw "System-key exhibit compile failed; see $systemKeysOutput/compile.log" }
Write-Output $systemKeysExe
