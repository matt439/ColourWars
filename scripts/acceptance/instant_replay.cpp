#include "game/pch.h"
#include "scripts/acceptance/paint_surfaces/fixtures.h"
#include "game/round_tape.h"
#include "scripts/acceptance/support/probe.h"
#include <iostream>

// ROAD#19: the end-of-round instant replay is the same round built again from
// its tape. A round is played as GameLevel plays it - full intro, shipping
// bots, a scripted pad human, a pause in the middle - and taped through the
// same recorder; it is then rebuilt as GameLevel::start_replay rebuilds it
// (no intro, the countdown skipped, every seat from the tape, the pause's
// gaps reapplied) and must match on every taped step, to the bit, through to
// the final paint. Uses the shipping API only, so Release checks it too.

namespace
{
    using namespace labrador;
    using namespace mattmath;
    using paint_surfaces_acceptance::require;

    class World
    {
    public:
        explicit World(const std::filesystem::path& content) : assets(content)
        {
            for (const char* stage : { "crossfeed", "turbulence", "king_of_the_hill", "close_quarters", "press_tower",
                "turbulence_ferry_trial", "turbulence_gate_trial" })
            {
                auto info = std::make_unique<LevelLoadedInfo>((content / "levels" /
                    (std::string(stage) + ".json")).string().c_str());
                info->validate(); levels.add(stage, std::move(info));
            }
        }
        std::unique_ptr<Level> build(const MatchSetup& settings, OpeningMode opening)
        {
            viewports.set_layout(settings.screen_layout());
            LevelBuilder builder(&viewports, &assets.render, &assets.audio, &assets.preferences,
                &levels, &assets.tuning, &assets.presentation, &resolution, nullptr, &partitioner);
            // Random, so the palette draw is part of what the seed must repeat.
            return builder.build_level(settings, TeamColourPair::random, opening);
        }
        paint_surfaces_acceptance::World assets;
        Registry<LevelLoadedInfo> levels{ "instant-replay" };
        ResolutionManager resolution;
        ViewportManager viewports{ &resolution };
        Mouse mouse;
        Partitioner partitioner;
    };

    // P1 a pad human, the other three shipping bots, on alternating teams.
    MatchSetup roster(const char* stage)
    {
        MatchSetup settings; settings.set_player_count(4);
        settings.begin_input_claims(0);
        std::ignore = settings.claim_input_slot(0);
        for (int seat = 1; seat < 4; ++seat)
        {
            settings.set_player_bot(seat, static_cast<BotDifficulty>(seat % 3));
        }
        settings.set_screen_layout(static_cast<ScreenLayout>(settings.human_count() - 1));
        settings.set_stage(stage); settings.set_duration(20.0f);
        for (int seat = 0; seat < 4; ++seat)
        {
            settings.set_player_team(seat, seat % 2 == 0 ? PlayerTeam::a : PlayerTeam::b);
            settings.set_player_weapon(seat, static_cast<WeaponType>((seat + 1) % 5));
        }
        return settings;
    }

    // What the replay is compared on, at one taped step.
    struct Moment
    {
        std::vector<RectangleF> players;
        std::vector<PlayerState> states;
        // The palette is drawn from rand() at the build: the seed's part.
        std::vector<Colour> colours;
        int projectiles = 0;
        bool operator==(const Moment&) const = default;
    };

    Moment moment(const Level& level)
    {
        Moment taken;
        for (int seat = 0; seat < level.player_count(); ++seat)
        {
            taken.players.push_back(level.player(seat).rectangle());
            taken.states.push_back(level.player(seat).state());
            taken.colours.push_back(level.player(seat).team_colour());
        }
        taken.projectiles = level.projectile_count();
        return taken;
    }

    bool same_paint(const LevelEndInfo& a, const LevelEndInfo& b)
    {
        return a.total_area == b.total_area && a.team_a_area == b.team_a_area &&
            a.team_b_area == b.team_b_area && a.player_areas == b.player_areas;
    }

    // The pad human: walks, turns, jumps and sprays on a fixed script.
    PlayerInputData scripted(int tick)
    {
        PlayerInputData input; input.connected = true;
        input.x_movement = (tick / 90) % 2 == 0 ? 1.0f : -1.0f;
        input.jump_held = tick % 70 < 12;
        input.jump_pressed = tick % 70 == 0;
        input.primary_shoot = (tick / 45) % 3 != 0;
        const float angle = static_cast<float>(tick % 360) * 0.0174533f;
        input.right_analog_stick = Vector2F(std::cos(angle), std::sin(angle));
        input.shoot_direction = input.right_analog_stick;
        input.shoot_direction_requested = true;
        input.shoot_angle = input.shoot_direction.angle();
        return input;
    }

    void check_stage(World& world, const char* stage, unsigned int seed)
    {
        const MatchSetup settings = roster(stage);
        const float step = static_cast<float>(StepTimer::TicksToSeconds(StepTimer::SecondsToTicks(1.0 / 60.0)));

        // The round, as GameLevel plays it.
        RoundTape tape;
        RoundTapeRecorder recorder(&tape);
        std::srand(seed);
        tape.start(seed, settings);
        auto level = world.build(settings, OpeningMode::full);
        std::vector<Moment> seen;
        std::vector<PlayerInputData> devices(input_slots::count);
        for (auto& device : devices) { device.connected = true; }
        int tick = 0;
        bool paused = false;
        while (level->state() != LevelState::zoom_out && tick < 20000)
        {
            devices[0] = scripted(tick);
            if (tape.steps() == 300 && !paused)
            {
                // Pause > Resume: the suspend, a held jump over the count-in.
                paused = true;
                tape.record_suspend(); level->suspend();
                std::vector<PlayerInputData> held = devices; held[0].jump_held = true;
                tape.record_resume(held); level->prepare_resume(held);
                level->resume();
            }
            const int before = tape.steps();
            recorder.prepare(nullptr, step);
            level->update(devices, step, &recorder);
            if (tape.steps() != before) { seen.push_back(moment(*level)); }
            ++tick;
        }
        require(level->state() == LevelState::zoom_out && paused, "The taped round did not reach time up");
        require(tape.playable() && tape.steps() == static_cast<int>(seen.size()) &&
            tape.steps() > 1100 && tape.steps() < 1300,
            "The tape did not hold exactly the round's logic steps");
        require(tape.step_before_end(8.0f) == tape.steps() - 480 && tape.step_before_end(1000.0f) == 0,
            "The replay's first step is not eight seconds before time up");
        const LevelEndInfo played = level->level_end_info();
        const std::vector<PaintSurfaceSample> played_surfaces = level->paint_surfaces();
        level.reset();

        // The replay, as GameLevel::start_replay builds and steps it.
        std::srand(tape.seed());
        auto replay = world.build(tape.settings(), OpeningMode::off);
        replay->set_player_cheats(tape.damage_immune(), tape.infinite_ammo());
        replay->start_without_countdown();
        RoundTapePlayer player(&tape, 0);
        require(player.replaces_bot_inputs(), "The replay let the rebuilt level's bots decide");
        std::vector<PlayerInputData> neutral(input_slots::count);
        for (auto& device : neutral) { device.connected = true; }
        while (!player.finished())
        {
            const int at = player.step();
            tape.apply_gaps(*replay, at);
            replay->update(neutral, tape.dt(at), &player);
            require(player.step() == at + 1, "A replay update did not consume exactly one taped step");
            if (!(moment(*replay) == seen[static_cast<size_t>(at)]))
            {
                throw std::runtime_error(std::string(stage) + ": the replay diverged at taped step " +
                    std::to_string(at) + " of " + std::to_string(tape.steps()));
            }
        }
        require(replay->state() == LevelState::zoom_out, "The replay did not end where time ran out");
        require(same_paint(replay->level_end_info(), played), "The replay's paint differs from the round's");
        const std::vector<PaintSurfaceSample> replay_surfaces = replay->paint_surfaces();
        require(replay_surfaces.size() == played_surfaces.size(), "The replay's paint surfaces differ");
        std::cout << "PASS " << stage << ": " << tape.steps() << " taped steps replay bit for bit "
            "from a full-intro round with a pause, through to the final paint\n";
    }

    void check_tape_rules()
    {
        RoundTape tape;
        require(!tape.playable(), "An unstarted tape was playable");
        tape.start(7, MatchSetup());
        require(!tape.playable(), "An empty tape was playable");
        tape.set_cheats(true, false);
        require(tape.damage_immune() && !tape.infinite_ammo(), "Cheats before the first step were lost");
        tape.record_step(std::vector<PlayerInputData>(2), 0.5f);
        tape.record_step(std::vector<PlayerInputData>(2), 0.5f);
        require(tape.playable() && tape.steps() == 2 && tape.step_before_end(0.75f) == 0 &&
            tape.step_before_end(0.5f) == 1, "Tape steps or their seconds were miscounted");
        tape.set_cheats(true, false);
        require(tape.playable(), "Re-applying the same cheats spoiled the tape");
        tape.set_cheats(false, false);
        require(!tape.playable(), "Changing cheats mid-round left the tape playable");
        tape.start(8, MatchSetup());
        tape.record_step(std::vector<PlayerInputData>(2), 0.5f);
        tape.spoil();
        tape.record_step(std::vector<PlayerInputData>(2), 0.5f);
        require(!tape.playable() && tape.seed() == 8, "A spoiled tape recorded again or lost its seed");
        tape.start(9, MatchSetup());
        require(!tape.playable() && tape.steps() == 0 && tape.seed() == 9 && !tape.damage_immune(),
            "A new round's tape kept the last round's steps, spoil or cheats");
        std::cout << "PASS tape rules: empty and spoiled tapes are not offered; cheats change only before play\n";
    }

    void check_disabled_capture(World& world)
    {
        class InputReplacement final : public SeatInputFilter
        {
        public:
            void filter_seats(std::vector<PlayerInputData>& seats, const Level&) override
            {
                for (auto& seat : seats) { seat = scripted(calls); }
                ++calls;
            }
            bool replaces_bot_inputs() const override { return true; }
            int calls = 0;
        } next;

        // The same recorder stays in the chain when capture is disabled. It
        // must still supply downstream input and suppress the shipping bots.
        RoundTape tape;
        RoundTapeRecorder recorder(&tape);
        const auto level = world.build(roster("crossfeed"), OpeningMode::off);
        const auto same_input = [](const PlayerInputData& actual, const PlayerInputData& expected)
        {
            return actual.connected == expected.connected && actual.x_movement == expected.x_movement &&
                actual.jump_pressed == expected.jump_pressed && actual.jump_held == expected.jump_held &&
                actual.primary_shoot == expected.primary_shoot && actual.shoot_direction == expected.shoot_direction;
        };
        recorder.prepare(&next, 1.0f / 60.0f);
        require(recorder.replaces_bot_inputs(), "Disabled capture lost downstream bot-input replacement");
        for (int tick = 0; tick < 10000; ++tick)
        {
            std::vector<PlayerInputData> seats(4);
            recorder.filter_seats(seats, *level);
            require(same_input(seats.front(), scripted(tick)) && same_input(seats.back(), scripted(tick)),
                "Disabled capture did not forward downstream final input");
            tape.record_suspend(); tape.record_resume(seats);
        }
        require(next.calls == 10000 && tape.steps() == 0 && !tape.playable(),
            "Disabled capture grew input steps or skipped the downstream filter");
        // Capture may be enabled again for the next timed round.
        tape.start(10, roster("crossfeed"));
        tape.record_step(std::vector<PlayerInputData>(4), 1.0f / 60.0f);
        require(tape.steps() == 1 && tape.playable(), "Capture did not restart after being disabled");
        tape.clear();
        recorder.prepare(nullptr, 1.0f / 60.0f);
        require(!recorder.replaces_bot_inputs(), "Disabled capture replaced bots without a downstream filter");
        std::vector<PlayerInputData> unchanged(4, scripted(37));
        recorder.filter_seats(unchanged, *level);
        require(same_input(unchanged.front(), scripted(37)) && tape.steps() == 0,
            "Disabled recorder changed ordinary final input or captured after clear");
        std::cout << "PASS disabled capture: 10000 steps forward downstream input and bot replacement; "
            "capture can restart and clear\n";
    }
}

int main(int argc, char** argv)
{
    return acceptance::run_probe("instant replay", [&]
    {
        require(argc == 2, "usage: instant_replay <content>");
        World world(std::filesystem::absolute(argv[1]));
        check_tape_rules();
        check_disabled_capture(world);
        unsigned int seed = 19019;
        // The ferry trial's movers and the gate trial's gate follow the paint,
        // so they replay only if it does.
        for (const char* stage : { "crossfeed", "turbulence", "king_of_the_hill", "close_quarters", "press_tower",
            "turbulence_ferry_trial", "turbulence_gate_trial" })
        {
            check_stage(world, stage, seed++);
        }
        return 0;
    });
}
