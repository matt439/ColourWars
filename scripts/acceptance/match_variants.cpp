#include "game/pch.h"
#include "scripts/acceptance/paint_surfaces/fixtures.h"
#include "game/contracts/match_variant.h"
#include "game/contracts/simulation.h"
#include "game/match/round_tape.h"
#include "scripts/acceptance/support/json_fixture.h"
#include "scripts/acceptance/support/probe.h"
#include <iostream>

namespace
{
    using namespace labrador;
    using acceptance::require;
    constexpr float dt = simulation::seconds_per_step;

    MatchSetup setup(MatchVariant variant, WeaponType tool, bool bot = true)
    {
        MatchSetup settings;
        settings.set_variant(variant);
        settings.set_player_count(2);
        if (bot) { settings.set_player_bot(1, BotDifficulty::normal); }
        settings.begin_input_claims(0);
        if (!bot) { require(settings.claim_input_slot(1), "Cannot claim the second seat"); }
        settings.set_screen_layout(layout_for_humans(settings.human_count()));
        settings.set_stage("crossfeed");
        settings.set_duration(2.0f);
        settings.set_player_team(0, PlayerTeam::a);
        settings.set_player_team(1, PlayerTeam::b);
        settings.set_player_weapon(0, tool);
        settings.set_player_weapon(1, tool);
        return settings;
    }

    class World final
    {
    public:
        explicit World(const std::filesystem::path& content) : assets(content)
        {
            auto info = std::make_unique<LevelLoadedInfo>(
                path_to_utf8(content / "levels/crossfeed.json").c_str());
            info->validate();
            levels.add("crossfeed", std::move(info));
        }

        std::vector<std::unique_ptr<Player>> players(const MatchSetup& settings)
        {
            viewports.set_layout(settings.screen_layout());
            PlayerBuilder builder(&assets.tuning, &assets.presentation);
            return builder.build_players(settings, levels.get(settings.stage()),
                TeamColourTools::team_colours(TeamColourPair::blue_orange, assets.presentation.team_palette()),
                &assets.render, &assets.audio, &assets.preferences, &viewports);
        }

        std::unique_ptr<Level> level(const MatchSetup& settings)
        {
            viewports.set_layout(settings.screen_layout());
            LevelBuilder builder(&viewports, &assets.render, &assets.audio, &assets.preferences,
                &levels, &assets.tuning, &assets.presentation, &resolution, nullptr, &partitioner);
            return builder.build_level(settings, TeamColourPair::blue_orange, OpeningMode::off,
                MatchRandom(2207));
        }

        paint_surfaces_acceptance::World assets;
        Registry<LevelLoadedInfo> levels{ "match-variants" };
        ResolutionManager resolution;
        ViewportManager viewports{ &resolution };
        Partitioner partitioner;
    };

    PlayerInputData firing()
    {
        PlayerInputData input;
        input.connected = true;
        input.primary_shoot = true;
        input.shoot_direction_requested = true;
        input.shoot_direction = Vector2F::direction_down;
        input.shoot_angle = pi / 2.0f;
        return input;
    }

    void check_tools(World& world)
    {
        for_each_tuned_weapon([&](WeaponType tool, const char* name)
        {
            auto standard = world.players(setup(MatchVariant::standard, tool));
            auto endless = world.players(setup(MatchVariant::endless_paint, tool));
            const float full = world.assets.tuning.starting_ammo();
            const WeaponDetails& details = world.assets.tuning.weapon(tool).details;
            for (size_t seat = 0; seat < endless.size(); ++seat)
            {
                standard[seat]->set_player_input(firing());
                endless[seat]->set_player_input(firing());
                bool standard_spent = false;
                int volleys = 0, last_volley = -1;
                for (int tick = 0; tick < 2400; ++tick)
                {
                    // Restoring normal Debug cheats must leave the match option intact.
                    endless[seat]->set_infinite_ammo(false);
                    const auto limited = standard[seat]->update_weapon_and_get_projectiles(dt, {});
                    const auto unlimited = endless[seat]->update_weapon_and_get_projectiles(dt, {});
                    standard_spent = standard_spent || standard[seat]->weapon_ammo() < full;
                    require(endless[seat]->weapon_ammo() == full,
                        "Endless paint spent the human's or bot's tank");
                    if (!unlimited.empty())
                    {
                        if (last_volley >= 0)
                        {
                            require(static_cast<float>(tick - last_volley) * dt + 0.0001f >= details.shoot_interval,
                                "Endless paint bypassed a tool's cooldown");
                        }
                        last_volley = tick;
                        ++volleys;
                    }
                    // The initial full tank affords the first two shots of every
                    // shipped tool, independently of the endless supply policy.
                    if (volleys <= 2)
                    { require(limited.size() == unlimited.size(), "The variant changed initial firing cadence"); }
                }
                require(standard_spent && static_cast<float>(volleys) * details.ammo_usage > full * 2.0f,
                    "Tool hold did not distinguish normal supply from multiple endless tanks");
                PlayerInputData released = firing();
                released.primary_shoot = false;
                endless[seat]->set_player_input(released);
                require(endless[seat]->update_weapon_and_get_projectiles(dt, {}).empty(),
                    "Endless paint fired with the trigger released");
                endless[seat]->kill();
                endless[seat]->set_player_input(firing());
                require(endless[seat]->update_weapon_and_get_projectiles(dt, {}).empty(),
                    "Endless paint let an eliminated player fire");
                endless[seat]->force_respawn();
                require(endless[seat]->weapon_ammo() == full, "Endless paint respawn lost full supply");
            }
            std::cout << "PASS " << name << ": human/bot full tanks, cooldown, release and elimination\n";
        });
    }

    void check_expensive_shots(World& world, const std::filesystem::path& content)
    {
        acceptance::JsonVariants variants(content / "tuning.json", L"endless-paint");
        for (const float cost : { 0.6f, 2.0f })
        {
            const auto path = variants.write([&](rapidjson::Document& document)
            {
                for_each_tuned_weapon([&](WeaponType, const char* name)
                { document["weapons"][name]["ammo_usage"].SetFloat(cost); });
            });
            const GameTuning tuning(path_to_utf8(path).c_str());
            for_each_tuned_weapon([&](WeaponType tool, const char*)
            {
                auto weapon = WeaponBuilder::build_weapon(&tuning, PlayerTeam::a, 0, 0, Colour::white,
                    tool, Vector2F::zero, &world.assets.render, &world.assets.audio, &world.assets.preferences);
                weapon->set_endless_paint(true);
                int volleys = 0;
                for (int tick = 0; tick < 180; ++tick)
                {
                    const auto shots = weapon->update_and_get_projectiles(firing(), Vector2F::zero,
                        Vector2F::zero, true, dt);
                    volleys += shots.empty() ? 0 : 1;
                    require(weapon->ammo() == tuning.starting_ammo() && !weapon->low_paint_warned() &&
                        !weapon->low_paint() && !weapon->take_rumble_events().dry_fire,
                        "An expensive endless shot spent supply or emitted depletion feedback");
                }
                require(volleys >= 2, "A valid shot costing more than the tank could not fire with endless paint");
            });
        }
        std::cout << "PASS costly valid tuning: endless shots ignore tank cost without low/dry feedback\n";
    }

    void check_round(World& world)
    {
        const MatchSetup settings = setup(MatchVariant::endless_paint, WeaponType::sprayer, false);
        auto level = world.level(settings);
        level->start_without_countdown();
        RoundTape tape;
        tape.start(2207, settings);
        RoundTapeRecorder recorder(&tape);
        std::vector<PlayerInputData> devices(input_slots::count);
        devices[0] = firing();
        devices[1].connected = true;
        int ticks = 0;
        while (level->state() != LevelState::zoom_out && ticks < 180)
        {
            recorder.prepare(nullptr, dt);
            level->update(devices, dt, &recorder);
            ++ticks;
        }
        require(level->state() == LevelState::zoom_out && ticks >= 119 && ticks <= 122,
            "Endless paint changed the timed match end condition");
        const LevelEndInfo result = level->level_end_info();
        require(result.team_a_area > 0.0f && result.winning_team() == PlayerTeam::a,
            "Endless paint did not use ordinary surface coverage scoring");
        require(tape.playable() && tape.settings().variant() == MatchVariant::endless_paint &&
            !tape.infinite_ammo(), "The shipping tape lost the variant or turned it into a Debug cheat");
        // Exact end-of-round replay uses its copied setup with cheats restored off.
        auto replay = world.level(tape.settings());
        replay->set_player_cheats(false, false);
        replay->start_without_countdown();
        RoundTapePlayer player(&tape, 0);
        while (!player.finished()) { replay->update(devices, dt, &player); }
        const LevelEndInfo shown = replay->level_end_info();
        require(shown.team_a_area == result.team_a_area && shown.team_b_area == result.team_b_area &&
            shown.player_areas == result.player_areas && replay->player(0).weapon_ammo() == world.assets.tuning.starting_ammo(),
            "Endless paint shipping replay changed paint credit or restored finite supply");
        const auto rebuilt = world.level(settings);
        require(rebuilt->player(0).weapon_ammo() == world.assets.tuning.starting_ammo(),
            "Endless paint rebuild did not start with full supply");
        std::cout << "PASS timed coverage result and shipping tape replay with normal Debug cheats\n";
    }
}

int wmain(int argc, wchar_t* argv[])
{
    return acceptance::run_probe("match variants", [&]
    {
        require(argc == 2, "match_variants <content>");
        const std::filesystem::path content = std::filesystem::absolute(argv[1]);
        World world(content);
        check_tools(world);
        check_expensive_shots(world, content);
        check_round(world);
        return 0;
    });
}
