#include "game/pch.h"
#include "scripts/acceptance/paint_surfaces/fixtures.h"
#if COLOURWARS_DEV_TOOLS
#include "game/debug/match_fingerprint.h"
#endif
#include <array>

namespace paint_surfaces_acceptance
{
    namespace
    {
        // Also used in Release, where MatchFingerprint's developer code is absent.
        std::vector<std::uint32_t> public_state(const Level& level)
        {
            std::vector<std::uint32_t> state{ static_cast<std::uint32_t>(level.state()),
                static_cast<std::uint32_t>(level.projectile_count()),
                static_cast<std::uint32_t>(level.collision_object_count()) };
            for (int view = 0; view < level.player_count(); ++view)
            {
                const Player& player = level.player(view);
                for (float value : { player.center().x, player.center().y, player.velocity().x, player.velocity().y,
                    player.health(), player.weapon_ammo(), player.respawn_timer() }) { state.push_back(bits(value)); }
                state.push_back(static_cast<std::uint32_t>(player.move_state()));
                state.push_back(static_cast<std::uint32_t>(player.state()));
            }
            const LevelEndInfo info = level.level_end_info();
            for (float value : { info.total_area, info.team_a_area, info.team_b_area }) { state.push_back(bits(value)); }
            for (float area : info.player_areas) { state.push_back(bits(area)); }
            return state;
        }
        void stable_snapshot(const Level& level, int updates)
        {
            const auto expected = sample_state(level.paint_surfaces());
            const auto before = public_state(level);
#if COLOURWARS_DEV_TOOLS
            const MatchFingerprint fingerprint = MatchFingerprint::of(level, updates);
#else
            (void)updates;
#endif
            for (int read = 0; read < 8; ++read)
            {
                const auto samples = level.paint_surfaces();
                require(sample_state(samples) == expected, "Level snapshot changed source/tile order or held values");
                for (const PaintSurfaceSample& sample : samples) { (void)static_rectangle(sample); }
                same_total(grouped_total(samples), from_end_info(level.level_end_info()));
            }
            require(public_state(level) == before,
                "Level snapshot changed world state, scoring or projectiles");
#if COLOURWARS_DEV_TOOLS
            const MatchFingerprint after = MatchFingerprint::of(level, updates);
            require(fingerprint.hash == after.hash && MatchFingerprint::difference(fingerprint, after).empty(),
                "Read-only Level snapshot changed the existing replay fingerprint");
#endif
        }
    }

    void check_levels(World& world)
    {
        const float dt = static_cast<float>(labrador::StepTimer::TicksToSeconds(labrador::StepTimer::TicksPerSecond / 60));
        unsigned int seed = 0;
        for (const char* stage : { "crossfeed", "press_tower" })
        {
            auto level = world.level(stage, seed);
            const auto initial = level->paint_surfaces();
            require(!initial.empty() && initial.back().source_index > initial.front().source_index,
                "Real stage snapshot did not collect multiple paintable roles");
            for (const PaintSurfaceSample& sample : initial)
            { require(sample.team == PlayerTeam::none && sample.paint_owner == -1, "New stage snapshot fabricated existing paint"); }
            const PaintTotal neutral = grouped_total(initial);
            require(neutral.total > 0.0f && neutral.team_a == 0.0f && neutral.team_b == 0.0f,
                "Real stage neutral coverage is invalid");
            same_total(neutral, from_end_info(level->level_end_info()));
            stable_snapshot(*level, 0);
            std::vector<PlayerInputData> inputs(static_cast<std::size_t>(input_slots::count));
            inputs[0].connected = true; inputs[1].connected = true;
            int updates = 0;
            while (!level->match_started() && updates < 1000) { level->update(inputs, dt); ++updates; }
            require(level->match_started(), "Real stage did not finish its bounded countdown");
            const std::array<float, 2> ammo{ level->player(0).weapon_ammo(), level->player(1).weapon_ammo() };
            for (std::size_t slot = 0; slot < ammo.size(); ++slot)
            {
                inputs[slot].shoot_direction_requested = true;
                inputs[slot].shoot_direction = mattmath::Vector2F::DIRECTION_DOWN;
                inputs[slot].shoot_angle = mattmath::PI / 2.0f;
                inputs[slot].primary_shoot = true;
            }
            for (int tick = 0; tick < 120; ++tick) { level->update(inputs, dt); ++updates; }
            const LevelEndInfo painted = level->level_end_info();
            require(painted.team_a_area > 0.0f && painted.team_b_area > 0.0f &&
                painted.player_areas[0] > 0.0f && painted.player_areas[1] > 0.0f &&
                level->player(0).weapon_ammo() < ammo[0] && level->player(1).weapon_ammo() < ammo[1],
                "Active stage control did not exercise actual paid fire and nontrivial paint ownership");
            require(bits(painted.total_area) == bits(neutral.total), "Real painting changed the arena denominator");
            same_total(grouped_total(level->paint_surfaces()), from_end_info(painted));
            // Probe RNG after simulation, so the fixture does not reseed active play.
            std::srand(2731u); const int expected_random = std::rand(); std::srand(2731u);
            stable_snapshot(*level, updates);
            require(std::rand() == expected_random, "Read-only Level snapshot advanced global RNG");
            seed = 0xffffffffu;
        }
    }
}
