#include "game/pch.h"
#include "scripts/acceptance/moving_structure/fixtures.h"
#include "game/contracts/horizontal_motion_contract.h"
#include <array>
#include <vector>

// A cycling vertical lift (ROAD#28): the axis, admission with its clearance
// beneath the lowest pose (E3a as amended), the hold (E3b), rider carry by y
// (E2), world queries on the full offset (E4) and the vertical budget (E6).
namespace moving_structure_acceptance
{
    namespace
    {
        using namespace mattmath;
        namespace contract = horizontal_motion_contract;

        // One storey of the Lift Trial: 256 down from a top anchor at y 800,
        // a 3.2 s round trip (160 units/s), its lowest pose's bottom at 1076.
        HorizontalMotionSpec lift(float phase = 0.0f, float x = 600.0f)
        {
            HorizontalMotionSpec spec{ { x, 800.0f, 120.0f, 20.0f }, 0.0f, 3.2f, phase };
            spec.travel_y = 256.0f;
            return spec;
        }
        GameTuning quick_tuning(World& world)
        {
            return world.variant([](rapidjson::Document& document) { set_countdown(document, dt / 4.0f); });
        }
        float feet(const Player& player) { return player.rectangle().bottom(); }

        void check_axis()
        {
            const HorizontalMotionSpec spec = lift();
            require(spec.vertical() && spec.travel() == 256.0f && spec.axis_offset(3.0f) == Vector2F(0.0f, 3.0f),
                "A lift's spec does not name its own axis");
            HorizontalMotion::validate_spec(spec);
            const HorizontalMotion motion(spec);
            require(motion.state().current_offset == Vector2F::ZERO && motion.bounds() == spec.anchor,
                "A lift at phase 0 is not at its top anchor");
            const HorizontalMotion quarter(lift(0.25f));
            require(quarter.state().current_offset == Vector2F(0.0f, 128.0f) &&
                quarter.bounds() == RectangleF(600.0f, 928.0f, 120.0f, 20.0f),
                "A quarter-cycle lift is not halfway down its storey");

            const HorizontalPreparedStep step = motion.prepare(dt);
            require(step.collision.segment_count == 1 && step.collision.segments[0].from == 0.0f &&
                step.collision.segments[0].to > 0.0f &&
                step.after_state.current_offset == Vector2F(0.0f, step.collision.segments[0].to) &&
                step.collision.after.x == step.collision.before.x &&
                step.collision.after.y == 800.0f + step.collision.segments[0].to,
                "A lift's step does not move only on y from its top anchor");
            require_near(step.collision.after.y - step.collision.before.y, 160.0f * dt, 0.001f);

            // The bottom turn splits a step as the horizontal one does.
            const HorizontalMotion turning(lift(0.5f - 0.25f * dt / 3.2f));
            const HorizontalPreparedStep turn = turning.prepare(dt);
            require(turn.collision.segment_count == 2 && turn.collision.segments[0].to == 256.0f &&
                turn.collision.segments[1].from == 256.0f && turn.after_state.direction == -1 &&
                turn.collision.segments[1].to < 256.0f, "A lift's bottom turn lost its split interval");

            // The hold keeps the phase and the pose, with one positive interval.
            const HorizontalPreparedStep held = quarter.hold(dt);
            require(held.after_state.phase == quarter.state().phase &&
                held.after_state.current_offset == quarter.state().current_offset &&
                held.after_state.previous_offset == quarter.state().current_offset &&
                held.collision.segment_count == 1 && held.collision.segments[0].from == 128.0f &&
                held.collision.segments[0].to == 128.0f && held.collision.before == held.collision.after &&
                bits(held.collision.dt) == bits(dt), "A held lift moved or lost its positive-time interval");
            const HorizontalPreparedStep zero = quarter.hold(0.0f);
            require(zero.collision.segment_count == 0, "A zero-time hold published an interval");
            std::cout << "PASS lift motion: y-only triangular travel, quarter pose, bottom turn split and phase hold\n";
        }

        void check_contract()
        {
            const contract::Spec spec = contract_spec(lift());
            const contract::Box path = contract::path(spec);
            require(path.left == 600.0 && path.right == 720.0 && path.top == 800.0 && path.bottom == 1076.0,
                "A lift's path is not its whole shaft");
            const contract::Box standing = contract::standing(path, 52.0f, 120.0f);
            require(standing.bottom == 800.0 && standing.top < 680.0 && standing.top > 679.99 &&
                standing.left < 548.0 && standing.right > 772.0,
                "A lift's standing corridor does not rise with its highest pose");
            const contract::Box underside = contract::underside(path, 52.0f, 120.0f);
            require(underside.top == 1076.0 && underside.bottom > 1196.0 && underside.bottom < 1196.01 &&
                underside.left < 548.0 && underside.left > 547.99 && underside.right > 772.0 && underside.right < 772.01,
                "A lift's clearance beneath its lowest pose is not one padded body height");
            const contract::Box column = contract::column(path, 52.0f, 120.0f);
            require(column.left == underside.left && column.right == underside.right &&
                column.bottom == underside.bottom && column.top < 680.0 && column.top > 679.99,
                "A lift's column is not its corridor, its sides and its clearance together");

            // One axis per mover; the idle axis must be exactly zero.
            const auto refused = [](contract::Spec value, const char* fragment)
            { throws_with([&] { contract::validate_spec(value); }, fragment); };
            contract::Spec both = spec; both.travel_x = 10.0f;
            refused(both, "Motion travels on one axis");
            contract::Spec nan = spec; nan.travel_y = std::numeric_limits<float>::quiet_NaN();
            refused(nan, "Motion travels on one axis");
            contract::Spec negative = spec; negative.travel_y = -256.0f;
            refused(negative, "Motion travels on one axis");
            contract::Spec painted = spec; painted.drive = contract::Drive::paint;
            refused(painted, "a paint-driven lift is not supported yet");
            // At y 1e8 floats are 8 apart: a 1-unit travel rounds back to the anchor.
            contract::Spec off_grid = spec; off_grid.anchor.y = 1.0e8f; off_grid.travel_y = 1.0f;
            refused(off_grid, "distinct representable endpoint");
            throws_with([&] { contract::validate_collision(spec, true); }, "solid paintable collision");
            contract::validate_collision(spec, false);
            contract::Spec horizontal = spec; horizontal.travel_y = 0.0f; horizontal.travel_x = 120.0f;
            contract::validate_collision(horizontal, true);

            // E3b: a step that would push a body through geometry holds.
            const contract::Box world{ 0.0, 0.0, 4000.0, 2000.0 };
            const contract::Box before{ 600.0, 900.0, 720.0, 920.0 };
            const contract::Box down{ 600.0, 903.0, 720.0, 923.0 };
            const contract::Box up{ 600.0, 897.0, 720.0, 917.0 };
            const contract::Box under{ 630.0, 922.0, 682.0, 1042.0 };
            // Pushed clear of the lowered underside the body spans 923..1043.
            const std::vector<contract::Box> floor_close{ { 0.0, 1040.0, 4000.0, 1200.0 } };
            const std::vector<contract::Box> floor_far{ { 0.0, 1200.0, 4000.0, 1400.0 } };
            require(contract::closes_on(before, down, under, false, world, floor_close) &&
                !contract::closes_on(before, down, under, false, world, floor_far),
                "A descending lift's hold does not follow the floor beneath the body");
            require(!contract::closes_on(before, up, under, false, world, floor_close),
                "A rising lift held for a body it is leaving");
            const contract::Box rider{ 630.0, 780.0, 682.0, 900.0 };
            const std::vector<contract::Box> ceiling{ { 0.0, 600.0, 4000.0, 778.0 } };
            require(contract::closes_on(before, up, rider, true, world, ceiling) &&
                !contract::closes_on(before, down, rider, true, world, ceiling) &&
                !contract::closes_on(before, up, rider, true, world, floor_far),
                "A rising lift's hold does not follow the ceiling above its rider");
            require(contract::closes_on(before, up, rider, true, { 0.0, 779.0, 4000.0, 2000.0 }, floor_far),
                "A lift that would carry its rider out of the scene did not hold");
            require(!contract::closes_on(before, before, rider, true, world, ceiling),
                "A lift that is not moving held");
            std::cout << "PASS lift contract: shaft path, corridor above the highest pose, padded clearance "
                "beneath the lowest, one-axis refusals, solid-only and the hold's push test\n";
        }

        void check_admission(World& world)
        {
            const auto make = [&](float floor_top)
            {
                auto fixture = std::make_unique<Fixture>(world);
                fixture->tuning = quick_tuning(world);
                fixture->surface({ -200.0f, floor_top, 1600.0f, 200.0f });
                auto& platform = fixture->mover(lift());
                fixture->add_player(platform.bounds().center().x, platform.bounds().top());
                return fixture;
            };
            // 1200 leaves 124 beneath the 1076 lowest bottom: a body height and its pad.
            { auto fixture = make(1200.0f); fixture->build(); }
            // The flush case and a gap a body cannot stand in are both refused.
            for (const float floor_top : { 1076.0f, 1100.0f, 1196.0f })
            {
                auto fixture = make(floor_top);
                throws_with([&] { fixture->build(); }, "full body height clear beneath its lowest pose");
            }
            {
                // A ceiling closing on the highest pose's rider.
                auto fixture = make(1200.0f);
                fixture->surface({ 500.0f, 600.0f, 320.0f, 90.0f });
                throws_with([&] { fixture->build(); }, "standing envelope intersects blocking geometry");
            }
            for (const float gap : { 10.0f, 51.0f })
            {
                // A ledge beside the shaft, closer than a body: a rider hanging
                // over the lift's edge would be carried into it.
                auto fixture = make(1200.0f);
                fixture->surface({ 720.0f + gap, 900.0f, 200.0f, 40.0f });
                throws_with([&] { fixture->build(); }, "body width clear beside its whole shaft");
            }
            {
                // A body width and a little more is enough.
                auto fixture = make(1200.0f);
                fixture->surface({ 720.0f + 53.0f, 900.0f, 200.0f, 40.0f });
                fixture->build();
            }
            {
                // A wall in the shaft.
                auto fixture = make(1200.0f);
                fixture->surface({ 700.0f, 950.0f, 40.0f, 40.0f });
                throws_with([&] { fixture->build(); }, "platform envelope intersects blocking geometry");
            }
            {
                // A clearance that leaves the scene.
                Fixture fixture(world, 1, { -1024.0f, -1024.0f, 8192.0f, 2150.0f });
                fixture.tuning = quick_tuning(world);
                auto& platform = fixture.mover(lift());
                fixture.add_player(platform.bounds().center().x, platform.bounds().top());
                throws_with([&] { fixture.build(); }, "clearance beneath its lowest pose leaves Scene bounds");
            }
            throws_with([&] { (void)world.mover(lift(), { true, true, true, true },
                CollisionObjectType::structure_jump_through); }, "solid paintable collision");
            {
                // E6: the lift's own travel joins the vertical budget. A step
                // nearly a body tall is refused though the player's own speed fits.
                auto fixture = std::make_unique<Fixture>(world);
                fixture->tuning = quick_tuning(world);
                // 2000 there and back in 0.4 s is about 167 a step, past the
                // 140 a body and a 20-thick lift allow; the shaft stays in the scene.
                auto spec = lift(); spec.travel_y = 2000.0f; spec.period_seconds = 0.4f;
                auto& platform = fixture->mover(spec);
                fixture->add_player(platform.bounds().center().x, platform.bounds().top());
                throws_with([&] { fixture->build(); }, "relative vertical displacement is unsafe");
            }
            std::cout << "PASS lift admission: clearance beneath the lowest pose (flush and short gaps refused), "
                "a body width beside the shaft, corridor, shaft, scene, solid-only and vertical relative-displacement refusals\n";
        }

        void check_carry(World& world)
        {
            Fixture fixture(world);
            fixture.tuning = quick_tuning(world);
            fixture.surface({ -200.0f, 1200.0f, 1600.0f, 200.0f });
            auto& platform = fixture.mover(lift());
            fixture.add_player(platform.bounds().center().x, platform.bounds().top());
            fixture.build(); fixture.start();
            // The lift leaves its top at 160 units/s, faster than a standing body
            // first falls, so the rider lands on it a few steps in.
            Player& rider = *fixture.players[0];
            for (int step = 0; step < 60 && rider.carrier_id() == 0; ++step) { fixture.tick(); }
            require(platform.motion_id() != 0 && rider.carrier_id() == platform.motion_id(),
                "A rider standing on a lift did not take it as its carrier");

            // Ride a whole round trip and a half: the rider's feet stay on the
            // lift's top on the way down and up, its x never moves, and the
            // lift's phase advances every step (the hold never fires).
            const float x = rider.rectangle().x;
            float lowest = feet(rider), highest = feet(rider);
            int down_steps = 0, up_steps = 0;
            double previous_phase = platform.motion_state().phase;
            for (int step = 0; step < 300; ++step)
            {
                const float before = platform.bounds().top();
                fixture.tick();
                const float after = platform.bounds().top();
                down_steps += after > before ? 1 : 0;
                up_steps += after < before ? 1 : 0;
                require(rider.carrier_id() == platform.motion_id(), "A lift dropped its standing rider");
                require(std::abs(feet(rider) - after) < 0.01f, "A rider's feet left the lift's top");
                require(rider.rectangle().x == x, "A lift carried its rider sideways");
                require(platform.motion_state().phase != previous_phase, "A ridden lift held its phase");
                previous_phase = platform.motion_state().phase;
                lowest = std::max(lowest, feet(rider)); highest = std::min(highest, feet(rider));
            }
            require(down_steps > 100 && up_steps > 100 && lowest > 1050.0f && highest < 806.0f,
                "A rider did not ride the lift's whole storey both ways");

            // Release adds no lift velocity: a jump from a rising lift leaves
            // with the same personal launch as a jump from still ground.
            for (int step = 0; step < 400 && !(platform.collision_step().after.y < platform.collision_step().before.y); ++step)
            { fixture.tick(); }
            require(platform.collision_step().after.y < platform.collision_step().before.y, "The lift never rose");
            fixture.tick(input(0.0f, true));
            const float lift_launch = rider.velocity().y;
            require(rider.carrier_id() == 0, "A jump did not release the lift's carry");

            Fixture still(world);
            still.tuning = quick_tuning(world);
            still.surface({ -200.0f, 1200.0f, 1600.0f, 200.0f });
            still.add_player(660.0f, 1200.0f);
            still.build(); still.start(); still.settle();
            still.tick(input(0.0f, true));
            require(bits(lift_launch) == bits(still.players[0]->velocity().y),
                "A jump from a rising lift kept some of the lift's velocity");
            std::cout << "PASS lift carry: a rider stays on the top through " << down_steps << " descending and " <<
                up_steps << " rising steps without x drift or a hold, and a jump leaves at the still-ground launch\n";
        }

        void check_world_queries(World& world)
        {
            const auto surface = world.mover(lift(0.25f));
            require(surface->translation() == Vector2F(0.0f, 128.0f), "The quarter-phase lift is not 128 down");
            require(surface->bounds() == RectangleF(600.0f, 928.0f, 120.0f, 20.0f) &&
                surface->shape()->bounding_box() == surface->bounds(), "A lift's collider did not ride with it");
            const RectangleF anchored = world.mover(lift(0.0f))->cull_bounds(1.0f);
            const RectangleF cull = surface->cull_bounds(1.0f);
            require(cull.x == anchored.x && cull.y == anchored.y + 128.0f && cull.width == anchored.width,
                "A lift's cull bounds did not ride with it");

            // Paint splashed on the translated underside lands on the anchor's bottom face.
            world.paint(*surface, { 640.0f, 944.0f, 20.0f, 8.0f }, PlayerTeam::a, 2);
            const auto local = snapshot(*surface);
            std::vector<PaintSurfaceSample> anchored_samples;
            surface->append_anchor_paint_surfaces(anchored_samples);
            require(local.size() == anchored_samples.size() && !local.empty(), "A lift's paint samples changed shape");
            bool painted = false;
            for (std::size_t index = 0; index < local.size(); ++index)
            {
                for (std::size_t corner = 0; corner < local[index].corners.size(); ++corner)
                {
                    require(local[index].corners[corner].x == anchored_samples[index].corners[corner].x &&
                        local[index].corners[corner].y == anchored_samples[index].corners[corner].y + 128.0f,
                        "A lift's world paint samples did not ride with it on y");
                }
                require(bits(local[index].scored_area) == bits(anchored_samples[index].scored_area),
                    "A lift's scored area changed with its pose");
                painted |= anchored_samples[index].team == PlayerTeam::a &&
                    anchored_samples[index].face == PaintSurfaceFace::bottom;
            }
            require(painted, "Paint on a translated lift's underside missed its anchored bottom face");
            require(surface->top_paint_team(660.0f) == PlayerTeam::none, "An unpainted lift top reported a team");

            TranslatedAirbrushShadow shadow(*surface, &world.render);
            const auto top = world.mover(lift(0.0f));
            TranslatedAirbrushShadow resting(*top, &world.render);
            require(shadow.bounds().x == resting.bounds().x && shadow.bounds().y == resting.bounds().y + 128.0f,
                "A lift's shadow did not ride with it");
            std::cout << "PASS lift world queries: collider, cull, shadow and paint samples ride on y; "
                "underside paint lands on the anchored bottom face with unchanged scored area\n";
        }
    }

    void check_lift(World& world)
    {
        check_axis();
        check_contract();
        check_admission(world);
        check_carry(world);
        check_world_queries(world);
    }
}
