#include "game/pch.h"
#include "scripts/acceptance/paint_underfoot/fixtures.h"
#include "engine/math/triangle.h"
#include <iostream>

namespace paint_underfoot_acceptance
{
    namespace
    {
        using namespace mattmath;
        void held(const Course& course, PlayerTeam team)
        { require(course.player->paint_underfoot_state().support_team == team, "Final foot-center support classification differs"); }
        void overlap(World& world, bool reverse, bool seam)
        {
            Course course(world);
            const RectangleF left = seam ? RectangleF(-100, 0, 100, 100) : RectangleF(-100, 0, 200, 100);
            const RectangleF right = seam ? RectangleF(0, 0, 100, 100) : left;
            for (int index = 0; index < 2; ++index)
            {
                const bool first = reverse ? index == 1 : index == 0;
                auto& surface = course.surface(first ? left : right);
                world.paint(surface, surface.shape()->bounding_box(), first ? PlayerTeam::a : PlayerTeam::b, first ? 0 : -1);
            }
            course.settle();
            held(course, seam ? PlayerTeam::b : PlayerTeam::none);
            course.tick(input(0.25f));
            require(course.player->paint_underfoot_state().step_team == (seam ? PlayerTeam::b : PlayerTeam::none),
                "Contact order, duplicate candidate or half-open seam changed the consumed support team");
        }
    }
    void check_support(World& world)
    {
        world.configure(true);
        overlap(world, false, false); overlap(world, true, false);
        overlap(world, false, true); overlap(world, true, true);
        {
            Course course(world); auto& floor = course.surface(); course.settle(); held(course, PlayerTeam::none);
            course.begin(input(1));
            world.paint(floor, floor.shape()->bounding_box(), PlayerTeam::a, 0);
            course.scene.resolve(); course.finish(); held(course, PlayerTeam::a);
            require(course.player->paint_underfoot_state().step_team == PlayerTeam::none,
                "New paint affected the movement/recovery step before contacts completed");
            course.tick(input(1));
            require(course.player->paint_underfoot_state().step_team == PlayerTeam::a,
                "Completed paint snapshot did not affect the following step");
            world.paint(floor, floor.shape()->bounding_box(), PlayerTeam::a, 8);
            require(floor.paint_total().player_areas[0] > 0 && floor.paint_total().player_areas[8] == 0,
                "Friendly-owner-retention fixture did not establish another player's actual paint credit");
            course.tick(); held(course, PlayerTeam::a);
            world.paint(floor, floor.shape()->bounding_box(), PlayerTeam::b, -1);
            course.tick(); held(course, PlayerTeam::b);
            require(course.player->paint_underfoot_state().step_team == PlayerTeam::a,
                "Enemy repaint bypassed the completed-contact latency");
            course.tick(); require(course.player->paint_underfoot_state().step_team == PlayerTeam::b,
                "Enemy uncredited paint was confused with neutral/friendly owner credit");
            course.player->kill(); held(course, PlayerTeam::none);
            require(course.player->state() == PlayerState::alive && course.player->health() <= 0,
                "Fatal-before-state control lost its actual boundary");
            course.tick(); held(course, PlayerTeam::none);
            course.player->force_respawn(); held(course, PlayerTeam::none);
            course.player->place_at({ 5000, -500 }); held(course, PlayerTeam::none);
        }
        {
            Course course(world, PlayerTeam::b, WeaponType::sprayer, 10.0f);
            auto& edge = course.surface({ -100, 0, 100, 100 });
            world.paint(edge, edge.shape()->bounding_box(), PlayerTeam::b, 8);
            course.settle();
            require(course.player->move_state() == PlayerMoveState::on_ground,
                "Overhang control must be actually body-edge supported");
            held(course, PlayerTeam::none);
        }
        for (bool clipped : { false, true })
        {
            Course course(world);
            const RectangleF cover(-10, -20, 20, 20);
            auto& floor = course.surface({ -100, 0, 200, 100 },
                clipped ? PaintableFaces{ false, true, false, false } : PaintableFaces{ true, false, true, true },
                clipped ? std::span<const RectangleF>(&cover, 1) : std::span<const RectangleF>{});
            world.paint(floor, floor.shape()->bounding_box(), PlayerTeam::a, 0);
            require(floor.paint_total().team_a > 0, "Excluded-top control did not contain actual side/exposed paint");
            course.settle(); held(course, PlayerTeam::none);
        }
        {
            Course course(world);
            auto& platform = course.surface({ -1000, 0, 2000, 20 }, { false, true, false, false }, {},
                CollisionObjectType::structure_jump_through);
            world.paint(platform, platform.shape()->bounding_box(), PlayerTeam::a, 0);
            course.settle(); held(course, PlayerTeam::a);
            course.tick(input(0, false, false, true));
            held(course, PlayerTeam::none);
            course.tick(); require(course.player->paint_underfoot_state().step_team == PlayerTeam::none,
                "Dropped-through platform retained paint effects in air");
        }
        {
            Course course(world); auto& floor = course.surface();
            world.paint(floor, floor.shape()->bounding_box(), PlayerTeam::a, 0);
            course.plain({ -1000, -world.tuning.player().size.y - 10.0f, 2000, 11.0f });
            course.tick();
            require(course.player->rectangle().bottom() > 0.0f, "Later ceiling-response control did not separate the final feet from paint");
            held(course, PlayerTeam::none);
        }
        {
            Course course(world, PlayerTeam::a, WeaponType::sprayer, -100.0f);
            course.plain({ -1000, 0, 1000, 100 });
            auto& own = course.surface({ 0, 0, 300, 100 }); world.paint(own, own.shape()->bounding_box(), PlayerTeam::a, 0);
            auto& enemy = course.surface({ 300, 0, 300, 100 }); world.paint(enemy, enemy.shape()->bounding_box(), PlayerTeam::b, -1);
            course.plain({ 600, 0, 10000, 100 }); course.settle();
            bool own_seen = false, enemy_seen = false, far_neutral = false;
            for (int tick = 0; tick < 200; ++tick)
            {
                const auto previous = course.player->paint_underfoot_state().support_team;
                course.tick(input(1));
                const auto state = course.player->paint_underfoot_state();
                require(state.step_team == previous, "Driven pass consumed an unfinished contact phase");
                own_seen |= state.support_team == PlayerTeam::a;
                enemy_seen |= state.support_team == PlayerTeam::b;
                far_neutral |= course.player->center().x > 700 && state.support_team == PlayerTeam::none;
            }
            require(own_seen && enemy_seen && far_neutral, "Actual driven pass did not cross neutral/own/enemy/neutral without teleports");
            bool back_own = false, back_enemy = false, back_neutral = false;
            for (int tick = 0; tick < 400 && !back_neutral; ++tick)
            {
                const auto previous = course.player->paint_underfoot_state().support_team;
                course.tick(input(-1));
                const auto state = course.player->paint_underfoot_state();
                require(state.step_team == previous, "Driven return consumed an unfinished contact phase");
                back_own |= state.support_team == PlayerTeam::a;
                back_enemy |= state.support_team == PlayerTeam::b;
                back_neutral = course.player->center().x < -100 && state.support_team == PlayerTeam::none;
            }
            require(back_own && back_enemy && back_neutral, "Driven return did not cross the same regions without a teleport");
        }
        {
            Course course(world);
            auto& floor = course.surface({ -8, 0, 16, 100 });
            world.paint(floor, { -3.9f, 0.1f, 0.5f, 0.5f }, PlayerTeam::a, 0);
            world.paint(floor, { 0.1f, 0.1f, 0.5f, 0.5f }, PlayerTeam::b, -1);
            require(floor.top_paint_team(-0.001f) == PlayerTeam::a && floor.top_paint_team(0.0f) == PlayerTeam::b,
                "Actual same-surface tile seam control did not establish opposite adjacent ownership");
            course.settle(); held(course, PlayerTeam::b);
        }
        {
            Course course(world, PlayerTeam::a, WeaponType::sprayer, 0, -20);
            auto& floor = course.surface({ -200, 100, 400, 100 });
            world.paint(floor, floor.shape()->bounding_box(), PlayerTeam::a, 0);
            const TriangleRightAxisAligned triangle(200, -100, -200, 100, 200, 100);
            course.scene.add(std::make_unique<Structure>("sprite_sheet_1", "square_white_4", triangle.bounding_box(),
                std::make_unique<TriangleRightAxisAligned>(triangle), &world.render, CollisionObjectType::structure_ramp_right));
            course.scene.end_tick();
            for (int tick = 0; tick < 30; ++tick) { course.tick(); }
            require(course.player->move_state() == PlayerMoveState::on_ramp_right,
                "Ramp exclusion control did not reach actual sloped support");
            held(course, PlayerTeam::none);
        }
        {
            Course course(world, PlayerTeam::a, WeaponType::sprayer, 0, -5);
            auto& floor = course.surface(); world.paint(floor, floor.shape()->bounding_box(), PlayerTeam::a, 0);
            course.tick(input(0, true));
            bool buffered = false;
            for (int tick = 0; tick < 12 && !buffered; ++tick)
            { course.tick(); buffered = course.player->move_state() == PlayerMoveState::jumping; }
            require(buffered, "Buffered-takeoff control did not land within its actual input window");
            held(course, PlayerTeam::none);
            course.tick(); require(course.player->paint_underfoot_state().step_team == PlayerTeam::none,
                "Buffered landing inherited persistent terrain context after takeoff");
        }
        {
            Course course(world); auto& floor = course.surface();
            world.paint(floor, floor.shape()->bounding_box(), PlayerTeam::a, 0); course.settle(); course.tick();
            const auto& details = world.tuning.projectile(ProjectileType::jet);
            require(details.details.player_damage > 0.0f, "Fatal-context control needs actual positive projectile damage");
            ProjectileJet hit(course.player->rectangle(), Vector2F::ZERO, PlayerTeam::b, 1, labrador::Colour::white,
                &world.render, resolve_projectile_strip(details, world.render), &details);
            for (int hit_count = 0; hit_count < 100 && course.player->health() > 0; ++hit_count)
            { course.player->on_contact(hit, Vector2F::ZERO, 0.0f); }
            require(course.player->health() <= 0.0f && course.player->state() == PlayerState::alive &&
                course.player->paint_underfoot_state().support_team == PlayerTeam::none &&
                course.player->paint_underfoot_state().step_team == PlayerTeam::none,
                "Actual fatal projectile did not clear context before the later dead state");
        }
        for (bool ceiling : { false, true })
        {
            Course course(world);
            const RectangleF bounds = ceiling ? RectangleF(-100, -130, 200, 11) : RectangleF(25, -300, 20, 600);
            auto& painted = course.surface(bounds, { true, true, true, true });
            world.paint(painted, bounds, PlayerTeam::a, 0);
            course.tick();
            held(course, PlayerTeam::none);
            require(course.player->paint_underfoot_state().step_team == PlayerTeam::none,
                "A painted wall or ceiling granted terrain effects without actual floor support");
        }
        for (int slot : { -1, 0, input_slots::keyboard_and_mouse })
        {
            Course course(world, PlayerTeam::b, WeaponType::sprayer, 0, 0, slot);
            auto& floor = course.surface(); world.paint(floor, floor.shape()->bounding_box(), PlayerTeam::b, 8);
            course.settle(); held(course, PlayerTeam::b);
            require(course.player->player_num() == slot, "Device/owner independence control lost the actual slot mapping");
        }
        std::cout << "PASS underfoot support: real contact ordering/seams/conflicts, delayed repaint, credit/palette independence and lifecycle/drop exclusions\n";
    }
}
