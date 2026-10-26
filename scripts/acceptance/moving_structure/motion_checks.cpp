#include "game/pch.h"
#include "scripts/acceptance/moving_structure/fixtures.h"

namespace moving_structure_acceptance
{
    namespace
    {
        void constructor_and_segments()
        {
            const mattmath::RectangleF anchor(300.0f, 500.0f, 200.0f, 10.0f);
            for (int quarter = 0; quarter < 4; ++quarter)
            {
                const HorizontalMotion motion({ anchor, 100.0f, 1.0f, quarter * 0.25f });
                const auto state = motion.state();
                const float offsets[] = { 0.0f, 50.0f, 100.0f, 50.0f };
                require(bits(state.current_offset.x) == bits(offsets[quarter]), "Quarter-phase initial offset is wrong");
                require(state.direction == (quarter < 2 ? 1 : -1), "Quarter-phase outgoing direction is wrong");
                const auto unchanged = motion_bits(state);
                const auto zero = motion.prepare(0.0f);
                require(zero.collision.segment_count == 0 && zero.collision.before == zero.collision.after &&
                    zero.collision.before == motion.bounds(), "Zero preparation reused positive movement");
                require(motion_bits(zero.after_state) == unchanged && motion_bits(motion.state()) == unchanged,
                    "Preparation changed persisted motion state");
            }
            const HorizontalMotion normal({ anchor, 40.0f, 2.0f * SECONDS_PER_STEP, 0.0f });
            const auto endpoint = normal.prepare(SECONDS_PER_STEP);
            require(endpoint.collision.segment_count == 1 && endpoint.after_state.phase == 0.5 &&
                endpoint.after_state.direction == -1 && endpoint.after_state.current_offset.x == 40.0f,
                "Endpoint must retain one interval and choose its outgoing direction");
            const HorizontalMotion reverse({ anchor, 40.0f, 2.0f * SECONDS_PER_STEP, 0.25f });
            const auto turn = reverse.prepare(SECONDS_PER_STEP);
            require(turn.after_state.phase == 0.75 && turn.after_state.direction == -1 &&
                turn.collision.before == turn.collision.after && turn.collision.segment_count == 2,
                "An equal-net interior reversal was collapsed to an endpoint chord");
            require(turn.collision.segments[0].begin == 0.0 && turn.collision.segments[0].end == 0.5 &&
                turn.collision.segments[1].begin == 0.5 && turn.collision.segments[1].end == 1.0 &&
                turn.collision.segments[0].to_x == 40.0f && turn.collision.segments[1].from_x == 40.0f,
                "Interior reversal does not retain both chronological paths");
            const HorizontalMotion wrapped({ anchor, 40.0f, 2.0f * SECONDS_PER_STEP, 0.5f });
            const auto wrap = wrapped.prepare(SECONDS_PER_STEP);
            require(wrap.after_state.phase == 0.0 && wrap.after_state.direction == 1 &&
                wrap.collision.segment_count == 1, "Phase wrap selected stale direction or duplicate interval");
            const HorizontalMotion rounded({ anchor, 4.0f, (std::numeric_limits<float>::max)(), 0.25f });
            const auto rounded_step = rounded.prepare(dt);
            require(rounded_step.collision.segment_count == 1 && rounded_step.collision.segments[0].begin == 0.0 &&
                rounded_step.collision.segments[0].end == 1.0 &&
                rounded_step.collision.segments[0].from_x == rounded_step.collision.segments[0].to_x,
                "A positive-duration rounded-stationary step lost flying-projectile coverage");
            const float nan = std::numeric_limits<float>::quiet_NaN();
            const float infinity = std::numeric_limits<float>::infinity();
            for (float bad : { -dt, nan, infinity, std::nextafter(SECONDS_PER_STEP, infinity), 1.0f })
            { throws_with([&] { std::ignore = normal.prepare(bad); }, "Horizontal motion"); }
            for (float bad : { 0.0f, -1.0f, nan, infinity })
            { throws_with([&] { std::ignore = HorizontalMotion({ anchor, bad, 1.0f, 0.0f }); }, "Horizontal motion"); }
            for (float bad : { 0.0f, std::nextafter(2.0f * SECONDS_PER_STEP, 0.0f), nan, infinity })
            { throws_with([&] { std::ignore = HorizontalMotion({ anchor, 40.0f, bad, 0.0f }); }, "Horizontal motion"); }
            for (float bad : { -0.1f, 1.0f, nan, infinity })
            { throws_with([&] { std::ignore = HorizontalMotion({ anchor, 40.0f, 1.0f, bad }); }, "Horizontal motion"); }
            throws_with([&] { std::ignore = HorizontalMotion({ { nan, 500.0f, 200.0f, 10.0f }, 40.0f, 1.0f, 0.0f }); }, "representable");
            throws_with([&] { std::ignore = HorizontalMotion({ { 1.0e20f, 500.0f, 1.0f, 10.0f }, 40.0f, 1.0f, 0.0f }); }, "representable");
        }

        std::vector<std::uint64_t> active_trace(World& world, bool insert_zero_reads)
        {
            Fixture fixture(world);
            fixture.plain({ -512.0f, 1200.0f, 4000.0f, 80.0f });
            auto& mover = fixture.mover({ { 512.0f, 500.0f, 128.0f, 10.0f }, 96.0f, 2.0f, 0.25f });
            auto& player = fixture.add_player(100.0f, 1200.0f);
            fixture.build();
            const auto initial = motion_bits(mover.motion_state());
            fixture.tick();
            require(!fixture.level->match_started() && motion_bits(mover.motion_state()) == initial,
                "Opening count-in advanced the platform");
            fixture.start();
            require(mover.motion_id() == 1 && fixture.level->has_horizontal_motion(), "Level did not bind actual admitted motion");
            std::vector<std::uint64_t> trace;
            bool moved = false; bool forward = false; bool backward = false; bool turn = false;
            for (int step = 0; step < 400; ++step)
            {
                const auto before = mover.motion_state();
                fixture.tick();
                const auto after = mover.motion_state();
                const auto collision = mover.collision_step();
                require(collision.segment_count >= 1 && collision.segment_count <= 2,
                    "An active actual Level tick lost its chronological step");
                moved |= before.current_offset.x != after.current_offset.x;
                forward |= after.direction == 1; backward |= after.direction == -1;
                turn |= collision.segment_count == 2;
                const auto row = motion_bits(after); trace.insert(trace.end(), row.begin(), row.end());
                const auto shape = step_bits(collision); trace.insert(trace.end(), shape.begin(), shape.end());
                if (insert_zero_reads)
                {
                    const auto body = player.rectangle(); const auto ammo = bits(player.weapon_ammo());
                    const auto persisted = motion_bits(after);
                    for (int pause = 0; pause < 2; ++pause)
                    {
                        fixture.tick(input(), 0.0f);
                        const auto zero = mover.collision_step();
                        require(zero.segment_count == 0 && zero.before == zero.after && zero.after == mover.bounds(),
                            "Zero actual Level update reused positive platform travel");
                        require(motion_bits(mover.motion_state()) == persisted && player.rectangle() == body &&
                            bits(player.weapon_ammo()) == ammo, "Zero actual update changed persisted world state");
                        std::ignore = snapshot(mover); std::ignore = mover.runoff_visuals();
                        std::ignore = mover.cull_bounds(4.0f);
                    }
                }
            }
            require(moved && forward && backward && turn, "Repeat trace was trivial or failed to exercise actual reversals");
            trace.push_back(static_cast<std::uint64_t>(std::rand()));
            return trace;
        }
    }
    void check_motion(World& world)
    {
        constructor_and_segments();
        const auto first = active_trace(world, false);
        require(active_trace(world, false) == first, "Same real Level motion trace is not bit-identical on rebuild");
        require(active_trace(world, true) == first, "Zero updates/observation changed real motion trace or RNG");
    }
}
