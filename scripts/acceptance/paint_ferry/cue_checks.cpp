#include "game/pch.h"
#include "scripts/acceptance/moving_structure/fixtures.h"
#include <iostream>

namespace
{
    using namespace labrador;
    using namespace mattmath;
    using namespace moving_structure_acceptance;

    bool contains(const RectangleF& outer, const RectangleF& inner)
    {
        return inner.left() >= outer.left() && inner.right() <= outer.right() &&
            inner.top() >= outer.top() && inner.bottom() <= outer.bottom();
    }

    void check_rounded_world_hold(World& world)
    {
        // At one million units, a world X has 0.0625-unit float spacing.
        // This admitted one-unit/second ferry advances its local offset by
        // about 0.0167, below half that spacing. The per-step safety budget
        // still has ample room inside the ordinary player's body width.
        HorizontalMotionSpec spec{ { 1000000.0f, 500.0f, 64.0f, 20.0f }, 64.0f, 128.0f, 0.25f };
        spec.drive = HorizontalDrive::paint;
        Fixture fixture(world, 1, { 999000.0f, -1024.0f, 8192.0f, 4096.0f });
        fixture.plain({ 999000.0f, 1200.0f, 4000.0f, 80.0f });
        auto& mover = fixture.mover(spec);
        fixture.add_player(1000200.0f, 1200.0f); fixture.build(); fixture.start();
        RectangleF splash = mover.bounds(); splash.inflate(24.0f);
        world.paint(mover, splash, PlayerTeam::a, 0);
        const float local = mover.translation().x;
        fixture.tick();
        require(mover.translation().x > local && mover.collision_step().before.x == mover.collision_step().after.x,
            "Rounded ferry fixture must advance locally while holding its world collision pose");
        require(mover.cue_state().committed_heading == 1 && mover.cue_state().movement_heading == 0 &&
            mover.cue_state().motion == FerryMotionCue::held,
            "A rounded-stationary ferry falsely reported actual world movement");
    }

    void check_heading(World& world, int a_heading)
    {
        HorizontalMotionSpec spec{ { 512.0f, 500.0f, 320.0f, 20.0f }, 120.0f, 1.0f, 0.25f };
        spec.drive = HorizontalDrive::paint; spec.team_a_heading = a_heading;
        Fixture fixture(world), control(world);
        const auto add = [&](Fixture& target, const FerryDirectionPresentation* look)
        {
            target.plain({ -512.0f, 1200.0f, 4000.0f, 80.0f });
            auto* mover = target.scene->add(std::make_unique<TranslatingStructurePaintable>(
                "sprite_sheet_1", "square_white_4", spec, &world.render, &world.audio,
                &world.preferences, &world.tuning.paint(), CollisionObjectType::structure_jump_through,
                world.palette, PaintableFaces{ true, true, true, true }, Colour::white,
                Vector2F::ZERO, SpriteFlip::none, 0.0f, nullptr, look));
            target.add_player(100.0f, 1200.0f); target.build();
            return mover;
        };
        auto& cued = *add(fixture, &world.presentation.ferry_direction());
        auto& plain = *add(control, nullptr);
        require(cued.cue_state().paint_driven && cued.cue_state().motion == FerryMotionCue::unstepped &&
            cued.cue_state().demand_team == PlayerTeam::none && cued.cue_state().committed_heading == 0,
            "An initial ferry cue claimed a committed movement");
        require(plain.cue_bounds() == RectangleF::ZERO, "An uncued ferry gained a plaque");
        fixture.start(); control.start();
        const auto unchanged = [&]()
        {
            require(motion_bits(cued.motion_state()) == motion_bits(plain.motion_state()) &&
                step_bits(cued.collision_step()) == step_bits(plain.collision_step()),
                "Ferry cue changed the exact committed motion/collision trace");
            require(sample_bits(snapshot(cued)) == sample_bits(snapshot(plain)),
                "Ferry cue changed exact paint geometry, team or participant credit");
            same_total(cued.paint_total(), plain.paint_total());
            const RectangleF plaque = cued.cue_bounds();
            require(plaque.top() >= cued.bounds().bottom() && plaque.width <= cued.bounds().width,
                "Ferry plaque obscured its own paintable faces or grew beyond its body width");
            for (const float units : { 0.5f, 1.0f, 2.0f, 4.0f, 16.0f })
            { require(contains(cued.cull_bounds(units), plaque), "Ferry plaque escaped declared cull bounds"); }
        };
        const auto tick = [&]() { fixture.tick(); control.tick(); unchanged(); };
        const auto paint = [&](PlayerTeam team, bool half = false)
        {
            for (TranslatingStructurePaintable* mover : { &cued, &plain })
            {
                RectangleF splash = mover->bounds(); splash.inflate(24.0f);
                if (half)
                {
                    splash.x = mover->bounds().center().x + 0.25f;
                    splash.width = mover->bounds().right() + 24.0f - splash.x;
                }
                world.paint(*mover, splash, team, team == PlayerTeam::a ? 0 : 1);
            }
            unchanged();
        };
        tick();
        require(cued.cue_state().motion == FerryMotionCue::held && cued.cue_state().movement_heading == 0,
            "An unpainted committed ferry did not show a hold");
        paint(PlayerTeam::a);
        const FerryCueState late_a = cued.cue_state();
        require(late_a.demand_team == PlayerTeam::a && late_a.demand_heading == a_heading &&
            late_a.committed_heading == 0 && late_a.motion == FerryMotionCue::held,
            "New paint rewrote a ferry's last committed hold");
        tick();
        require(cued.cue_state().motion == FerryMotionCue::moving &&
            cued.cue_state().committed_heading == a_heading && cued.cue_state().movement_heading == a_heading,
            "A ferry did not separate its committed motion from current demand");
        paint(PlayerTeam::b);
        const FerryCueState late_b = cued.cue_state();
        require(late_b.demand_team == PlayerTeam::b && late_b.demand_heading == -a_heading &&
            late_b.committed_heading == a_heading && late_b.movement_heading == a_heading,
            "A repaint claimed reversal before a motion step committed");
        fixture.tick(std::vector<PlayerInputData>{}, 0.0f);
        control.tick(std::vector<PlayerInputData>{}, 0.0f); unchanged();
        require(cued.cue_state().committed_heading == a_heading && cued.cue_state().motion == FerryMotionCue::moving,
            "A zero update rewrote the last positive committed ferry step");
        tick();
        require(cued.cue_state().committed_heading == -a_heading && cued.cue_state().movement_heading == -a_heading,
            "A ferry failed to publish the committed repaint reversal");
        for (int index = 0; index < 40; ++index) { tick(); }
        const FerryMotionCue b_stop = a_heading > 0 ? FerryMotionCue::left_stop : FerryMotionCue::right_stop;
        require(cued.cue_state().motion == b_stop && cued.cue_state().movement_heading == 0,
            "A ferry did not distinguish team B's reached end stop from a tie");
        paint(PlayerTeam::a); paint(PlayerTeam::b, true);
        require(cued.paint_total().team_a == cued.paint_total().team_b &&
            cued.cue_state().demand_team == PlayerTeam::none && cued.cue_state().demand_heading == 0 &&
            cued.cue_state().motion == b_stop, "Tied paint erased a prior end stop before settlement");
        tick();
        require(cued.cue_state().motion == FerryMotionCue::held && cued.cue_state().committed_heading == 0,
            "A committed tie at an endpoint still claimed end-stop pressure");
        paint(PlayerTeam::a);
        for (int index = 0; index < 40; ++index) { tick(); }
        require(cued.cue_state().motion == (a_heading > 0 ? FerryMotionCue::right_stop : FerryMotionCue::left_stop),
            "A ferry did not publish the opposite reached end stop");
    }
}

void check_ferry_cue(const std::filesystem::path& content)
{
    using namespace moving_structure_acceptance;
    World world(content);
    check_heading(world, 1); check_heading(world, -1);
    check_rounded_world_hold(world);
    HorizontalMotionSpec cycle{ { 512.0f, 500.0f, 320.0f, 20.0f }, 120.0f, 1.0f, 0.25f };
    TranslatingStructurePaintable cycling("sprite_sheet_1", "square_white_4", cycle,
        &world.render, &world.audio, &world.preferences, &world.tuning.paint(),
        CollisionObjectType::structure_jump_through, world.palette, { true, true, true, true },
        labrador::Colour::white, mattmath::Vector2F::ZERO, labrador::SpriteFlip::none, 0.0f,
        nullptr, &world.presentation.ferry_direction());
    require(!cycling.cue_state().paint_driven && cycling.cue_bounds() == mattmath::RectangleF::ZERO,
        "A cyclic mover falsely displayed paint-driven directions");
    std::cout << "PASS ferry cue CPU: both team-A headings, current paint vs committed movement/zero updates, "
        "tie and endpoint holds, no cycle cue, contained translated cull and exact uncued motion/paint/credit controls\n";
}
