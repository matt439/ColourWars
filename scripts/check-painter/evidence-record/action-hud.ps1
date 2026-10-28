# Dot-sourced by evidence-record.ps1 after its explicit precedence selection.
# Keep these bodies in caller scope: result key order and dynamic inputs matter.
param([string] $PainterActionHudScope)

switch -Exact ($PainterActionHudScope) {
    'IdentityOnly' {
        $result.player_counts = @(2, 4)
        $result.weapons = @('sprayer', 'sniper')
        $result.actions = @('identity-ink-live-consumers', 'identity-background-live-consumers', 'independent-badge-paper-substitution', 'retained-black-glyphs-and-borders', 'unchanged-scene-pixels-outside-badges', 'hidden-and-focus-policy', 'frozen-geometry-state-rng-and-debug-fingerprint')
        foreach ($key in 'contact_checks', 'weapon_checks', 'quiet_checks', 'aim_marker_checks', 'blocked_shot_checks', 'monitor_checks', 'countdown_checks', 'closing_checks') {
            $result.Remove($key)
        }
        $result.checks = $result.player_identity_checks
        $result.checks += ' The additional background-colour companion preserves the existing ink proof and uses preconstructed PlayerBadge/UiPlayerBadge roster cells plus the frozen four-view world/HUD badges. Default WHITE, named RED, RGB35BCE1, opaque RGBA2060C0FF and exact restoration require28 additional readbacks,five consumer CSV rows,four separate identity-background crops and zero state updates per preset. Literal baseline WHITE paper is substituted only inside independently computed badge rectangles, with nonzero paper and unchanged BLACK glyph/border pixels in every direct/UI/world/HUD roster. Exact full-frame references preserve all unrelated scene white and every pixel outside those bounds; captured geometry, enabled/hidden/focus policy, public simulation/cosmetic state,RNG and Debug fingerprint remain fixed. Team/Weapon/Results are source-audited users of the shared badge renderer; their pages are not newly opened. No human contrast/readability, physical-device,listening or performance acceptance is claimed.'
        if ($Preset -eq 'x64-debug') {
            $result.actions += @('identity-background-real-tune-reset-save', 'identity-background-atomic-semantic-refusals', 'identity-background-one-token-save-rebase', 'preserved-private-play-and-jump-preferences')
            $result.checks += ' Debug background commands separately require40 additional readbacks,ten attempts(seven accepted,three semantic refusals),eight tune CSV rows and zero additional crops/state updates. Translucent,zero-alpha and current-ink-equal backgrounds must leave live colour/text/ledger/private bytes unchanged. Real named/RGB/opaque-RGBA edits, targeted reset, one-token private save/rebase and saved-baseline reset operate on the same preconstructed consumers; actual content and unrelated private play and Jump preferences remain unchanged. These are the source-enforced background budgets in addition to the preserved ink evidence.'
        }
    }
    'QuietOnly' {
        $result.weapons = @('sprayer', 'sniper', 'bucket')
        $result.actions = @()
        foreach ($key in 'contact_checks', 'weapon_checks', 'aim_marker_checks', 'blocked_shot_checks', 'player_identity_checks', 'monitor_checks', 'countdown_checks', 'closing_checks') {
            $result.Remove($key)
        }
        $result.checks = $result.quiet_checks
    }
    'MonitorOnly' {
        $result.player_counts = @(1, 2, 4)
        $result.palettes = @('blue_orange')
        $result.weapons = @('roller')
        $result.actions = @()
        foreach ($key in 'contact_checks', 'weapon_checks', 'quiet_checks', 'aim_marker_checks', 'blocked_shot_checks', 'player_identity_checks', 'countdown_checks', 'closing_checks') {
            $result.Remove($key)
        }
        $result.checks = $result.monitor_checks
    }
    'CountdownOnly' {
        $result.player_counts = @(1, 2, 4)
        $result.palettes = @('blue_orange')
        $result.weapons = @('roller')
        $result.actions = @()
        $result.input_slots = @(4, 3, 1, 0)
        $result.inputs = 'Scripted keyboard/mouse seat and sparse pad-slot inputs; no physical-device claim.'
        foreach ($key in 'contact_checks', 'weapon_checks', 'quiet_checks', 'aim_marker_checks', 'blocked_shot_checks', 'player_identity_checks', 'monitor_checks', 'closing_checks') {
            $result.Remove($key)
        }
        $result.checks = $result.countdown_checks
    }
    'ClosingOnly' {
        $result.player_counts = @(1, 2, 4)
        $result.weapons = @('roller')
        $result.actions = @()
        foreach ($key in 'contact_checks', 'weapon_checks', 'quiet_checks', 'aim_marker_checks', 'blocked_shot_checks', 'player_identity_checks', 'monitor_checks', 'countdown_checks') {
            $result.Remove($key)
        }
        $result.checks = $result.closing_checks
    }
    'SupplyOnly' {
        $result.weapons = @('sprayer', 'mister')
        $result.actions = @()
        foreach ($key in 'contact_checks', 'weapon_checks', 'quiet_checks', 'aim_marker_checks', 'blocked_shot_checks', 'player_identity_checks', 'monitor_checks', 'countdown_checks', 'closing_checks') {
            $result.Remove($key)
        }
        $result.checks = $result.supply_checks
    }
    'ResumeOnly' {
        $result.player_counts = @(1, 2, 4)
        $result.palettes = @('runtime-save-default')
        $result.weapons = @('sprayer')
        $result.actions = @('pause', 'resume', 'jump', 'fire', 'drop', 'death', 'focus', 'restart', 'quit')
        $result.input_slots = @(4, -1)
        $result.inputs = 'Public keyboard/mouse message input through actual GameLevel, StateContext and pause pages; other seats are idle dummies, with idle bots in Debug. No physical pad claim.'
        foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
        $result.checks = 'Actual 1P/2P/4P pause-page Resume freezes the world through count-in, nonpositive time, re-pause, focus loss/return and a long GO-crossing update. Public player health/ammo/cooldown/dead respawn, live projectile count, detailed cosmetic observation and underlying world/HUD raster remain fixed. Held jump/fire/drop remain suppressed until neutral and a deliberate new request; fresh pause interrupts the count. Actual keyboard-driven Restart clears the gate and Quit leaves immediately. Shared RESUMING paper is bounded and frozen at native 720p/1080p/QHD with exact resize round-trip. Muted hardware rendering; no human readability or listening acceptance.'
    }
    'HudOnly' {
        $result.player_counts = @(1, 2, 3, 4)
        $result.weapons = @('sprayer', 'sniper', 'mister', 'bucket')
        $result.actions = @()
        $result.input_slots = @(4, 3, 1, 0)
        $result.inputs = 'Direct HUD quantities and sparse keyboard/pad-slot labels; one real four-player builder-owned frozen match. No physical-device claim.'
        foreach ($key in 'contact_checks', 'weapon_checks', 'quiet_checks', 'aim_marker_checks', 'blocked_shot_checks', 'player_identity_checks', 'monitor_checks', 'countdown_checks', 'closing_checks', 'supply_checks') {
            $result.Remove($key)
        }
        $result.checks = $result.hud_checks
    }
    default { throw "Unknown painter action-hud evidence scope: $PainterActionHudScope" }
}
