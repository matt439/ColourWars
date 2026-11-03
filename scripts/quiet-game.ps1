# Check runs are silent. The game, and the probes that open a real audio device,
# hold every sound at zero while COLOURWARS_MUTE is set to anything but "0"
# (game/config/sound_groups.h). A check that starts one dot-sources this file,
# calls Enter-QuietGame before its first launch and Exit-QuietGame in a finally,
# so its children inherit the switch and the caller's environment is left as it
# was found. Set COLOURWARS_MUTE=0 before a run to hear it.
function Enter-QuietGame {
    $previous = [Environment]::GetEnvironmentVariable('COLOURWARS_MUTE', 'Process')
    if ($null -eq $previous) {
        [Environment]::SetEnvironmentVariable('COLOURWARS_MUTE', '1', 'Process')
    }
    return $previous
}

function Exit-QuietGame([AllowNull()][string] $Previous) {
    if ([string]::IsNullOrEmpty($Previous)) { $Previous = $null }
    [Environment]::SetEnvironmentVariable('COLOURWARS_MUTE', $Previous, 'Process')
}
