#include "game/pch.h"
#include "scripts/acceptance/bot_navigation/fixtures.h"
#include "game/objects/horizontal_motion.h"
#include "game/simulation.h"
#include "scripts/acceptance/support/probe.h"
#include <array>
#include <cstdlib>
#include <iostream>

void check_ferry_cue(const std::filesystem::path& content);

// ROAD#15's first paint-activated stage object: the paint ferry, a horizontal
// mover that travels only while one team holds more of its own paint, toward
// that team's heading. The motion checks drive HorizontalMotion directly; the
// stage checks run bots on the explicit Turbulence Yard ferry trial and hold
// every step of both ferries to the rule, from the paint held before the step.
namespace
{
    using namespace labrador;
    using namespace mattmath;
    using namespace bot_navigation_acceptance;

    constexpr float step = SECONDS_PER_STEP;

    HorizontalMotionSpec ferry(float phase, int team_a_heading = 1)
    {
        HorizontalMotionSpec spec{ RectangleF(100.0f, 500.0f, 200.0f, 20.0f), 120.0f, 4.0f, phase };
        spec.drive = HorizontalDrive::paint;
        spec.team_a_heading = team_a_heading;
        return spec;
    }

    bool refused(const HorizontalMotionSpec& spec)
    {
        try { HorizontalMotion::validate_spec(spec); }
        catch (const std::invalid_argument&) { return true; }
        return false;
    }

    // Steps a standalone motion as a Level would: prepare, then commit a copy.
    class Driven
    {
    public:
        explicit Driven(const HorizontalMotionSpec& spec) : spec_(spec), state_(HorizontalMotion(spec).state()) {}
        HorizontalMotionStep advance(int heading)
        {
            HorizontalMotionSpec resumed = this->spec_;
            // Resuming rounds the phase to a float; the checks allow for it.
            resumed.initial_phase = static_cast<float>(this->state_.phase);
            const HorizontalPreparedStep prepared = HorizontalMotion(resumed).prepare(step, heading);
            this->state_ = prepared.after_state;
            return prepared.collision;
        }
        const HorizontalMotionState& state() const { return this->state_; }
    private:
        HorizontalMotionSpec spec_;
        HorizontalMotionState state_;
    };

    void check_motion()
    {
        require(!refused(ferry(0.0f)) && !refused(ferry(0.5f)) && !refused(ferry(0.25f, -1)),
            "A paint ferry on its outgoing leg was refused");
        require(refused(ferry(0.75f)), "A paint ferry accepted a return-leg phase");
        require(refused(ferry(0.0f, 0)) && refused(ferry(0.0f, 2)), "A paint ferry accepted no team A heading");
        HorizontalMotionSpec cycle = ferry(0.75f);
        cycle.drive = HorizontalDrive::cycle;
        require(!refused(cycle), "A cycle lost its return-leg phase");

        const HorizontalMotionSpec right = ferry(0.0f);
        require(HorizontalMotion::paint_heading(right, 3.0f, 1.0f) == 1 &&
            HorizontalMotion::paint_heading(right, 1.0f, 3.0f) == -1 &&
            HorizontalMotion::paint_heading(right, 2.0f, 2.0f) == 0 &&
            HorizontalMotion::paint_heading(right, 0.0f, 0.0f) == 0, "Team A heading right misread paint");
        const HorizontalMotionSpec left = ferry(0.0f, -1);
        require(HorizontalMotion::paint_heading(left, 3.0f, 1.0f) == -1 &&
            HorizontalMotion::paint_heading(left, 1.0f, 3.0f) == 1, "Team A heading left misread paint");
        require(HorizontalMotion::paint_heading(cycle, 3.0f, 1.0f) == 0, "A cycle read a paint heading");

        // A cycle ignores any heading it is handed.
        const HorizontalMotion cycling(cycle);
        require(cycling.prepare(step, -1).after_state.phase == cycling.prepare(step, 1).after_state.phase,
            "A cycle followed a paint heading");

        // Unpainted, it holds still but still publishes its positive-time step.
        Driven driven(right);
        HorizontalMotionStep held = driven.advance(0);
        require(held.segment_count == 1 && held.segments[0].from_x == 0.0f && held.segments[0].to_x == 0.0f &&
            held.before.x == held.after.x && driven.state().phase == 0.0, "An unpainted ferry moved");
        held = driven.advance(-1);
        require(held.before.x == held.after.x && driven.state().phase == 0.0, "A ferry left its left end stop");

        // Rightward at a cycle leg's speed: 120 units in half of 4 s.
        const HorizontalMotionStep first = driven.advance(1);
        require(first.segment_count == 1 && std::fabs(first.after.x - first.before.x - 60.0f * step) < 1.0e-3f &&
            driven.state().direction == 1, "A team A ferry did not head right at the leg speed");
        int steps = 1;
        bool stopped = false;
        const int crossing = static_cast<int>(std::lround(2.0f / step));
        while (!stopped && steps < crossing * 2)
        {
            const HorizontalMotionStep next = driven.advance(1);
            ++steps;
            stopped = driven.state().phase == 0.5;
            require(next.after.x >= next.before.x, "A ferry heading right moved left");
            if (stopped)
            {
                require(next.segments[next.segment_count - 1].to_x == 120.0f &&
                    driven.state().current_offset.x == 120.0f, "A ferry overran its right end stop");
            }
        }
        require(stopped && std::abs(steps - crossing) <= 1, "A ferry took the wrong time to cross");
        held = driven.advance(1);
        require(held.before.x == held.after.x && driven.state().phase == 0.5, "A ferry left its right end stop");

        // Team B takes it back; a turn mid-path reverses with no end stop.
        for (int index = 0; index < crossing / 4; ++index) { driven.advance(-1); }
        require(driven.state().phase < 0.5 && driven.state().phase > 0.0 &&
            driven.state().current_offset.x < 120.0f, "A team B ferry did not head back left");
        const float turned = driven.state().current_offset.x;
        driven.advance(1);
        require(driven.state().current_offset.x > turned, "A ferry did not turn mid-path");

        // A step that reaches an end stop part-way splits into travel then hold.
        HorizontalMotionSpec near_end = ferry(0.49609375f);
        const HorizontalPreparedStep split = HorizontalMotion(near_end).prepare(step, 1);
        require(split.collision.segment_count == 2 && split.collision.segments[0].end < 1.0 &&
            split.collision.segments[1].from_x == 120.0f && split.collision.segments[1].to_x == 120.0f &&
            split.after_state.phase == 0.5, "A ferry reaching its end mid-step did not split travel and hold");

        // Zero-time updates keep history, as for a cycle.
        const HorizontalPreparedStep zero = HorizontalMotion(ferry(0.25f)).prepare(0.0f, 1);
        require(zero.collision.segment_count == 0 && zero.after_state.phase == 0.25,
            "A zero step moved a ferry");
        std::cout << "PASS paint ferry motion: holds unpainted and at end stops, follows either team at leg "
            "speed, turns mid-path and splits a mid-step stop\n";
    }

    struct Roster
    {
        unsigned int seed;
        std::array<WeaponType, 4> tools;
        BotDifficulty difficulty;
    };

    // Seat 0 is an idle human on team A; seats 1-3 are bots, two of them on B.
    MatchSetup roster(const Roster& variant)
    {
        MatchSetup settings;
        settings.set_player_count(4);
        settings.begin_input_claims(input_slots::keyboard_and_mouse);
        settings.set_screen_layout(ScreenLayout::one_player);
        settings.set_stage("turbulence_ferry_trial");
        settings.set_duration(120.0f);
        for (int seat = 0; seat < 4; ++seat)
        {
            if (seat > 0) { settings.set_player_bot(seat, variant.difficulty); }
            settings.set_player_team(seat, seat < 2 ? PlayerTeam::a : PlayerTeam::b);
            settings.set_player_weapon(seat, variant.tools[static_cast<std::size_t>(seat)]);
        }
        return settings;
    }

    // The same rule as the object, from the anchor-local paint snapshot.
    int expected_heading(const HorizontalStructureState& state, bool& decisive)
    {
        float team_a = 0.0f, team_b = 0.0f;
        for (const PaintSurfaceSample& sample : state.local_paint)
        {
            if (sample.team == PlayerTeam::a) { team_a += sample.scored_area; }
            if (sample.team == PlayerTeam::b) { team_b += sample.scored_area; }
        }
        // Different summation order may only decide a near tie differently.
        decisive = std::fabs(team_a - team_b) > 1.0f || (team_a == 0.0f && team_b == 0.0f);
        return HorizontalMotion::paint_heading(state.motion.spec, team_a, team_b);
    }

    struct Totals
    {
        int moved_steps = 0;
        int carried_moving = 0;
    };

    void check_stage(World& world, const Roster& variant, Totals& totals)
    {
        std::unique_ptr<Level> level = world.build(roster(variant), variant.seed);
        require(level->has_horizontal_motion(), "The ferry trial lost its movers");
        std::vector<HorizontalStructureState> states = level->horizontal_structure_states();
        require(states.size() == 2, "The ferry trial needs exactly two ferries");
        for (const HorizontalStructureState& state : states)
        {
            require(state.motion.spec.drive == HorizontalDrive::paint && state.motion.spec.team_a_heading == 1,
                "A ferry trial mover is not a team-A-right paint ferry");
        }
        const float start_a = states[0].motion.current_offset.x, start_b = states[1].motion.current_offset.x;
        require(start_a == 0.0f && start_b == states[1].motion.spec.travel_x,
            "Each ferry must start at its own team's end");

        const std::vector<PlayerInputData> physical = physical_inputs();
        for (int tick = 0; tick < 900 && level->state() != LevelState::active; ++tick) { level->update(physical, dt); }
        require(level->state() == LevelState::active, "The ferry trial did not finish its countdown");
        states = level->horizontal_structure_states();
        require(states[0].motion.current_offset.x == start_a && states[1].motion.current_offset.x == start_b,
            "An unpainted ferry moved during the countdown");

        int moved_steps = 0, carried_steps = 0, carried_moving = 0, reversals = 0, deaths = 0;
        std::array<int, 2> last_heading{};
        std::array<float, 2> span_min{ start_a, start_b }, span_max{ start_a, start_b };
        std::vector<bool> alive(4, true);
        constexpr int trial_steps = 7200;
        for (int tick = 0; tick < trial_steps && level->state() == LevelState::active; ++tick)
        {
            const std::vector<HorizontalStructureState> before = level->horizontal_structure_states();
            level->update(physical, dt);
            const std::vector<HorizontalStructureState> after = level->horizontal_structure_states();
            for (std::size_t index = 0; index < 2; ++index)
            {
                bool decisive = false;
                const int heading = expected_heading(before[index], decisive);
                const float from = before[index].motion.current_offset.x, to = after[index].motion.current_offset.x;
                const float delta = to - from;
                if (decisive)
                {
                    require(heading != 0 || delta == 0.0f, "A tied or unpainted ferry moved");
                    require(heading <= 0 || delta >= 0.0f, "A ferry moved against its painted heading (right)");
                    require(heading >= 0 || delta <= 0.0f, "A ferry moved against its painted heading (left)");
                    const bool at_stop = (heading > 0 && from == after[index].motion.spec.travel_x) ||
                        (heading < 0 && from == 0.0f);
                    require(heading == 0 || at_stop || delta != 0.0f, "A painted ferry away from its end stop held");
                }
                if (delta != 0.0f)
                {
                    ++moved_steps;
                    const int sign = delta > 0.0f ? 1 : -1;
                    reversals += last_heading[index] != 0 && last_heading[index] != sign ? 1 : 0;
                    last_heading[index] = sign;
                }
                span_min[index] = std::min(span_min[index], to);
                span_max[index] = std::max(span_max[index], to);
            }
            for (int seat = 0; seat < level->player_count(); ++seat)
            {
                const Player& player = level->player(seat);
                require(std::isfinite(player.center().x) && std::isfinite(player.center().y),
                    "A ferry trial player reached an invalid position");
                const bool now = player.state() == PlayerState::alive;
                deaths += alive[static_cast<std::size_t>(seat)] && !now ? 1 : 0;
                alive[static_cast<std::size_t>(seat)] = now;
                if (seat == 0 || player.carrier_id() == 0) { continue; }
                ++carried_steps;
                const HorizontalStructureState& ride = after[player.carrier_id() - 1];
                carried_moving += ride.motion.current_offset.x != ride.motion.previous_offset.x ? 1 : 0;
            }
        }
        std::cout << "MEASURE paint ferry trial seed=" << variant.seed << " steps=" << trial_steps <<
            " ferry_moving_steps=" << moved_steps << " reversals=" << reversals << " span_a=" << span_min[0] <<
            ".." << span_max[0] << " span_b=" << span_min[1] << ".." << span_max[1] << " bot_carried_steps=" <<
            carried_steps << " bot_carried_while_moving=" << carried_moving << " deaths=" << deaths << '\n';
        totals.moved_steps += moved_steps;
        totals.carried_moving += carried_moving;
    }
}

int main(int argc, char** argv)
{
    return acceptance::run_probe("paint ferry", [&]
    {
        require(argc == 2, "usage: paint_ferry <content>");
        check_motion();
        check_ferry_cue(std::filesystem::absolute(argv[1]));
        World world(std::filesystem::absolute(argv[1]), "paint-ferry",
            "Paint ferry trial needs authored support '", "turbulence_ferry_trial");
        const Roster rosters[] = {
            { 41731, { WeaponType::sprayer, WeaponType::roller, WeaponType::bucket, WeaponType::mister },
                BotDifficulty::hard },
            { 9127, { WeaponType::sprayer, WeaponType::sprayer, WeaponType::roller, WeaponType::sniper },
                BotDifficulty::normal },
            { 30011, { WeaponType::sprayer, WeaponType::mister, WeaponType::sprayer, WeaponType::bucket },
                BotDifficulty::easy } };
        Totals totals;
        for (const Roster& variant : rosters) { check_stage(world, variant, totals); }
        require(totals.moved_steps > 0, "No bot painted a ferry into motion in any trial");
        std::cout << "PASS paint ferry trial: two team-A-right ferries load and hold through the countdown; "
            "every step of three whole 3-bot matches follows the paint held before it; riding is a measurement\n";
        return 0;
    });
}
