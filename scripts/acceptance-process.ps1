# Load once before opening the acceptance runspace pool. The probe wrapper is
# deliberately separate from compiler/CMake invocation and uses C# 5 syntax so
# the same source loads under Windows PowerShell 5.1 and PowerShell 7.
function Initialize-AcceptanceProcess {
    [CmdletBinding()]
    param()

    if ('ColourWars.Acceptance.OwnedCommand' -as [type]) { return }
    Add-Type -Path (Join-Path $PSScriptRoot 'acceptance-process/owned-command.cs') -ErrorAction Stop
}
