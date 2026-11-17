# Optional-bank fallback and Debug's adapter identity are ordinary diagnostics.
function Test-OptionalBankDiagnostic([string] $Line) {
    return $Line -cmatch '^no audio: .+\. The manifest marks this bank optional, so it plays nothing\. See the repository''s README, ''Audio''\.$'
}

function Test-DebugAdapterDiagnostic([string] $Line) {
    # Labrador v0.4 sends this existing informational trace to stderr instead
    # of OutputDebugString. Keep warnings and every other engine notice fatal.
    return $Line -cmatch '^Direct3D Adapter \((0|[1-9][0-9]*)\): VID:[0-9A-F]{4}, PID:[0-9A-F]{4} - [^\x00-\x1F\x7F]+$'
}

function Assert-CheckDiagnostics {
    param(
        [AllowEmptyCollection()][string[]] $Lines = @(),
        [string] $Context = 'probe',
        [switch] $RequireAudio,
        [switch] $RequireNoAudio,
        [switch] $AllowDebugAdapter,
        [string] $AllowedPattern
    )
    $fallback = $false
    foreach ($line in $Lines) {
        if (-not $line.Trim()) { continue }
        if (Test-OptionalBankDiagnostic $line) {
            if ($RequireAudio) { throw "The audible $Context check unexpectedly used a silent bank." }
            $fallback = $true
        }
        elseif ($AllowDebugAdapter -and (Test-DebugAdapterDiagnostic $line)) { continue }
        elseif (-not ($AllowedPattern -and $line -cmatch $AllowedPattern)) {
            throw "Unexpected ${Context} diagnostic: $line"
        }
    }
    if ($RequireNoAudio -and -not $fallback) { throw 'The expected optional-wave-bank fallback was not reported.' }
}
