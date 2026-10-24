# Compile the guarded Window-only exhibit. This script never launches it.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$EngineSourceDirectory,
    [Parameter(Mandatory=$true)][string]$EngineBuildDirectory,
    [Parameter(Mandatory=$true)][string]$OutputDirectory,
    [ValidateSet('Debug','Release')][string]$Configuration='Debug'
)
$ErrorActionPreference='Stop'
$resizeEngine=[IO.Path]::GetFullPath($EngineSourceDirectory)
$resizeBuild=[IO.Path]::GetFullPath($EngineBuildDirectory)
$resizeOutput=[IO.Path]::GetFullPath($OutputDirectory)
$resizeSource=Join-Path $PSScriptRoot 'hidden_resize_exhibit.cpp'
$resizeLibrary=Join-Path $resizeBuild 'engine/LabradorEngine.lib'
$resizeMath=Join-Path $resizeBuild 'engine/math/MattMath.lib'
foreach($resizeInput in @($resizeSource,$resizeLibrary,$resizeMath)){
    if(-not(Test-Path -LiteralPath $resizeInput -PathType Leaf)){throw "Missing input: $resizeInput"}
}
$null=Get-Command cl.exe -ErrorAction Stop
$null=New-Item -ItemType Directory -Path $resizeOutput -Force
$resizeExe=Join-Path $resizeOutput 'hidden_resize_exhibit.exe'
$resizeArguments=@('/nologo','/EHsc','/std:c++20','/W4','/WX','/permissive-',
    '/DUNICODE','/D_UNICODE','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN',
    ('/I'+$resizeEngine),('/Fe:'+$resizeExe),
    ('/Fo:'+(Join-Path $resizeOutput 'hidden_resize_exhibit.obj')),
    ('/Fd:'+(Join-Path $resizeOutput 'hidden_resize_exhibit.pdb')))
if($Configuration -eq 'Debug'){$resizeArguments+=@('/MDd','/Od','/D_DEBUG')}
else{$resizeArguments+=@('/MD','/O2','/DNDEBUG')}
$resizeArguments+=@($resizeSource,$resizeLibrary,$resizeMath,'user32.lib','gdi32.lib','kernel32.lib')
$resizeArguments|Set-Content -LiteralPath (Join-Path $resizeOutput 'compile-arguments.txt')
& cl.exe @resizeArguments *> (Join-Path $resizeOutput 'compile.log')
if($LASTEXITCODE -ne 0){throw "Exhibit compile failed; retain $resizeOutput/compile.log"}
Write-Output $resizeExe