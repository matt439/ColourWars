# Shared evidence contracts. JSON key sets, CSV column names, canonical PPM
# headers and Save log lines use ordinal case; numbers use the invariant culture.
function Assert-PainterPpm {
    param([string] $Path, [int] $Width, [int] $Height, $Header, [byte[]] $Bytes,
        [switch] $PassThru)
    if ($null -eq $Bytes) { $Bytes = [IO.File]::ReadAllBytes($Path) }
    if ($null -ne $Header) {
        $expected = if ($Header -is [byte[]]) { [Text.Encoding]::ASCII.GetString($Header) } else { [string]$Header }
        if ($expected -cnotmatch '^P6\n([1-9][0-9]*) ([1-9][0-9]*)\n255\n$') { throw 'Invalid expected painter PPM header.' }
        $Width = [int]$Matches[1]; $Height = [int]$Matches[2]
    }
    $prefix = [Text.Encoding]::ASCII.GetString($Bytes, 0, [Math]::Min(64, $Bytes.Length))
    if ($prefix -cnotmatch '^P6\n([1-9][0-9]*) ([1-9][0-9]*)\n255\n') { throw "Noncanonical painter PPM header: $Path" }
    $actualWidth = [int]$Matches[1]; $actualHeight = [int]$Matches[2]
    $headerLength = $Matches[0].Length
    if (($Width -gt 0 -and $actualWidth -ne $Width) -or ($Height -gt 0 -and $actualHeight -ne $Height) -or
        $Bytes.LongLength -ne $headerLength + [long]$actualWidth * $actualHeight * 3) {
        throw "Painter PPM dimensions or complete RGB payload differ: $Path"
    }
    if ($PassThru) { return [pscustomobject]@{ width=$actualWidth; height=$actualHeight; header_length=$headerLength; bytes=$Bytes } }
}

function ConvertTo-PainterFinite {
    param($Value, [string] $Label = 'metric')
    $number = 0.0
    if (-not [double]::TryParse([string]$Value, [Globalization.NumberStyles]::Float,
        [Globalization.CultureInfo]::InvariantCulture, [ref]$number) -or
        [double]::IsNaN($number) -or [double]::IsInfinity($number)) { throw "Invalid finite painter number: $Label" }
    return $number
}

function Assert-PainterKeys {
    param($Object, [string[]] $Expected, [string] $Scope)
    if (-not (Test-PainterKeys $Object $Expected)) { throw "Painter exact key set differs: $Scope" }
}

function Test-PainterKeys {
    param($Object, [string[]] $Expected)
    if ($Object -isnot [pscustomobject]) { return $false }
    [string[]] $actual = @($Object.PSObject.Properties.Name)
    [string[]] $wanted = @($Expected)
    if ($actual.Count -ne $wanted.Count) { return $false }
    [Array]::Sort($actual, [StringComparer]::Ordinal)
    [Array]::Sort($wanted, [StringComparer]::Ordinal)
    for ($index = 0; $index -lt $actual.Count; ++$index) {
        if (-not [string]::Equals($actual[$index], $wanted[$index], [StringComparison]::Ordinal)) { return $false }
    }
    return $true
}

function Assert-PainterCsvColumns {
    param([object[]] $Rows, [string[]] $Expected, [int] $Count = -1, [string] $Scope)
    if (($Count -ge 0 -and $Rows.Count -ne $Count) -or
        -not (Test-PainterCsvColumns $Rows $Expected)) { throw "Painter CSV shape differs: $Scope" }
}

function Test-PainterCsvColumns {
    param([object[]] $Rows, [string[]] $Expected)
    if ($Rows.Count -eq 0) { return $false }
    foreach ($row in $Rows) {
        [string[]] $actual = @($row.PSObject.Properties.Name)
        if ($actual.Count -ne $Expected.Count) { return $false }
        for ($index = 0; $index -lt $actual.Count; ++$index) {
            if (-not [string]::Equals($actual[$index], $Expected[$index], [StringComparison]::Ordinal)) { return $false }
        }
    }
    return $true
}

function Assert-PainterFlags {
    param($Row, [string[]] $Flags)
    foreach ($flag in $Flags) { if ($Row.$flag -cne 'true') { throw "Painter CSV flag differs: $flag" } }
}

function ConvertTo-PainterCsvInteger {
    param($Value, [string] $Label)
    $number = 0L
    if ([string]$Value -cnotmatch '^(0|[1-9][0-9]*)$' -or
        -not [long]::TryParse([string]$Value, [Globalization.NumberStyles]::None,
            [Globalization.CultureInfo]::InvariantCulture, [ref]$number)) { throw "Painter canonical CSV integer differs: $Label" }
    return $number
}

function ConvertTo-PainterInteger {
    param($Value, [string] $Label, [long] $Minimum = 0, [long] $Maximum = [int]::MaxValue)
    $number = 0L
    if (-not [long]::TryParse([string]$Value, [Globalization.NumberStyles]::Integer,
        [Globalization.CultureInfo]::InvariantCulture, [ref]$number) -or
        $number -lt $Minimum -or $number -gt $Maximum) { throw "Painter integer is invalid or out of bounds: $Label" }
    return $number
}

function Get-PainterCsvInteger {
    param($Row, [string] $Key)
    return ConvertTo-PainterCsvInteger $Row.$Key $Key
}

function Assert-PainterInteger {
    param($Object, [string] $Key, [long] $Expected)
    $value = $Object.$Key
    if (($value -isnot [int] -and $value -isnot [long]) -or $value -ne $Expected) { throw "Painter exact integer differs: $Key" }
}

function Get-PainterSaveCounts {
    param([string] $RunDirectory)
    $lines = @(Get-Content -LiteralPath (Join-Path $RunDirectory 'run.log'))
    return [pscustomobject]@{
        calls = @($lines | Where-Object { $_ -cmatch 'Saving\.\.\.$' }).Count
        successes = @($lines | Where-Object { $_ -ceq 'Save was successful.' }).Count
    }
}

function Get-PainterSavePath {
    param([string] $ProfileDirectory)
    return Join-Path $ProfileDirectory 'ColourWars/save_data.json'
}

# Return an owned bitmap; the caller disposes it after its page composition.
function ConvertFrom-PainterPpm {
    param([string] $Path)
    $ppm = Assert-PainterPpm -Path $Path -PassThru
    $width = $ppm.width
    $height = $ppm.height
    $bytes = $ppm.bytes
    $bitmap = [Drawing.Bitmap]::new($width, $height, [Drawing.Imaging.PixelFormat]::Format24bppRgb)
    try {
        $data = $bitmap.LockBits([Drawing.Rectangle]::new(0, 0, $width, $height),
            [Drawing.Imaging.ImageLockMode]::WriteOnly, [Drawing.Imaging.PixelFormat]::Format24bppRgb)
        try {
            $pixels = [byte[]]::new($data.Stride * $height)
            $source = $ppm.header_length
            for ($y = 0; $y -lt $height; ++$y) {
                for ($x = 0; $x -lt $width; ++$x) {
                    $destination = $y * $data.Stride + $x * 3
                    $pixels[$destination] = $bytes[$source + 2]
                    $pixels[$destination + 1] = $bytes[$source + 1]
                    $pixels[$destination + 2] = $bytes[$source]
                    $source += 3
                }
            }
            [Runtime.InteropServices.Marshal]::Copy($pixels, 0, $data.Scan0, $pixels.Length)
        }
        finally { $bitmap.UnlockBits($data) }
        $bitmap.Save([IO.Path]::ChangeExtension($Path, '.png'), [Drawing.Imaging.ImageFormat]::Png)
        return $bitmap
    }
    catch { $bitmap.Dispose(); throw }
}
