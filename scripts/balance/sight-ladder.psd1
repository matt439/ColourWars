@{
    Schema = 1
    Name = 'hard-sight-ladder'
    Stages = @('crossfeed', 'close_quarters', 'press_tower', 'turbulence_slab_trial')
    SeedStart = 1000
    SeedCount = 16
    Duration = 60
    Difficulties = @('hard')
    Tools = @('sprayer', 'sniper', 'roller', 'mister', 'bucket')
    Rosters = @(@{ Kind = 'ordered_pairs' })
    # REV#19: bots.sight_distance alone, around the shipped 1600. The 1v1
    # runs also print REV#37's shooter-pane hit-view lines into each job's
    # run.log (scripts/balance/hit_view.py). Evidence only: no value here
    # is ever written to tuning.json.
    Variants = @(
        @{ Name = 'baseline'; Patch = @{} }
        @{ Name = 'sight-1000'; Patch = @{ 'bots.sight_distance' = 1000.0 } }
        @{ Name = 'sight-600'; Patch = @{ 'bots.sight_distance' = 600.0 } }
    )
    ComparisonMargins = @{ paint = 0.03; kills = 0.20; deaths = 0.20 }
}
