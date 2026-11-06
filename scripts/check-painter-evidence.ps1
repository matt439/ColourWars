# CPU-only qualification of the shared painter evidence contracts.
[CmdletBinding()]
param([string] $OutputDirectory)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot 'check-path.ps1')
. (Join-Path $PSScriptRoot 'check-painter/evidence.ps1')
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $PSScriptRoot '../out/validation/painter-evidence' }
$scratch = Join-Path (Resolve-CheckFileSystemPath $OutputDirectory) ([Guid]::NewGuid().ToString('N'))
$null = New-Item -ItemType Directory -Path $scratch -Force
function Require-Evidence([bool] $Condition, [string] $Message) { if (-not $Condition) { throw $Message } }
function Reject-Evidence([scriptblock] $Action) {
    $rejected = $false
    try { & $Action } catch { $rejected = $true }
    Require-Evidence $rejected 'Malformed painter evidence was accepted.'
}
$path = Join-Path $scratch 'sample.ppm'
$header = [Text.Encoding]::ASCII.GetBytes("P6`n2 1`n255`n")
$bytes = [byte[]]@($header + [byte[]]@(255,0,0,0,255,128))
[IO.File]::WriteAllBytes($path, $bytes)
Assert-PainterPpm -Path $path -Width 2 -Height 1
Reject-Evidence { Assert-PainterPpm -Bytes $bytes[0..($bytes.Length-2)] -Width 2 -Height 1 }
Reject-Evidence { Assert-PainterPpm -Bytes ([byte[]]@($bytes + 0)) -Width 2 -Height 1 }
Reject-Evidence { Assert-PainterPpm -Bytes $bytes -Width 1 -Height 2 }
$lowercase = [byte[]]$bytes.Clone(); $lowercase[0] = [byte][char]'p'
Reject-Evidence { Assert-PainterPpm -Bytes $lowercase }
Reject-Evidence { Assert-PainterPpm -Bytes ([Text.Encoding]::ASCII.GetBytes("P6`r`n2 1`r`n255`r`n123456")) }
$priorCulture = [Globalization.CultureInfo]::CurrentCulture
try {
    [Globalization.CultureInfo]::CurrentCulture = [Globalization.CultureInfo]::GetCultureInfo('de-DE')
    Require-Evidence ((ConvertTo-PainterFinite '1.25' 'fixture') -eq 1.25) 'Painter metric followed caller culture.'
    foreach ($invalid in @('NaN','Infinity','1,25','junk','')) { Reject-Evidence { ConvertTo-PainterFinite $invalid } }
}
finally { [Globalization.CultureInfo]::CurrentCulture = $priorCulture }
Assert-PainterKeys ([pscustomobject]@{alpha=1;beta=2}) @('beta','alpha') 'fixture'
Reject-Evidence { Assert-PainterKeys ([pscustomobject]@{Alpha=1;beta=2}) @('alpha','beta') 'fixture' }
Reject-Evidence { Assert-PainterKeys ([pscustomobject]@{alpha=1}) @('alpha','beta') 'fixture' }
Reject-Evidence { Assert-PainterKeys ([pscustomobject]@{'alpha|beta'=1}) @('alpha','beta') 'fixture' }
$rows = @([pscustomobject]@{state='baseline';ok='true'})
Assert-PainterCsvColumns $rows @('state','ok') 1 'fixture'
Assert-PainterFlags $rows[0] @('ok')
Reject-Evidence { Assert-PainterCsvColumns $rows @('ok','state') 1 'fixture' }
Reject-Evidence { Assert-PainterCsvColumns @([pscustomobject]@{'state|ok'='x'}) @('state','ok') 1 'fixture' }
Reject-Evidence { Assert-PainterCsvColumns @($rows[0], [pscustomobject]@{State='baseline';ok='true'}) @('state','ok') 2 'fixture' }
Reject-Evidence { Assert-PainterFlags ([pscustomobject]@{ok='True'}) @('ok') }
Require-Evidence ((ConvertTo-PainterCsvInteger '12') -eq 12) 'Painter canonical count changed.'
foreach ($invalid in @('-1','01','1.0','+1','9223372036854775808')) { Reject-Evidence { ConvertTo-PainterCsvInteger $invalid } }
Assert-PainterInteger ([pscustomobject]@{count=2}) count 2
Reject-Evidence { Assert-PainterInteger ([pscustomobject]@{count=2.0}) count 2 }
[IO.File]::WriteAllLines((Join-Path $scratch 'run.log'), @('Saving...','Save was successful.','saving...','Save was successful. extra'))
$counts = Get-PainterSaveCounts $scratch
Require-Evidence ($counts.calls -eq 1 -and $counts.successes -eq 1) 'Save log counts changed their exact case or boundaries.'
Require-Evidence ((Get-PainterSavePath $scratch) -eq (Join-Path $scratch 'ColourWars/save_data.json')) 'Painter private save path changed.'
Add-Type -AssemblyName System.Drawing
$bitmap = ConvertFrom-PainterPpm $path
try {
    Require-Evidence ($bitmap.Width -eq 2 -and $bitmap.Height -eq 1 -and
        $bitmap.GetPixel(0,0).ToArgb() -eq [Drawing.Color]::FromArgb(255,0,0).ToArgb() -and
        $bitmap.GetPixel(1,0).ToArgb() -eq [Drawing.Color]::FromArgb(0,255,128).ToArgb()) 'PPM conversion changed RGB order or native dimensions.'
}
finally { $bitmap.Dispose() }
$decoded = [Drawing.Bitmap]::new([IO.Path]::ChangeExtension($path, '.png'))
try { Require-Evidence ($decoded.GetPixel(1,0).G -eq 255 -and $decoded.GetPixel(1,0).B -eq 128) 'Saved PNG changed native RGB.' }
finally { $decoded.Dispose() }
Write-Host "PASS painter evidence contracts; scratch: $scratch"
