#include "game/pch.h"
#include "game/contracts/simulation.h"
#include "game/objects/fixed_slab_paintable.h"
#include "game/objects/player.h"
#include "game/objects/projectile_ball.h"
#include "game/objects/projectile_grenade.h"
#include "game/objects/projectile_wall.h"
#include "game/objects/round_shot_shape.h"
#include "game/objects/shot_retirements.h"
#include "game/objects/structure.h"
#include "game/objects/weapon_builder.h"
#include "game/config/presentation/presentation.h"
#include "game/config/play_settings.h"
#include "engine/math/circle.h"
#include "scripts/acceptance/support/content_world.h"
#include "scripts/acceptance/support/probe.h"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <memory>
#include <vector>

// ROAD#44: round shots (the bucket's ball, a sub's grenade and their drops)
// built as circles behind RoundShotShape, against plain cover, a player and
// a fixed slab, with no window, device or save. The shipped shape stays the
// rectangle; this probe sets circle only around its own shots and puts the
// rectangle back.

using namespace labrador;

namespace
{
    using acceptance::require;

    const float dt = simulation::seconds_per_step;

    bool close_to(float first, float second, float tolerance = 0.01f)
    {
        return std::isfinite(first) && std::abs(first - second) <= tolerance;
    }

    // Puts the shipped shape back however a check ends.
    struct ShapeScope
    {
        explicit ShapeScope(RoundShotShape shape) { set_round_shot_shape(shape); }
        ~ShapeScope() { set_round_shot_shape(RoundShotShape::rectangle); }
        ShapeScope(const ShapeScope&) = delete;
        ShapeScope& operator=(const ShapeScope&) = delete;
    };

    struct World
    {
        RenderResources render;
        AudioResources audio;
        PlaySettings preferences;
        GameTuning tuning;
        Presentation presentation;
        // A 40-unit ball flying straight, with time to spare: the sweep alone
        // decides where it stops.
        ProjectileTuning ball;

        explicit World(const std::filesystem::path& content) :
            tuning((content / "tuning.json").string().c_str()),
            presentation((content / "presentation.json").string().c_str()),
            ball(tuning.projectile(ProjectileType::ball))
        {
            acceptance::load_cpu_assets(content, this->render, this->audio);
            this->ball.details.col_rect_size = { 40.0f, 40.0f };
            this->ball.details.gravity = this->ball.details.wind_resistance = 0.0f;
            this->ball.details.delete_timer = 10.0f;
            this->ball.max_velocity = { 1.0e6f, 1.0e6f };
        }

        // Centred at centre, travelling `travel` in its first step.
        std::unique_ptr<ProjectileBall> shot(const Vector2F& centre, const Vector2F& travel,
            RoundShotShape shape)
        {
            return std::make_unique<ProjectileBall>(RectangleF(centre, this->ball.details.col_rect_size),
                travel / dt, PlayerTeam::a, 0, Colour::blue, &this->render,
                resolve_projectile_strip(this->ball, this->render), &this->ball,
                0.0f, Vector2F::zero, SpriteFlip::none, 0.0f, shape);
        }

        std::unique_ptr<Structure> cover(const RectangleF& rectangle)
        {
            return std::make_unique<Structure>("sprite_sheet_1", "square_white_4", rectangle,
                std::make_unique<RectangleF>(rectangle), &this->render, CollisionObjectType::structure);
        }

        std::unique_ptr<Player> player(const Vector2F& top_left)
        {
            const RectangleF rectangle(top_left, this->tuning.player().size);
            auto weapon = WeaponBuilder::build_weapon(&this->tuning, PlayerTeam::b, 1, 0,
                Colour::red, WeaponType::sniper, rectangle.center(), &this->render,
                &this->audio, &this->preferences);
            return std::make_unique<Player>(rectangle, &this->tuning,
                &this->presentation.movement_feedback(), &this->presentation.combat_feedback(),
                &this->presentation.respawn_redraw(), &this->render, &this->audio,
                &this->preferences, 1, PlayerTeam::b, WeaponType::sniper, std::move(weapon),
                WeaponType::none, Colour::red, Viewport{}, Vector2F::zero);
        }
    };

    // One step of flight, then the sweep against `targets`, as Level runs it.
    void fly(Projectile& shot, std::vector<CollisionObject*> targets)
    {
        shot.update(dt);
        shot.resolve_movement_contacts(targets);
    }

    void check_default_and_shape(World& world)
    {
        require(round_shot_shape() == RoundShotShape::rectangle, "Round shots are not rectangles by default");
        const auto shipped = std::make_unique<ProjectileBall>(RectangleF(Vector2F(10.0f, 20.0f),
            world.ball.details.col_rect_size), Vector2F::zero, PlayerTeam::a, 0, Colour::blue,
            &world.render, resolve_projectile_strip(world.ball, world.render), &world.ball);
        require(shipped->round_shape() == RoundShotShape::rectangle &&
            shipped->shape()->shape_type() == ShapeType::rectangle,
            "A ball built by default does not collide as its rectangle");

        auto ball = world.shot({ 10.0f, 20.0f }, { 30.0f, -12.0f }, RoundShotShape::circle);
        require(ball->shape()->shape_type() == ShapeType::circle, "A circle ball does not collide as a circle");
        const auto& circle = static_cast<const Circle&>(*ball->shape());
        require(close_to(circle.radius(), 20.0f) && close_to(circle.center().x, 10.0f) && close_to(circle.center().y, 20.0f),
            "A circle ball is not the disc inscribed in its collision square");
        require(ball->bounds().width == 40.0f && ball->bounds().height == 40.0f,
            "A circle ball changed its drawn and painted square");
        ball->update(dt);
        require(close_to(circle.center().x, 40.0f) && close_to(circle.center().y, 8.0f) &&
            close_to(circle.center().x, ball->bounds().center().x),
            "A circle ball's disc does not follow its flight");
        {
            const ShapeScope scope(RoundShotShape::circle);
            const auto global = std::make_unique<ProjectileBall>(RectangleF(Vector2F::zero,
                world.ball.details.col_rect_size), Vector2F::zero, PlayerTeam::a, 0, Colour::blue,
                &world.render, resolve_projectile_strip(world.ball, world.render), &world.ball);
            require(global->round_shape() == RoundShotShape::circle,
                "A ball built under the circle setting is not a circle");
        }
        require(round_shot_shape() == RoundShotShape::rectangle, "The probe left the circle setting on");
        std::cout << "PASS shape: rectangle by default; a circle is the inscribed disc and follows the flight\n";
    }

    void check_face_hit(World& world)
    {
        // Head on into a wall's face: the same stop in both shapes.
        float stopped[2]{};
        for (const RoundShotShape shape : { RoundShotShape::rectangle, RoundShotShape::circle })
        {
            auto wall = world.cover(RectangleF(100.0f, -200.0f, 50.0f, 400.0f));
            auto ball = world.shot({ 0.0f, 0.0f }, { 160.0f, 0.0f }, shape);
            fly(*ball, { wall.get() });
            require(ball->for_deletion() && ball->retire_cause() == ShotRetireCause::cover,
                "A ball flying into a wall's face did not stop on it");
            stopped[shape == RoundShotShape::circle ? 1 : 0] = ball->bounds().center().x;
            ShotRetirements tally;
            tally.count(*ball, false);
            require(tally.ball.cover == 1 && tally.ball.total() == 1 && tally.grenade.total() == 0,
                "A ball stopped by cover was not tallied as a cover retirement");
        }
        require(close_to(stopped[0], stopped[1]) && stopped[0] > 79.0f && stopped[0] < 81.0f,
            "A face hit stopped the circle and the rectangle in different places");

        // A fast circle against a thin wall still stops (no tunnelling).
        auto thin = world.cover(RectangleF(100.0f, -200.0f, 2.0f, 400.0f));
        auto fast = world.shot({ 0.0f, 0.0f }, { 600.0f, 0.0f }, RoundShotShape::circle);
        fly(*fast, { thin.get() });
        require(fast->for_deletion() && fast->bounds().center().x < 100.0f,
            "A fast circle ball passed through a thin wall");
        std::cout << "PASS face: a head-on hit stops both shapes at the same face; a fast circle does not tunnel\n";
    }

    // A diagonal path past a wall's corner, 24 units from it: the 40-unit
    // square clips the corner, the 20-unit disc clears it.
    void check_corner_graze(World& world)
    {
        const Vector2F start(40.0f, 126.0f), finish(126.0f, 40.0f);
        for (const RoundShotShape shape : { RoundShotShape::rectangle, RoundShotShape::circle })
        {
            auto wall = world.cover(RectangleF(100.0f, 100.0f, 200.0f, 200.0f));
            auto ball = world.shot(start, finish - start, shape);
            fly(*ball, { wall.get() });
            if (shape == RoundShotShape::rectangle)
            {
                require(ball->for_deletion() && ball->retire_cause() == ShotRetireCause::cover,
                    "A rectangle ball clipping a corner did not stop on it");
            }
            else
            {
                require(!ball->for_deletion() && ball->retire_cause() == ShotRetireCause::none &&
                    close_to(ball->bounds().center().x, finish.x) && close_to(ball->bounds().center().y, finish.y),
                    "A circle ball clearing a corner stopped or did not fly its whole step");
            }
        }
        // The same graze past a player's corner.
        for (const RoundShotShape shape : { RoundShotShape::rectangle, RoundShotShape::circle })
        {
            auto body = world.player({ 100.0f, 100.0f });
            auto ball = world.shot(start, finish - start, shape);
            fly(*ball, { body.get() });
            const bool hit = ball->for_deletion() && ball->retire_cause() == ShotRetireCause::player;
            require(shape == RoundShotShape::rectangle ? hit : !hit && !ball->for_deletion(),
                "A corner graze past a player was not a hit for the square and a miss for the disc");
            if (shape == RoundShotShape::rectangle)
            {
                ShotRetirements tally;
                tally.count(*ball, false);
                require(tally.ball.player == 1 && tally.ball.total() == 1,
                    "A ball that hit a player was not tallied as a player retirement");
            }
        }
        std::cout << "PASS corner: a path 24 units off a corner hits as a square and clears as a disc, "
            "past cover and past a player\n";
    }

    // Paths that just miss or only just reach a wall's corner, far from the origin where
    // float steps are coarse, with a thin second wall further along: however
    // the corner graze resolves, the disc never ends its step past the second
    // wall. One the narrow phase cannot confirm at the corner is still swept
    // against the rest of the step.
    void check_graze_then_wall(World& world)
    {
        const float r = 20.0f;
        const Vector2F corner(5000.0f, 5000.0f);
        const Vector2F along = Vector2F(1.0f, -1.0f) / std::sqrt(2.0f);
        const Vector2F away = Vector2F(-1.0f, -1.0f) / std::sqrt(2.0f);
        int stopped_on_corner = 0;
        for (int k = 0; k < 400; ++k)
        {
            // From a clear miss to a 1e-3 overlap, through the float step there.
            const float depth = 5.0e-5f * static_cast<float>(k % 40 - 20);
            const Vector2F closest = corner + away * (r - depth) + along * (0.01f * static_cast<float>(k / 40));
            const Vector2F start = closest - along * 60.0f;
            const Vector2F finish = closest + along * 150.0f;
            auto first = world.cover(RectangleF(5000.0f, 5000.0f, 200.0f, 200.0f));
            auto second = world.cover(RectangleF(5020.0f, 4800.0f, 2.0f, 175.0f));
            auto ball = world.shot(start, finish - start, RoundShotShape::circle);
            fly(*ball, { first.get(), second.get() });
            require(ball->for_deletion() && ball->retire_cause() == ShotRetireCause::cover &&
                ball->bounds().center().x < 5020.0f,
                "A circle ball that grazed a corner flew on through the wall behind it");
            if (ball->bounds().center().x < 4995.0f) { ++stopped_on_corner; }
        }
        std::cout << "PASS graze: 400 paths just past or into a far corner stop on it or on the wall behind it ("
            << stopped_on_corner << " on the corner)\n";
    }

    // A planted sub is a ball by kind but not a round shot, so the tally
    // leaves it out however it retires.
    void check_planted_not_tallied(World& world)
    {
        const SubTuning& sub = world.tuning.sub(SubType::wall);
        auto wall = std::make_unique<ProjectileWall>(RectangleF(Vector2F::zero, sub.body.details.col_rect_size),
            Vector2F::zero, PlayerTeam::a, 0, Colour::blue, &world.render,
            resolve_projectile_strip(sub.body, world.render), resolve_projectile_strip(sub.burst, world.render), &sub);
        wall->set_for_deletion(true);
        ShotRetirements tally;
        tally.count(*wall, false);
        tally.count(*wall, true);
        require(tally.ball.total() == 0 && tally.grenade.total() == 0,
            "A planted sub was tallied as a round shot");
        std::cout << "PASS planted: a paint wall is not tallied as a round shot\n";
    }

    void check_grenade(World& world)
    {
        const SubTuning& sub = world.tuning.sub(SubType::grenade);
        for (const RoundShotShape shape : { RoundShotShape::rectangle, RoundShotShape::circle })
        {
            auto grenade = std::make_unique<ProjectileGrenade>(RectangleF(Vector2F(0.0f, 0.0f),
                sub.body.details.col_rect_size), Vector2F(100.0f, 0.0f), PlayerTeam::a, 0, Colour::blue,
                &world.render, resolve_projectile_strip(sub.body, world.render), &sub.body,
                resolve_projectile_strip(sub.burst, world.render), &sub.burst,
                sub.burst_count, sub.burst_speed, shape);
            const ShapeType type = shape == RoundShotShape::circle ? ShapeType::circle : ShapeType::rectangle;
            require(grenade->shape()->shape_type() == type, "A grenade does not collide as its shape");
            grenade->set_for_deletion(true);
            ShotRetirements tally;
            tally.count(*grenade, false);
            require(tally.grenade.expired == 1 && tally.ball.total() == 0,
                "A grenade retired with no contact was not tallied as an expired grenade");
            // Built under the other setting: the drops still take the grenade's.
            const ShapeScope other(shape == RoundShotShape::circle ? RoundShotShape::rectangle : RoundShotShape::circle);
            const auto drops = grenade->take_burst();
            require(!drops.empty(), "The grenade released no drops");
            for (const auto& drop : drops)
            {
                require(drop->shape()->shape_type() == type, "A grenade's drop does not take the grenade's shape");
            }
        }
        std::cout << "PASS grenade: its body and every drop collide as the shape it was built with\n";
    }

    void check_slab(World& world)
    {
        auto slab = std::make_unique<FixedSlabPaintable>(FixedSlabSpec{ { 0.0f, 80.0f }, FixedSlabRise::right },
            &world.render, &world.tuning.paint(), TeamColour{}, PaintableFaces{ true, true, true, true });
        const RectangleF box = slab->shape()->bounding_box();
        auto ball = world.shot({ box.center().x, box.top() - 60.0f }, { 0.0f, 120.0f + box.height },
            RoundShotShape::circle);
        fly(*ball, { slab.get() });
        require(ball->for_deletion() && ball->retire_cause() == ShotRetireCause::cover,
            "A circle ball falling onto a fixed slab did not stop on it");
        // A circle's paint footprint on a slab is its square, as everywhere.
        slab->on_contact(*ball, Vector2F::zero, 0.0f);
        std::cout << "PASS slab: a circle ball stops on a fixed slab and paints it without refusal\n";
    }
}

int main(int argc, char** argv)
{
    return acceptance::run_probe("round shots", [&]
    {
        require(argc == 2, "Usage: round_shots.exe <game/content directory>");
        World world{ std::filesystem::path(argv[1]) };
        check_default_and_shape(world);
        check_face_hit(world);
        check_corner_graze(world);
        check_graze_then_wall(world);
        check_planted_not_tallied(world);
        check_grenade(world);
        check_slab(world);
        require(round_shot_shape() == RoundShotShape::rectangle, "The probe left the circle setting on");
        return 0;
    });
}
