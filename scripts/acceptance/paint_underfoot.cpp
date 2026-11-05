#include "game/pch.h"
#include "scripts/acceptance/paint_underfoot/fixtures.h"
#include "scripts/acceptance/support/probe.h"
#include <iostream>

int wmain(int argc, wchar_t* argv[])
{
    return acceptance::run_probe("paint underfoot", [&]
    {
        if (argc != 2) { throw std::invalid_argument("paint_underfoot <content>"); }
        paint_underfoot_acceptance::World world(std::filesystem::absolute(argv[1]));
        // POL#30, accepted 2026-11-05: the mechanic ships on at the POL#36
        // trial amounts, with the health lockout off.
        const PaintUnderfootTuning& shipped = world.tuning.player().paint_underfoot;
        paint_underfoot_acceptance::require(shipped.enabled && shipped.own_speed_multiplier == 1.10f &&
            shipped.own_ammo_regen_multiplier == 1.25f && shipped.enemy_speed_multiplier == 0.85f &&
            shipped.enemy_ammo_regen_multiplier == 0.0f && !shipped.enemy_blocks_health_regen,
            "Paint underfoot must ship enabled at the accepted trial amounts with the health lockout off");
        paint_underfoot_acceptance::check_movement(world);
        paint_underfoot_acceptance::check_support(world);
        paint_underfoot_acceptance::check_recovery(world);
        paint_underfoot_acceptance::check_schema_and_bounds(world);
        paint_underfoot_acceptance::check_health_regen(world);
        std::cout << "PASS paint underfoot: actual contact/movement/recovery/schema/bounds; no human balance acceptance\n";
        return 0;
    });
}
