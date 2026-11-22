# Optional-bank fallback and Debug's adapter identity are ordinary diagnostics.
function Test-OptionalBankDiagnostic([string] $Line) {
    return $Line -cmatch '^no audio: .+\. The manifest marks this bank optional, so it plays nothing\. See the repository''s README, ''Audio''\.$'
}

function Test-DebugAdapterDiagnostic([string] $Line) {
    # Labrador v0.4 sends this existing informational trace to stderr instead
    # of OutputDebugString. Keep warnings and every other engine notice fatal.
    return $Line -ceq 'Direct3D Adapter - WARP' -or
        $Line -cmatch '^Direct3D Adapter \((0|[1-9][0-9]*)\): VID:[0-9A-F]{4}, PID:[0-9A-F]{4} - [^\x00-\x1F\x7F]+$'
}

function Test-SoftwareTestDeviceDiagnostic([string] $Line) {
    return $Line -cmatch '^Direct3D 11 software-test device: [^\x00-\x1F\x7F]+ \(software\)$'
}

function Test-SoftwareTestDebugDiagnostic([string] $Line) {
    return $Line -ceq 'Direct3D Adapter - WARP (explicit software test)'
}

function Test-RenderDeviceRefusal([string] $Record, [switch] $AllowLegacyReason) {
    if ($Record -cnotmatch '(?m)^report_format=1\r?$' -or
        $Record -cnotmatch '(?m)^category=startup\r?$' -or
        $Record -cnotmatch '(?m)^phase=application_initialization\r?$') { return $false }
    return $Record -cmatch '(?m)^cause_[0-9]+_reason=render_device_unavailable\r?$' -or
        ($AllowLegacyReason -and $Record -cmatch '(?m)^cause_[0-9]+_reason=no_hardware_device\r?$')
}

function Assert-CheckDiagnostics {
    param(
        [AllowEmptyCollection()][string[]] $Lines = @(),
        [string] $Context = 'probe',
        [switch] $RequireAudio,
        [switch] $RequireNoAudio,
        [switch] $AllowDebugAdapter,
        [string] $AllowedPattern,
        [switch] $AllowSoftwareTestAdapter,
        [switch] $RequireSoftwareTestAdapter
    )
    $fallback = $false
    $software = $false
    $allowSoftware = $AllowSoftwareTestAdapter -or $RequireSoftwareTestAdapter
    foreach ($line in $Lines) {
        if (-not $line.Trim()) { continue }
        if (Test-OptionalBankDiagnostic $line) {
            if ($RequireAudio) { throw "The audible $Context check unexpectedly used a silent bank." }
            $fallback = $true
        }
        elseif ($AllowDebugAdapter -and (Test-DebugAdapterDiagnostic $line)) { continue }
        elseif ($allowSoftware -and (Test-SoftwareTestDeviceDiagnostic $line)) { $software = $true }
        elseif ($allowSoftware -and $AllowDebugAdapter -and (Test-SoftwareTestDebugDiagnostic $line)) {
            continue
        }
        elseif (-not ($AllowedPattern -and $line -cmatch $AllowedPattern)) {
            throw "Unexpected ${Context} diagnostic: $line"
        }
    }
    if ($RequireNoAudio -and -not $fallback) { throw 'The expected optional-wave-bank fallback was not reported.' }
    if ($RequireSoftwareTestAdapter -and -not $software) { throw 'The D3D11 software-test device was not reported.' }
}
