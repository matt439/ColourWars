#include "game/pch.h"
#include "scripts/acceptance/bot_navigation/fixtures.h"
#include "game/debug/dev_tools.h"
#include "scripts/acceptance/support/probe.h"
#if COLOURWARS_DEV_TOOLS
#include "game/debug/match_fingerprint.h"
#endif
#include <iostream>
#include <map>
#include <sstream>

// POL#42's shipping-bot route case on the Turbulence Yard blockout, built on
// the bot_turbulence_routes pattern. Three starts - the ordinary home spawn,
// the bot's own upper approach and the centre of the lower crossing - each
// measured against the authored chains that leave from there: the lower
// route and the upper departure from home, the fixed crossover from the
// upper approach, and the return home (and back to height) from the centre.
// Chains are ordered named grounded contacts. A chain the bot does not
// finish is recorded with its stop point; only bounded, valid driver output
// and the final-input replays are gated.
namespace
{
    using namespace labrador;
    using namespace mattmath;
    using namespace bot_navigation_acceptance;

    constexpr int trial_steps = 3600;

    enum class Start { home, upper, centre };
    const char* start_name(Start start)
    {
        switch (start)
        {
        case Start::home: return "home";
        case Start::upper: return "upper";
        case Start::centre: return "centre";
        }
        throw std::runtime_error("Unknown blockout route start");
    }

    struct Trial
    {
        Start start;
        PlayerTeam team;
        WeaponType tool;
        BotDifficulty difficulty;
    };

    struct Contact
    {
        RectangleF support;
        const char* name;
    };

    struct Chain
    {
        const char* name;
        std::vector<Contact> contacts;
    };

    // Named from the bot's own side. The yard contacts are ground windows
    // inside the authored approach bands (600..1100 and its mirror).
    std::vector<Chain> chains(const World& world, const Trial& trial)
    {
        const bool left = trial.team == PlayerTeam::a;
        const char own = left ? 'a' : 'b', other = left ? 'b' : 'a';
        const auto n = [&](const char* stem, char letter) -> Contact
        {
            const std::string key = std::string(stem) + letter;
            return { world.named(key), stem };
        };
        const RectangleF& ground = world.named("ground");
        const Contact own_home{ world.named(std::string("home_floor_") + own), "own_home" };
        const Contact own_yard{ RectangleF(left ? 600.0f : 2900.0f, ground.top(), 500.0f, ground.height), "own_yard" };
        const Contact far_yard{ RectangleF(left ? 2900.0f : 600.0f, ground.top(), 500.0f, ground.height), "far_yard" };
        Contact own_bed = n("lower_bed_", own); own_bed.name = "own_bed";
        Contact far_bed = n("lower_bed_", other); far_bed.name = "far_bed";
        Contact side_step = n("side_step_", own); side_step.name = "side_step";
        Contact own_upper = n("upper_approach_", own); own_upper.name = "own_upper";
        Contact far_upper = n("upper_approach_", other); far_upper.name = "far_upper";
        Contact own_crossover = n("crossover_", own); own_crossover.name = "own_crossover";
        Contact far_crossover = n("crossover_", other); far_crossover.name = "far_crossover";
        switch (trial.start)
        {
        case Start::home:
            return { { "lower_route", { own_home, own_yard, own_bed, far_bed, far_yard } },
                { "upper_departure", { own_home, side_step, own_upper, own_crossover, far_crossover } } };
        case Start::upper:
            return { { "fixed_crossover", { own_upper, own_crossover, far_crossover, far_upper } } };
        case Start::centre:
            return { { "return_home", { own_yard, own_home } },
                { "return_to_height", { own_yard, side_step, own_upper } } };
        }
        throw std::runtime_error("Unknown blockout route start");
    }

    MatchSetup roster(const Trial& trial)
    {
        MatchSetup settings;
        settings.set_player_count(2);
        settings.set_player_bot(1, trial.difficulty);
        settings.begin_input_claims(input_slots::keyboard_and_mouse);
        settings.set_screen_layout(ScreenLayout::one_player);
        settings.set_stage("turbulence_blockout"); settings.set_duration(120.0f);
        settings.set_player_team(0, trial.team == PlayerTeam::a ? PlayerTeam::b : PlayerTeam::a);
        settings.set_player_team(1, trial.team);
        settings.set_player_weapon(0, trial.tool); settings.set_player_weapon(1, trial.tool);
        return settings;
    }

    // Injection ends at initial placement; the home start keeps the ordinary
    // spawn. No route, destination, input, paint or enemy override follows.
    void prepare(Level& level, const World& world, const Trial& trial)
    {
        const std::vector<PlayerInputData> physical = physical_inputs();
        for (int tick = 0; tick < 900 && level.state() != LevelState::active; ++tick)
        { level.update(physical, dt); }
        require(level.state() == LevelState::active, "Blockout-route trial did not finish its ordinary countdown");
        if (trial.start == Start::home) { return; }
        const bool left = trial.team == PlayerTeam::a;
        const RectangleF support = trial.start == Start::upper ?
            world.named(left ? "upper_approach_a" : "upper_approach_b") : world.named("ground");
        const float x = trial.start == Start::upper ? (left ? 1060.0f : 2940.0f) : 2000.0f;
        const Vector2F size = world.assets.tuning.player().size;
        require(level.place_player(1, Vector2F(x - size.x * 0.5f, support.top() - size.y)),
            "Blockout-route trial could not place its initial endpoint");
        require(std::fabs(level.player(1).rectangle().bottom() - support.top()) <= endpoint_tolerance,
            "Blockout-route initial feet are not at the named support height");
    }

    struct ChainResult
    {
        std::vector<int> steps;   // First in-order arrival per contact; -1 if never.
        std::size_t next = 0;
        bool completed() const { return this->next == this->steps.size(); }
    };

    struct TrialResult
    {
        std::vector<ChainResult> chains;
        int jumps = 0, deaths = 0;
        float owned_paint = 0.0f;
    };

    TrialResult run(World& world, const Trial& trial, bool replay_check)
    {
        const MatchSetup settings = roster(trial);
        std::unique_ptr<Level> level = world.build(settings);
        prepare(*level, world, trial);
        const std::vector<Chain> route = chains(world, trial);
        TrialResult result;
        for (const Chain& chain : route) { result.chains.push_back({ std::vector<int>(chain.contacts.size(), -1), 0 }); }
        const std::vector<PlayerInputData> physical = physical_inputs();
        bool was_alive = level->player(1).state() == PlayerState::alive;
        Tape tape("Blockout-route playback exhausted its finite final-input tape");
        for (int tick = 0; tick < trial_steps; ++tick)
        {
            level->update(physical, dt, &tape);
            require(level->state() == LevelState::active && tape.steps.size() == static_cast<std::size_t>(tick + 1),
                "Blockout-route trial escaped its finite active-step budget");
            require(tape.steps.back().size() == 2, "Blockout-route trial lost a roster participant");
            const PlayerInputData& input = tape.steps.back()[1];
            require(input.connected && !input.toggle_pause_menu && !input.pause_held && !input.toggle_debug,
                "Blockout-route bot emitted a device/menu/pause action");
            require(tape.steps.back()[0].x_movement == 0.0f && !tape.steps.back()[0].primary_shoot,
                "Blockout-route driver changed the stationary human's input");
            const Player& bot = level->player(1);
            require(bot.player_num() == -1 && bot.team() == trial.team && bot.primary_weapon().type() == trial.tool &&
                std::isfinite(bot.center().x) && std::isfinite(bot.center().y) && std::isfinite(bot.weapon_ammo()) &&
                bot.weapon_ammo() >= 0.0f, "Blockout-route bot changed identity/team/tool or produced invalid state");
            const bool alive = bot.state() == PlayerState::alive;
            result.deaths += was_alive && !alive ? 1 : 0; was_alive = alive;
            result.jumps += input.jump_pressed ? 1 : 0;
            for (std::size_t chain = 0; chain < route.size(); ++chain)
            {
                ChainResult& x = result.chains[chain];
                if (!x.completed() && on_support(bot, route[chain].contacts[x.next].support)) { x.steps[x.next++] = tick; }
            }
        }
        result.owned_paint = level->level_end_info().player_areas.at(static_cast<std::size_t>(paint_owners::index(-1, 1)));
        std::cout << "MEASURE bot blockout route start=" << start_name(trial.start) <<
            " team=" << (trial.team == PlayerTeam::a ? 'a' : 'b') << " tool=" << static_cast<int>(trial.tool) <<
            " difficulty=" << static_cast<int>(trial.difficulty);
        for (std::size_t chain = 0; chain < route.size(); ++chain)
        {
            const ChainResult& x = result.chains[chain];
            std::cout << ' ' << route[chain].name << '=';
            for (std::size_t contact = 0; contact < x.steps.size(); ++contact)
            { std::cout << route[chain].contacts[contact].name << ':' << x.steps[contact] << ','; }
            std::cout << (x.completed() ? "completed" : "stopped_before:") <<
                (x.completed() ? "" : route[chain].contacts[x.next].name);
        }
        std::cout << " takeoffs=" << result.jumps << " deaths=" << result.deaths << " owned_paint=" << result.owned_paint <<
            " final_x=" << level->player(1).center().x << " final_feet=" << level->player(1).rectangle().bottom() << '\n';
        if (replay_check)
        {
            const std::vector<std::uint64_t> expected = snapshot(*level);
#if COLOURWARS_DEV_TOOLS
            const MatchFingerprint expected_full = MatchFingerprint::of(*level, trial_steps);
#endif
            level.reset();
            level = world.build(settings); prepare(*level, world, trial);
            tape.playback = true;
            for (int tick = 0; tick < trial_steps; ++tick) { level->update(physical, dt, &tape); }
            require(tape.next == trial_steps && snapshot(*level) == expected,
                "Blockout-route final-input replay changed public players, projectile count or exact paint state");
#if COLOURWARS_DEV_TOOLS
            require(MatchFingerprint::difference(expected_full, MatchFingerprint::of(*level, trial_steps)).empty(),
                "Blockout-route final-input replay changed the complete Debug fingerprint");
#endif
        }
        return result;
    }
}

int main(int argc, char** argv)
{
    return acceptance::run_probe("bot blockout routes", [&]
    {
        require(argc == 2, "usage: bot_blockout_routes <content>");
        World world(std::filesystem::absolute(argv[1]), "bot-blockout-routes",
            "Bot blockout route needs authored support '", "turbulence_blockout");
        int trials = 0, replays = 0;
        for (const Start start : { Start::home, Start::upper, Start::centre })
        {
            for (const PlayerTeam team : { PlayerTeam::a, PlayerTeam::b })
            {
                struct Tally
                {
                    int completed = 0, contacts = 0;
                    long long completion_sum = 0;
                    std::map<std::string, int> stops;
                };
                std::vector<Tally> tally;
                std::vector<Chain> route;
                int team_trials = 0, takeoffs = 0, deaths = 0;
                double paint = 0.0;
                for (int tool = 0; tool < 5; ++tool)
                {
                    for (const BotDifficulty difficulty : { BotDifficulty::easy, BotDifficulty::normal, BotDifficulty::hard })
                    {
                        const Trial trial{ start, team, static_cast<WeaponType>(tool), difficulty };
                        const bool replay = tool == 0 && difficulty == BotDifficulty::normal && team == PlayerTeam::a;
                        const TrialResult result = run(world, trial, replay);
                        if (route.empty()) { route = chains(world, trial); tally.resize(route.size()); }
                        for (std::size_t chain = 0; chain < route.size(); ++chain)
                        {
                            const ChainResult& x = result.chains[chain];
                            Tally& t = tally[chain];
                            t.contacts += static_cast<int>(x.next);
                            if (x.completed()) { ++t.completed; t.completion_sum += x.steps.back(); }
                            else { ++t.stops[route[chain].contacts[x.next].name]; }
                        }
                        takeoffs += result.jumps; deaths += result.deaths; paint += result.owned_paint;
                        replays += replay ? 1 : 0; ++trials; ++team_trials;
                    }
                }
                for (std::size_t chain = 0; chain < route.size(); ++chain)
                {
                    const Tally& t = tally[chain];
                    std::cout << "MEASURE bot blockout routes start=" << start_name(start) << " team=" <<
                        (team == PlayerTeam::a ? 'a' : 'b') << " chain=" << route[chain].name << " completed=" <<
                        t.completed << '/' << team_trials << " contacts_reached=" << t.contacts << '/' <<
                        team_trials * static_cast<int>(route[chain].contacts.size()) << " arrival_update_mean=" <<
                        (t.completed > 0 ? static_cast<double>(t.completion_sum) / t.completed : -1.0) << " stops=";
                    for (const auto& [stop, count] : t.stops) { std::cout << stop << ':' << count << ','; }
                    std::cout << " takeoffs=" << takeoffs << " deaths=" << deaths << " mean_owned_paint=" <<
                        paint / team_trials << '\n';
                }
            }
        }
        require(trials == 90 && replays == 3, "Blockout-route matrix changed size");
        std::cout << "PASS 90 bounded actual-driver Turbulence Yard blockout trials (home spawn, upper approach and "
            "lower-crossing starts; both teams, five tools, three difficulties; 3600 steps) and three final-input "
            "replays; chain completion, stop points, arrival updates, takeoffs and paint are measurements, not "
            "requirements\n";
        return 0;
    });
}
