#include "game/pch.h"
#include "scripts/acceptance/paint_surfaces/fixtures.h"
#include "game/debug/dev_tools.h"
#if COLOURWARS_DEV_TOOLS
#include "game/debug/match_fingerprint.h"
#endif
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>

namespace
{
    using namespace labrador;
    using namespace mattmath;
    using paint_surfaces_acceptance::require;

    constexpr int trial_steps = 600;
    constexpr unsigned int trial_seed = 41731;
    const float dt = static_cast<float>(StepTimer::TicksToSeconds(StepTimer::SecondsToTicks(1.0 / 60.0)));

    class World
    {
    public:
        explicit World(const std::filesystem::path& content) : assets(content)
        {
            for (const char* stage : { "crossfeed", "close_quarters", "king_of_the_hill", "turbulence" })
            {
                auto info = std::make_unique<LevelLoadedInfo>(path_to_utf8(content / "levels" /
                    path_from_utf8(std::string(stage) + ".json")).c_str());
                info->validate(); levels.add(stage, std::move(info));
            }
        }
        std::unique_ptr<Level> build(const MenuLevelSettings& settings)
        {
            std::srand(trial_seed);
            viewports.set_layout(settings.screen_layout());
            LevelBuilder builder(&viewports, &assets.render, &assets.audio, &assets.preferences,
                &levels, &assets.tuning, &assets.presentation, &resolution, nullptr, &partitioner, &mouse);
            return builder.build_level(settings, TeamColourPair::blue_orange, OpeningMode::off);
        }
        paint_surfaces_acceptance::World assets;
        Registry<LevelLoadedInfo> levels{ "bot-trials" };
        ResolutionManager resolution;
        ViewportManager viewports{ &resolution };
        Mouse mouse;
        Partitioner partitioner;
    };

    struct Trial
    {
        const char* stage;
        WeaponType tool;
        PlayerTeam team;
        BotDifficulty difficulty;
        bool local_combat = false;
    };

    MenuLevelSettings roster(const Trial& trial)
    {
        MenuLevelSettings settings;
        settings.set_player_count(2);
        settings.set_player_bot(1, trial.difficulty);
        settings.begin_input_claims(input_slots::keyboard_and_mouse);
        settings.set_screen_layout(ScreenLayout::one_player);
        settings.set_stage(trial.stage); settings.set_duration(20.0f);
        settings.set_player_team(0, trial.team == PlayerTeam::a ? PlayerTeam::b : PlayerTeam::a);
        settings.set_player_team(1, trial.team);
        settings.set_player_weapon(0, trial.tool); settings.set_player_weapon(1, trial.tool);
        return settings;
    }

    std::vector<PlayerInputData> physical_inputs()
    {
        std::vector<PlayerInputData> physical(input_slots::count);
        for (PlayerInputData& input : physical) { input.connected = true; }
        return physical;
    }

    // Initial local-combat placement is an explicitly separate capability
    // trial. Natural-spawn trials never inject positions or use cheats.
    void prepare(Level& level, const Trial& trial)
    {
        const std::vector<PlayerInputData> physical = physical_inputs();
        for (int tick = 0; tick < 900 && level.state() != LevelState::active; ++tick)
        { level.update(physical, dt); }
        require(level.state() == LevelState::active, "Trial did not finish the ordinary countdown");
        if (trial.local_combat)
        {
            const float width = level.player(1).rectangle().width;
            const float height = level.player(1).rectangle().height;
            const bool left = trial.team == PlayerTeam::a;
            const float bot_x = left ? 300.0f : 3600.0f - 300.0f - width;
            const float human_x = left ? 1000.0f : 3600.0f - 1000.0f - width;
            require(level.place_player(1, Vector2F(bot_x, 1280.0f - height)) &&
                level.place_player(0, Vector2F(human_x, 1280.0f - height)), "Local trial placement failed");
            require(level.bot_line_of_sight(level.player(1).center(), level.player(0).center()),
                "Local trial initial opponent is not visible");
        }
    }

    // The public final-input seam records after the shipping driver. Playback
    // tells Level to bypass that driver, in Debug and Release alike.
    class Tape final : public SeatInputFilter
    {
    public:
        void filter_seats(std::vector<PlayerInputData>& seats, const Level&) override
        {
            if (playback)
            {
                require(next < steps.size(), "Final-input playback exhausted its finite tape");
                seats = steps[next++];
            }
            else { steps.push_back(seats); }
        }
        bool replaces_bot_inputs() const override { return playback; }
        std::vector<std::vector<PlayerInputData>> steps;
        bool playback = false;
        std::size_t next = 0;
    };

    struct Metrics
    {
        Vector2F initial;
        float min_x = 0, max_x = 0, min_y = 0, max_y = 0;
        float path = 0, progress = 0, minimum_ammo = 1;
        float damage = 0, minimum_separation = 0, last_separation = 0;
        float final_paint = 0;
        int painted_sources = 0, painted_tiles = 0;
        int movement_steps = 0, blocked_steps = 0, jumps = 0, drops = 0, fire_steps = 0, firing_steps = 0;
        int empty_steps = 0, low_steps = 0, recovered = 0, first_damage = -1;
        int deaths = 0;
        std::uint64_t kills = 0;
    };

    std::vector<std::uint64_t> snapshot(const Level& level)
    {
        std::vector<std::uint64_t> state = paint_surfaces_acceptance::sample_state(level.paint_surfaces());
        for (int seat = 0; seat < level.player_count(); ++seat)
        {
            const Player& player = level.player(seat);
            for (const float value : { player.center().x, player.center().y, player.velocity().x,
                player.velocity().y, player.weapon_ammo(), player.health() })
            { state.push_back(paint_surfaces_acceptance::bits(value)); }
            state.push_back(static_cast<std::uint64_t>(player.state()));
            state.push_back(static_cast<std::uint64_t>(player.move_state()));
            for (const std::uint64_t count : player.lethal_projectiles().enemy_by_owner) { state.push_back(count); }
            state.push_back(player.lethal_projectiles().uncredited);
        }
        state.push_back(static_cast<std::uint64_t>(level.projectile_count()));
        return state;
    }

    Metrics run(World& world, const Trial& trial, bool replay_check)
    {
        const MenuLevelSettings settings = roster(trial);
        std::unique_ptr<Level> level = world.build(settings);
        prepare(*level, trial);
        Metrics metrics;
        metrics.initial = level->player(1).center();
        metrics.min_x = metrics.max_x = metrics.initial.x;
        metrics.min_y = metrics.max_y = metrics.initial.y;
        metrics.minimum_separation = Vector2F::distance(level->player(0).center(), metrics.initial);
        float previous_health = level->player(0).health();
        Vector2F previous = metrics.initial;
        PlayerState previous_state = level->player(1).state();
        bool was_low = false;
        const std::vector<PlayerInputData> physical = physical_inputs();
        Tape tape;
        for (int tick = 0; tick < trial_steps; ++tick)
        {
            level->update(physical, dt, &tape);
            require(level->state() == LevelState::active && tape.steps.size() == static_cast<std::size_t>(tick + 1),
                "Trial ran outside its finite active-step budget");
            require(tape.steps.back().size() == 2, "Final-input trial lost a roster participant");
            const PlayerInputData& input = tape.steps.back()[1];
            require(input.connected && !input.toggle_pause_menu && !input.pause_held && !input.toggle_debug,
                "Trial bot emitted a menu/pause/debug action");
            const Player& bot = level->player(1);
            require(bot.primary_weapon().type() == trial.tool && bot.player_num() == -1,
                "Trial changed the bot tool or assigned a physical device");
            const Vector2F current = bot.center();
            require(std::isfinite(current.x) && std::isfinite(current.y) && std::isfinite(bot.weapon_ammo()) &&
                bot.weapon_ammo() >= 0.0f, "Trial produced invalid movement or reserves");
            metrics.min_x = std::min(metrics.min_x, current.x); metrics.max_x = std::max(metrics.max_x, current.x);
            metrics.min_y = std::min(metrics.min_y, current.y); metrics.max_y = std::max(metrics.max_y, current.y);
            const float displacement = Vector2F::distance(current, previous);
            if (bot.state() == PlayerState::alive && previous_state == PlayerState::alive)
            { metrics.path += displacement; }
            const float forward = (current.x - metrics.initial.x) * (trial.team == PlayerTeam::a ? 1.0f : -1.0f);
            metrics.progress = std::max(metrics.progress, forward);
            metrics.movement_steps += input.x_movement != 0.0f ? 1 : 0;
            metrics.blocked_steps += input.x_movement != 0.0f && displacement < 0.1f ? 1 : 0;
            metrics.jumps += input.jump_pressed ? 1 : 0; metrics.drops += input.drop_through ? 1 : 0;
            metrics.fire_steps += input.primary_shoot ? 1 : 0;
            metrics.firing_steps += bot.primary_weapon().shooting_this_update() ? 1 : 0;
            const float ammo = bot.weapon_ammo() / world.assets.tuning.starting_ammo();
            metrics.minimum_ammo = std::min(metrics.minimum_ammo, ammo);
            const bool empty = bot.primary_weapon().readiness() == WeaponReadiness::empty;
            metrics.empty_steps += empty ? 1 : 0;
            metrics.low_steps += ammo <= world.assets.tuning.bots().reserve_low ? 1 : 0;
            if (empty || ammo <= world.assets.tuning.bots().reserve_low) { was_low = true; }
            if (was_low && ammo >= world.assets.tuning.bots().reserve_resume) { ++metrics.recovered; was_low = false; }
            const float health = level->player(0).health();
            if (health < previous_health)
            {
                metrics.damage += previous_health - health;
                if (metrics.first_damage < 0) { metrics.first_damage = tick; }
            }
            metrics.last_separation = Vector2F::distance(level->player(0).center(), current);
            metrics.minimum_separation = std::min(metrics.minimum_separation, metrics.last_separation);
            metrics.deaths += previous_state == PlayerState::alive && bot.state() == PlayerState::dead ? 1 : 0;
            previous = current; previous_health = health; previous_state = bot.state();
        }
        std::set<std::size_t> sources;
        const int owner = paint_owners::index(-1, 1);
        metrics.kills = level->player(0).lethal_projectiles().enemy_by_owner.at(static_cast<std::size_t>(owner));
        for (const PaintSurfaceSample& surface : level->paint_surfaces())
        {
            if (surface.paint_owner == owner)
            {
                metrics.final_paint += surface.scored_area; ++metrics.painted_tiles;
                sources.insert(surface.source_index);
            }
        }
        metrics.painted_sources = static_cast<int>(sources.size());
        if (replay_check)
        {
            const std::vector<std::uint64_t> expected = snapshot(*level);
#if COLOURWARS_DEV_TOOLS
            const MatchFingerprint expected_full = MatchFingerprint::of(*level, trial_steps);
#endif
            // Destroy before reconstructing because both scenes borrow the same partitioner.
            level.reset();
            level = world.build(settings); prepare(*level, trial);
            tape.playback = true;
            for (int tick = 0; tick < trial_steps; ++tick) { level->update(physical, dt, &tape); }
            require(tape.next == trial_steps && snapshot(*level) == expected,
                "Trial final-input replay changed actors, projectiles or exact paint state");
#if COLOURWARS_DEV_TOOLS
            require(MatchFingerprint::difference(expected_full, MatchFingerprint::of(*level, trial_steps)).empty(),
                "Trial final-input replay changed the complete Debug fingerprint");
#endif
        }
        return metrics;
    }

    const char* name(WeaponType tool)
    {
        constexpr const char* names[] = { "sprayer", "sniper", "roller", "mister", "bucket" };
        return names[static_cast<int>(tool)];
    }
    const char* name(BotDifficulty difficulty)
    {
        constexpr const char* names[] = { "easy", "normal", "hard" };
        return names[static_cast<int>(difficulty)];
    }

    void write(std::ostream& csv, const Trial& trial, const Metrics& m)
    {
        csv << (trial.local_combat ? "local_combat" : "spawn") << ',' << trial.stage << ',' << name(trial.tool) << ',' <<
            (trial.team == PlayerTeam::a ? "a" : "b") << ',' << name(trial.difficulty) << ',' << trial_seed << ',' <<
            trial_steps << ',' << m.initial.x << ',' << m.initial.y << ',' << m.max_x - m.min_x << ',' <<
            m.max_y - m.min_y << ',' << m.progress << ',' << m.path << ',' << m.movement_steps << ',' <<
            m.blocked_steps << ',' << m.jumps << ',' << m.drops << ',' << m.fire_steps << ',' << m.firing_steps << ',' <<
            m.minimum_ammo << ',' << m.empty_steps << ',' << m.low_steps << ',' << m.recovered << ',' << m.final_paint << ',' <<
            m.painted_sources << ',' << m.painted_tiles << ',' << m.damage << ',' << m.first_damage << ',' <<
            m.minimum_separation << ',' << m.last_separation << ',' << m.deaths << ',' << m.kills << '\n';
    }
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "usage: bot_trials <content>");
        World world(std::filesystem::absolute(argv[1]));
        const std::filesystem::path report = std::filesystem::absolute(argv[0]).parent_path() / "bot_trials.csv";
        std::ofstream csv(report);
        require(csv.good(), "Could not open the bounded bot-trial report");
        csv << std::fixed << std::setprecision(3);
        csv << "mode,stage,tool,team,difficulty,seed,steps,start_x,start_y,x_extent,y_extent,forward_progress,path,";
        csv << "movement_steps,blocked_steps,jumps,drop_steps,fire_steps,firing_steps,min_ammo,empty_steps,low_steps,";
        csv << "reserve_recoveries,final_paint,painted_sources,painted_tiles,opponent_damage,first_damage_step,";
        csv << "min_separation,last_separation,bot_deaths,bot_kills\n";
        int trials = 0;
        for (const char* stage : { "crossfeed", "close_quarters", "king_of_the_hill", "turbulence" })
        {
            int painted = 0;
            for (int tool = 0; tool < 5; ++tool)
            {
                for (const PlayerTeam team : { PlayerTeam::a, PlayerTeam::b })
                {
                    for (const BotDifficulty difficulty : { BotDifficulty::easy, BotDifficulty::normal, BotDifficulty::hard })
                    {
                        const Trial trial{ stage, static_cast<WeaponType>(tool), team, difficulty };
                        const bool replay = team == PlayerTeam::a &&
                            ((std::string_view(stage) == "crossfeed" && tool == 0 && difficulty == BotDifficulty::hard) ||
                            (std::string_view(stage) == "close_quarters" && tool == 2 && difficulty == BotDifficulty::normal));
                        const Metrics metrics = run(world, trial, replay);
                        write(csv, trial, metrics); ++trials; painted += metrics.final_paint > 0 ? 1 : 0;
                        if (std::string_view(stage) == "close_quarters")
                        {
                            csv.flush();
                            require(metrics.firing_steps > 0 && metrics.final_paint > 0.0f,
                                "CQ natural-spawn bot did not pay for shots and hold paint within 600 steps");
                        }
                    }
                }
            }
            csv.flush();
            std::cout << "MEASURE natural spawn " << stage << ": " << painted << "/30 trials hold paint after " <<
                trial_steps << " steps; all five tools, both teams, three difficulties\n";
        }
        for (int tool = 0; tool < 5; ++tool)
        {
            for (const PlayerTeam team : { PlayerTeam::a, PlayerTeam::b })
            {
                const Trial trial{ "crossfeed", static_cast<WeaponType>(tool), team, BotDifficulty::hard, true };
                const Metrics metrics = run(world, trial, tool == 4 && team == PlayerTeam::a);
                write(csv, trial, metrics); ++trials;
                require(metrics.damage > 0.0f && metrics.firing_steps > 0,
                    "Local combat trial did not pay for shots and damage its visible opponent within 600 steps");
                std::cout << "MEASURE local combat " << name(trial.tool) << '/' << (team == PlayerTeam::a ? 'a' : 'b') <<
                    ": damage=" << metrics.damage << " first=" << metrics.first_damage << " closest=" <<
                    metrics.minimum_separation << " last=" << metrics.last_separation << " firing_steps=" << metrics.firing_steps <<
                    " kills=" << metrics.kills << '\n';
            }
        }
        csv.flush(); require(csv.good(), "Could not complete the bounded bot-trial report");
        require(trials == 130, "Bot trial matrix did not cover its finite contract");
        std::cout << "PASS " << trials << " bounded actual-driver trials, CQ paid-paint gates and three 600-step final-input replays; report=" <<
            path_to_utf8(report) << "; measurements do not establish arena/difficulty quality\n";
        return 0;
    }
    catch (const std::exception& error) { std::cerr << "FAIL bot trials: " << error.what() << '\n'; return 1; }
}
