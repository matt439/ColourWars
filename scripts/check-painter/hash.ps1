# Byte-array fingerprints for the painter's retained-evidence verifiers.
function Get-PainterSha256 {
    param([byte[]] $Bytes)
    $algorithm = [Security.Cryptography.SHA256]::Create()
    try {
        return [BitConverter]::ToString($algorithm.ComputeHash($Bytes)).Replace('-', '')
    }
    finally { $algorithm.Dispose() }
}
