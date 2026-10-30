#include "game/pch.h"
#include "scripts/acceptance/paint_surfaces/fixtures.h"
#include "game/ai/match_bot_driver.h"
#include "scripts/acceptance/support/probe.h"
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>

// Measure only: the wall time the shipping bot driver adds to a simulation
// step on every shipped arena. Gates only that the case runs and bots move.
namespace
{
    using namespace labrador;
    using namespace mattmath;
    using paint_surfaces_acceptance::require;
    using Clock = std::chrono::steady_clock;

    constexpr int trial_steps = 1800;
    constexpr unsigned int trial_seed = 41731;
    constexpr double frame_budget_us = 1.0e6 / 60.0;
    const float dt = static_cast<float>(StepTimer::TicksToSeconds(StepTimer::SecondsToTicks(1.0 / 60.0)));
    const char* const stages[] = { "crossfeed", "close_quarters", "king_of_the_hill", "turbulence", "press_tower" };

    double micros(Clock::duration elapsed)
    { return std::chrono::duration<double, std::micro>(elapsed).count(); }

    class World
    {
    public:
        explicit World(const std::filesystem::path& content) : assets(content)
        {
            for (const char* stage : stages)
            {
                auto info = std::make_unique<LevelLoadedInfo>(path_to_utf8(content / "levels" /
                    path_from_utf8(std::string(stage) + ".json")).c_str());
                info->validate(); levels.add(stage, std::move(info));
            }
        }
        std::unique_ptr<Level> build(const MatchSetup& settings)
        {
            std::srand(trial_seed);
            viewports.set_layout(settings.screen_layout());
            LevelBuilder builder(&viewports, &assets.render, &assets.audio, &assets.preferences,
                &levels, &assets.tuning, &assets.presentation, &resolution, nullptr, &partitioner);
            return builder.build_level(settings, TeamColourPair::blue_orange, OpeningMode::off);
        }
        paint_surfaces_acceptance::World assets;
        Registry<LevelLoadedInfo> levels{ "bot-scan-cost" };
        ResolutionManager resolution;
        ViewportManager viewports{ &resolution };
        Mouse mouse;
        Partitioner partitioner;
    };

    struct Trial
    {
        const char* stage;
        int bots;
        int rotation;
        BotDifficulty difficulty;
        WeaponType tool(int seat) const { return static_cast<WeaponType>((rotation + seat) % 5); }
    };

    // One idle human view; bots alternate teams so every roster has opponents.
    MatchSetup roster(const Trial& trial)
    {
        MatchSetup settings;
        settings.set_player_count(trial.bots + 1);
        for (int seat = 1; seat <= trial.bots; ++seat) { settings.set_player_bot(seat, trial.difficulty); }
        settings.begin_input_claims(input_slots::keyboard_and_mouse);
        settings.set_screen_layout(ScreenLayout::one_player);
        settings.set_stage(trial.stage); settings.set_duration(120.0f);
        for (int seat = 0; seat <= trial.bots; ++seat)
        {
            settings.set_player_team(seat, seat % 2 == 0 ? PlayerTeam::a : PlayerTeam::b);
            settings.set_player_weapon(seat, trial.tool(seat));
        }
        return settings;
    }

    std::vector<PlayerInputData> physical_inputs()
    {
        std::vector<PlayerInputData> physical(input_slots::count);
        for (PlayerInputData& input : physical) { input.connected = true; }
        return physical;
    }

    enum class Work { idle, scan, replan };

    // Replaces the level's own driver with an identical one so its call can be
    // timed. The step's work class uses the driver's own territory predicate:
    // a replan samples territory; a ready slow tool or finishing window scans
    // supports without sampling.
    class TimedDriver final : public SeatInputFilter
    {
    public:
        TimedDriver(const GameTuning* tuning, const MatchSetup& settings) :
            driver_(tuning, settings, trial_seed) {}
        void filter_seats(std::vector<PlayerInputData>& seats, const Level& level) override
        {
            work = Work::idle;
            for (int seat = 0; seat < level.player_count(); ++seat)
            {
                const MatchBot* bot = this->driver_.bot(seat);
                const Player& player = level.player(seat);
                if (bot == nullptr || player.state() != PlayerState::alive) { continue; }
                if (bot->needs_territory()) { work = Work::replan; }
                else if (work == Work::idle && (bot->finishing() || (bot->deliberate(player.primary_weapon().type()) &&
                    player.primary_weapon().readiness() == WeaponReadiness::ready))) { work = Work::scan; }
            }
            const Clock::time_point start = Clock::now();
            this->driver_.filter_seats(seats, level);
            elapsed = Clock::now() - start;
        }
        bool replaces_bot_inputs() const override { return true; }
        Work work = Work::idle;
        Clock::duration elapsed{};
    private:
        MatchBotDriver driver_;
    };

    struct Series
    {
        std::vector<double> samples;
        void add(double value) { samples.push_back(value); }
        double mean() const
        {
            double total = 0.0;
            for (const double value : samples) { total += value; }
            return samples.empty() ? 0.0 : total / static_cast<double>(samples.size());
        }
        double percentile(double fraction) const
        {
            if (samples.empty()) { return 0.0; }
            std::vector<double> sorted = samples;
            const std::size_t index = std::min(sorted.size() - 1,
                static_cast<std::size_t>(fraction * static_cast<double>(sorted.size())));
            std::nth_element(sorted.begin(), sorted.begin() + static_cast<std::ptrdiff_t>(index), sorted.end());
            return sorted[index];
        }
        double max() const { return samples.empty() ? 0.0 : *std::max_element(samples.begin(), samples.end()); }
        void append(const Series& other) { samples.insert(samples.end(), other.samples.begin(), other.samples.end()); }
    };

    struct Costs
    {
        Series step, driver, rest;
        Series by_work[3];
        Series surfaces;
        std::size_t surface_count = 0;
        void append(const Costs& other)
        {
            step.append(other.step); driver.append(other.driver); rest.append(other.rest);
            for (int index = 0; index < 3; ++index) { by_work[index].append(other.by_work[index]); }
            surfaces.append(other.surfaces);
            surface_count = std::max(surface_count, other.surface_count);
        }
    };

    void report(const std::string& label, const Costs& costs)
    {
        std::cout << std::fixed << std::setprecision(2) << "MEASURE bot scan cost " << label <<
            " steps=" << costs.step.samples.size() <<
            " step_us mean=" << costs.step.mean() << " p99=" << costs.step.percentile(0.99) << " max=" << costs.step.max() <<
            " filter_seats_us mean=" << costs.driver.mean() << " p99=" << costs.driver.percentile(0.99) <<
            " max=" << costs.driver.max() <<
            " rest_of_update_us mean=" << costs.rest.mean() << " p99=" << costs.rest.percentile(0.99) <<
            " idle_steps=" << costs.by_work[0].samples.size() << " idle_us mean=" << costs.by_work[0].mean() <<
            " scan_steps=" << costs.by_work[1].samples.size() << " scan_us mean=" << costs.by_work[1].mean() <<
            " p99=" << costs.by_work[1].percentile(0.99) <<
            " replan_steps=" << costs.by_work[2].samples.size() << " replan_us mean=" << costs.by_work[2].mean() <<
            " p99=" << costs.by_work[2].percentile(0.99) <<
            " paint_surfaces_us mean=" << costs.surfaces.mean() << " surfaces=" << costs.surface_count <<
            " filter_p99_frame_share=" << costs.driver.percentile(0.99) / frame_budget_us << '\n';
    }

    Costs run(World& world, const Trial& trial, int& moved, int& bots)
    {
        const MatchSetup settings = roster(trial);
        std::unique_ptr<Level> level = world.build(settings);
        const std::vector<PlayerInputData> physical = physical_inputs();
        for (int tick = 0; tick < 900 && level->state() != LevelState::active; ++tick) { level->update(physical, dt); }
        require(level->state() == LevelState::active, "Scan-cost trial did not finish the ordinary countdown");
        TimedDriver timed(&world.assets.tuning, settings);
        std::vector<Vector2F> start, low, high;
        for (int seat = 0; seat < level->player_count(); ++seat)
        {
            start.push_back(level->player(seat).center());
        }
        low = high = start;
        Costs costs;
        for (int tick = 0; tick < trial_steps; ++tick)
        {
            const Clock::time_point begin = Clock::now();
            level->update(physical, dt, &timed);
            const double step = micros(Clock::now() - begin);
            const double driver = micros(timed.elapsed);
            require(level->state() == LevelState::active && std::isfinite(step) && std::isfinite(driver),
                "Scan-cost trial left its active match or produced invalid timing");
            costs.step.add(step); costs.driver.add(driver); costs.rest.add(step - driver);
            costs.by_work[static_cast<int>(timed.work)].add(driver);
            if (timed.work != Work::idle)
            {
                // The driver's one surface copy, timed apart; it changes no state.
                const Clock::time_point copy = Clock::now();
                const std::vector<PaintSurfaceSample> surfaces = level->paint_surfaces();
                costs.surfaces.add(micros(Clock::now() - copy));
                costs.surface_count = std::max(costs.surface_count, surfaces.size());
            }
            for (int seat = 1; seat < level->player_count(); ++seat)
            {
                const Vector2F center = level->player(seat).center();
                low[static_cast<size_t>(seat)].x = std::min(low[static_cast<size_t>(seat)].x, center.x);
                low[static_cast<size_t>(seat)].y = std::min(low[static_cast<size_t>(seat)].y, center.y);
                high[static_cast<size_t>(seat)].x = std::max(high[static_cast<size_t>(seat)].x, center.x);
                high[static_cast<size_t>(seat)].y = std::max(high[static_cast<size_t>(seat)].y, center.y);
            }
        }
        for (int seat = 1; seat < level->player_count(); ++seat)
        {
            const Vector2F extent = high[static_cast<size_t>(seat)] - low[static_cast<size_t>(seat)];
            moved += extent.x + extent.y > world.assets.tuning.bots().progress_distance ? 1 : 0;
            ++bots;
        }
        return costs;
    }
}

int main(int argc, char** argv)
{
    return acceptance::run_probe("bot scan cost", [&]
    {
        require(argc == 2, "usage: bot_scan_cost <content>");
        World world(std::filesystem::absolute(argv[1]));
        Costs all;
        int moved = 0, bots = 0, trials = 0;
        for (const char* stage : stages)
        {
            Costs arena;
            for (int count = 1; count <= 3; ++count)
            {
                Costs roster_costs;
                for (const BotDifficulty difficulty : { BotDifficulty::easy, BotDifficulty::normal, BotDifficulty::hard })
                {
                    for (int rotation = 0; rotation < 5; ++rotation)
                    {
                        roster_costs.append(run(world, { stage, count, rotation, difficulty }, moved, bots));
                        ++trials;
                    }
                }
                report(std::string(stage) + " bots=" + std::to_string(count), roster_costs);
                arena.append(roster_costs);
            }
            report(std::string(stage) + " all", arena);
            all.append(arena);
        }
        report("all", all);
        std::cout << "MEASURE bot scan cost trials=" << trials << " bots_moved=" << moved << '/' << bots << '\n';
        require(trials == 225 && bots == 450 && moved == bots, "A scan-cost bot did not move");
        std::cout << "PASS 225 bounded 1-3 bot scan-cost trials over five shipped arenas, five tool rotations and three "
            "difficulties, 1800 active steps each; every bot moved; timings are measurements against a "
            << std::setprecision(1) << frame_budget_us / 1000.0 << " ms frame\n";
        return 0;
    });
}
