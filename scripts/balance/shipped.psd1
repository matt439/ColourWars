@{
    Schema = 1
    Name = 'shipped-20261110'
    Stages = @('crossfeed', 'close_quarters', 'press_tower', 'turbulence_slab_trial')
    SeedStart = 1000
    SeedCount = 16
    Duration = 60
    Difficulties = @('normal', 'hard')
    Tools = @('sprayer', 'sniper', 'roller', 'mister', 'bucket')
    Rosters = @(
        @{ Kind = 'ordered_pairs' }
        @{ Kind = 'tool_with_painter'; Painters = @('roller', 'mister') }
    )
    Variants = @(@{ Name = 'baseline'; Patch = @{} })
    ComparisonMargins = @{ paint = 0.03; kills = 0.20; deaths = 0.20 }
}
