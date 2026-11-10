@{
    Schema = 1
    Name = 'hard-aim-reaction-ladder'
    Stages = @('crossfeed', 'close_quarters', 'press_tower', 'turbulence_slab_trial')
    SeedStart = 1000
    SeedCount = 16
    Duration = 60
    Difficulties = @('hard')
    Tools = @('sprayer', 'sniper', 'roller', 'mister', 'bucket')
    Rosters = @(@{ Kind = 'ordered_pairs' })
    # One factor changes at a time around the shipped Hard record. These
    # diagnostic probes never write tuning.json or propose accepted values.
    Variants = @(
        @{ Name = 'baseline'; Patch = @{} }
        @{ Name = 'aim-perfect'; Patch = @{ 'bots.hard.aim_error_radians' = 0 } }
        @{ Name = 'aim-wide'; Patch = @{ 'bots.hard.aim_error_radians' = 0.20 } }
        @{ Name = 'reaction-fast'; Patch = @{ 'bots.hard.reaction_steps' = 1 } }
        @{ Name = 'reaction-slow'; Patch = @{ 'bots.hard.reaction_steps' = 24 } }
    )
    ComparisonMargins = @{ paint = 0.03; kills = 0.20; deaths = 0.20 }
}
