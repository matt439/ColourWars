# The optional silent bank is the only ordinary startup/probe diagnostic.
function Test-OptionalBankDiagnostic([string] $Line) {
    return $Line -cmatch '^no audio: .+\. The manifest marks this bank optional, so it plays nothing\. See the repository''s README, ''Audio''\.$'
}

function Assert-CheckDiagnostics {
    param(
        [AllowEmptyCollection()][string[]] $Lines = @(),
        [string] $Context = 'probe',
        [switch] $RequireAudio,
        [switch] $RequireNoAudio,
        [string] $AllowedPattern
    )
    $fallback = $false
    foreach ($line in $Lines) {
        if (-not $line.Trim()) { continue }
        if (Test-OptionalBankDiagnostic $line) {
            if ($RequireAudio) { throw "The audible $Context check unexpectedly used a silent bank." }
            $fallback = $true
        }
        elseif (-not ($AllowedPattern -and $line -cmatch $AllowedPattern)) {
            throw "Unexpected ${Context} diagnostic: $line"
        }
    }
    if ($RequireNoAudio -and -not $fallback) { throw 'The expected optional-wave-bank fallback was not reported.' }
}
