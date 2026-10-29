# Explicit check paths follow PowerShell's caller location, which can differ
# from the process directory. Resolve without expanding wildcards or requiring
# a not-yet-created output leaf to exist.
function Resolve-CheckFileSystemPath([string] $Path) {
    if ([string]::IsNullOrEmpty($Path)) { throw 'A check path must not be empty.' }
    $providerInfo = $null
    $driveInfo = $null
    $resolved = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath(
        $Path, [ref]$providerInfo, [ref]$driveInfo)
    if ($null -eq $providerInfo -or $providerInfo.Name -ne 'FileSystem') {
        throw 'A check path must use the FileSystem provider.'
    }
    return [IO.Path]::GetFullPath($resolved)
}
