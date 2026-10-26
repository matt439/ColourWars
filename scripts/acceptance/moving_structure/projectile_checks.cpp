#include "game/pch.h"
#include "scripts/acceptance/moving_structure/fixtures.h"
#include "game/objects/projectile_spray.h"
#include "game/objects/projectile_mist.h"
#include "game/objects/projectile_ball.h"
#include "game/objects/projectile_rolling.h"

namespace moving_structure_acceptance
{
    namespace
    {
        using namespace mattmath;
        using namespace labrador;
        constexpr int source_owner = 5;
        constexpr int repaint_owner = 6;

        ProjectileTuning isolated_shot(World& world, ProjectileType type,
            float width = 2.0f, float height = 2.0f)
        {
            ProjectileTuning values = world.tuning.projectile(type);
            values.details.col_rect_size = { width, height };
            values.details.gravity = 0.0f;
            values.details.wind_resistance = 0.0f;
            values.max_velocity = { 10000.0f, 10000.0f };
            values.diffusion.type = DiffusionType::no_diffusion;
            return values;
        }

        std::unique_ptr<Projectile> shot(World& world, const ProjectileTuning& values,
            ProjectileType type, RectangleF desired, const Vector2F& velocity)
        {
            // Real constructors offset by their authored collider half-size.
            // The caller specifies the actual starting collision rectangle.
            desired.offset(values.details.col_rect_size / 2.0f);
            const auto strip = resolve_projectile_strip(values, world.render);
            const auto colour = world.palette.team_colour(PlayerTeam::a);
            switch (type)
            {
            case ProjectileType::spray:
                return std::make_unique<ProjectileSpray>(desired, velocity, PlayerTeam::a,
                    source_owner, colour, &world.render, strip, &values);
            case ProjectileType::jet:
                return std::make_unique<ProjectileJet>(desired, velocity, PlayerTeam::a,
                    source_owner, colour, &world.render, strip, &values);
            case ProjectileType::mist:
                return std::make_unique<ProjectileMist>(desired, velocity, PlayerTeam::a,
                    source_owner, colour, &world.render, strip, &values);
            case ProjectileType::ball:
                return std::make_unique<ProjectileBall>(desired, velocity, PlayerTeam::a,
                    source_owner, colour, &world.render, strip, &values);
            case ProjectileType::rolling:
                return std::make_unique<ProjectileRolling>(desired, velocity, PlayerTeam::a,
                    source_owner, colour, &world.render, strip, &values);
            case ProjectileType::count:
                break;
            }
            throw std::runtime_error("Unknown actual moving projectile kind");
        }

        std::unique_ptr<Fixture> course(World& world, const HorizontalMotionSpec& spec,
            const PaintableFaces& faces = { true, true, true, true })
        {
            auto result = std::make_unique<Fixture>(world);
            result->plain({ -500.0f, 600.0f, 1000.0f, 80.0f });
            result->add_player(-200.0f, 600.0f);
            result->mover(spec, faces);
            result->build(); result->start();
            return result;
        }

        void external_step(Fixture& fixture, Projectile& projectile)
        {
            // Actual Level prepares/commits motion; the independently owned
            // concrete shot then uses that complete step and live target span.
            // Owning it here allows inspection after a flying hit retires it.
            fixture.tick(); projectile.update(dt);
            projectile.resolve_movement_contacts(fixture.scene->collision_objects());
        }

        void check_crossings(World& world)
        {
            for (ProjectileType type : { ProjectileType::spray, ProjectileType::jet,
                ProjectileType::mist, ProjectileType::ball })
            {
                const ProjectileTuning values = isolated_shot(world, type);
                auto fixture = course(world, { { 100.0f, 300.0f, 16.0f, 100.0f }, 12.0f, 1.0f, 0.25f });
                auto& moving = *fixture->movers.front();
                const RectangleF before = moving.bounds();
                auto projectile = shot(world, values, type,
                    { before.left() - 24.0f, 340.0f, 2.0f, 2.0f }, { 3840.0f, 0.0f });
                const RectangleF start = projectile->shape()->bounding_box();
                require(!start.intersects(before), "Flying crossing must start clear of moving cover");
                external_step(*fixture, *projectile);
                require(projectile->for_deletion(), "Actual flying kind missed relative moving cover");
                require(moving.paint_total().player_areas.at(source_owner) > 0.0f,
                    "Moving impact lost original source paint owner");
                require(!projectile->take_blocked_contact(), "Wet moving hit was misclassified as blocked bare cover");
                const float unblocked_finish_left = start.left() + 3840.0f * dt;
                require(unblocked_finish_left > moving.bounds().right(),
                    "Flying crossing must have an independently clear unswept endpoint");
            }
        }

        void check_reversal_and_miss(World& world)
        {
            for (bool miss : { false, true })
            {
                const ProjectileTuning values = isolated_shot(world, ProjectileType::jet, 0.025f, 0.025f);
                auto fixture = course(world, { { 100.0f, 300.0f, 16.0f, 100.0f }, 12.0f, 1.0f, 0.475f });
                auto& moving = *fixture->movers.front();
                const RectangleF before = moving.bounds();
                const float peak_right = 128.0f; // anchor.right + authored travel, independent of net step.
                require(peak_right - before.right() > 0.1f,
                    "Reversal fixture did not approach the authored extremum from below");
                const float x = (before.right() + peak_right) / 2.0f;
                auto projectile = shot(world, values, ProjectileType::jet,
                    { x, miss ? 290.0f : 350.0f, 0.025f, 0.025f }, Vector2F::ZERO);
                const RectangleF volume = projectile->shape()->bounding_box();
                require(!volume.intersects(before), "Stationary reversal shot must start clear");
                external_step(*fixture, *projectile);
                require(moving.collision_step().segment_count == 2,
                    "Actual moving step must reverse within this one update");
                require(!volume.intersects(moving.bounds()), "Stationary reversal shot must finish clear");
                require(projectile->for_deletion() != miss,
                    "Piecewise moving sweep lost an interior hit or invented a vertical near miss");
                require((moving.paint_total().player_areas.at(source_owner) > 0.0f) != miss,
                    "Reversal face paint did not match actual relative contact");
            }
        }

        void check_zero_and_rearming(World& world)
        {
            const ProjectileTuning values = isolated_shot(world, ProjectileType::jet, 0.1f, 0.1f);
            auto fixture = course(world, { { 100.0f, 300.0f, 16.0f, 100.0f }, 12.0f, 1.0f, 0.25f });
            auto& moving = *fixture->movers.front();
            const float x = moving.bounds().right() + 0.6f;
            auto projectile = shot(world, values, ProjectileType::jet,
                { x, 350.0f, 0.1f, 0.1f }, Vector2F::ZERO);
            external_step(*fixture, *projectile);
            require(!projectile->for_deletion(), "Readiness miss control unexpectedly contacted moving cover");
            const auto history = motion_bits(moving.motion_state());
            for (int repeat = 0; repeat < 3; ++repeat)
            {
                fixture->tick(input(), 0.0f);
                projectile->resolve_movement_contacts(fixture->scene->collision_objects());
                const auto zero = moving.collision_step();
                require(zero.segment_count == 0 && zero.before == zero.after &&
                    motion_bits(moving.motion_state()) == history,
                    "Zero step reused moving segments or changed persisted history");
                require(!projectile->for_deletion() && moving.paint_total().team_a == 0.0f,
                    "Zero or repeated full-span resolution rearmed moving projectile contact");
            }
            external_step(*fixture, *projectile);
            require(projectile->for_deletion() && moving.paint_total().player_areas.at(source_owner) > 0.0f,
                "Next positive actual projectile update did not rearm the relative-only hit");
        }

        void check_earliest_and_cover_tie(World& world)
        {
            for (bool barrier_first : { false, true })
            {
                const ProjectileTuning values = isolated_shot(world, ProjectileType::jet);
                Fixture fixture(world);
                fixture.plain({ -500.0f, 600.0f, 1000.0f, 80.0f });
                fixture.add_player(-200.0f, 600.0f);
                auto& moving = fixture.mover({ { 100.0f, 300.0f, 16.0f, 100.0f }, 12.0f, 1.0f, 0.25f });
                auto& barrier = fixture.surface({ barrier_first ? 96.0f : 132.0f, 300.0f, 1.0f, 100.0f });
                fixture.build(); fixture.start();
                auto projectile = shot(world, values, ProjectileType::jet,
                    { moving.bounds().left() - 24.0f, 340.0f, 2.0f, 2.0f }, { 3840.0f, 0.0f });
                external_step(fixture, *projectile);
                require(projectile->for_deletion(), "Earliest obstruction control did not make a real hit");
                require((barrier.paint_total().player_areas.at(source_owner) > 0.0f) == barrier_first &&
                    (moving.paint_total().player_areas.at(source_owner) > 0.0f) != barrier_first,
                    "Moving/static sweep did not preserve global earliest obstruction");
            }
            for (bool reverse : { false, true })
            {
                const ProjectileTuning values = isolated_shot(world, ProjectileType::jet);
                Fixture fixture(world, 2);
                fixture.plain({ -500.0f, 600.0f, 1000.0f, 80.0f });
                fixture.add_player(-200.0f, 600.0f);
                auto& enemy = fixture.add_player(-300.0f, 600.0f, PlayerTeam::b);
                auto& moving = fixture.mover({ { 100.0f, 300.0f, 16.0f, 100.0f }, 2.0f, 1.0e20f, 0.25f });
                fixture.build(); fixture.start(); fixture.tick();
                const auto step = moving.collision_step();
                require(step.segment_count == 1 && step.segments[0].from_x == step.segments[0].to_x,
                    "Positive equal-offset segment control was not established");
                enemy.place_at({ moving.bounds().left(), 320.0f });
                const float health = enemy.health();
                auto projectile = shot(world, values, ProjectileType::jet,
                    { moving.bounds().left() - 6.0f, 340.0f, 2.0f, 2.0f }, { 16.0f / dt, 0.0f });
                const float start_left = projectile->shape()->bounding_box().left();
                projectile->update(dt);
                require(projectile->shape()->bounding_box().left() - start_left == 16.0f,
                    "Cover tie requires exact binary16 projectile displacement");
                std::vector<CollisionObject*> targets(fixture.scene->collision_objects().begin(),
                    fixture.scene->collision_objects().end());
                if (reverse) { std::reverse(targets.begin(), targets.end()); }
                projectile->resolve_movement_contacts(targets);
                require(projectile->for_deletion() && bits(enemy.health()) == bits(health) &&
                    moving.paint_total().player_areas.at(source_owner) > 0.0f,
                    "Moving cover must win tied entry against an enemy in either target order");
            }
        }

        void check_diffusion_and_current_cue(World& world)
        {
            ProjectileTuning values = isolated_shot(world, ProjectileType::mist);
            values.diffusion.type = DiffusionType::linear_size_and_col_rect;
            values.diffusion.start_time = 0.0f; values.diffusion.end_time = dt;
            values.diffusion.end_scale = { 3.0f, 3.0f };
            auto fixture = course(world, { { 1000.125f, 350.0f, 16.0f, 0.125f }, 2.0f, 1.0e20f, 0.25f });
            auto& moving = *fixture->movers.front();
            auto projectile = shot(world, values, ProjectileType::mist,
                { moving.bounds().left() - 10.0f, 349.05f, 2.0f, 2.0f }, { 16.0f / dt, 0.0f });
            const RectangleF start = projectile->shape()->bounding_box();
            fixture->tick(); projectile->update(dt);
            const RectangleF finish = projectile->shape()->bounding_box();
            require(finish.width > start.width + 3.0f, "Actual Mister collider did not diffuse in this control");
            const double entry = (static_cast<double>(moving.bounds().left()) - start.right()) /
                (static_cast<double>(finish.right()) - start.right());
            const double edge_travel = std::max({ std::abs(static_cast<double>(finish.left()) - start.left()),
                std::abs(static_cast<double>(finish.right()) - start.right()),
                std::abs(static_cast<double>(finish.top()) - start.top()),
                std::abs(static_cast<double>(finish.bottom()) - start.bottom()) });
            const float expected_width = static_cast<float>(start.width +
                (finish.width - start.width) * (entry + 0.01 / edge_travel));
            projectile->resolve_movement_contacts(fixture->scene->collision_objects());
            require(projectile->for_deletion(), "Thin moving relative impact was erased by a rounded re-gate");
            require_near(projectile->shape()->bounding_box().width, expected_width, 0.00001f);
            require(projectile->shape()->bounding_box().width < finish.width - 0.5f &&
                projectile->shape()->bounding_box().width > start.width + 0.5f,
                "Moving impact reused an endpoint size instead of interpolating diffusion edges");
            require(moving.paint_total().player_areas.at(source_owner) > 0.0f,
                "Thin diffusing impact lost original local paint ownership");

            const ProjectileTuning bare_values = isolated_shot(world, ProjectileType::jet);
            auto bare_fixture = course(world, { { 100.0f, 300.0f, 16.0f, 100.0f }, 12.0f, 1.0f, 0.25f },
                { false, false, false, false });
            auto& bare = *bare_fixture->movers.front();
            auto blocked = shot(world, bare_values, ProjectileType::jet,
                { bare.bounds().left() - 24.0f, 340.0f, 2.0f, 2.0f }, { 3840.0f, 0.0f });
            external_step(*bare_fixture, *blocked);
            const auto cue = blocked->take_blocked_contact();
            require(blocked->for_deletion() && cue && bits(cue->position.x) == bits(bare.bounds().left()),
                "Bare moving impact cue is not attached to the final current face");
            require(bare.paint_total().team_a == 0.0f, "Disabled paint faces were invented by relative contact");
        }

        void check_roller_actual_scene(World& world)
        {
            const ProjectileTuning values = isolated_shot(world, ProjectileType::rolling, 4.0f, 1.0f);
            Fixture fixture(world, 3);
            fixture.plain({ -500.0f, 350.0f, 600.0f, 80.0f }); // Ends at anchor.left100.
            const RectangleF body{ 53.0f, 230.0f, 52.0f, 120.0f }; // Supported5-unit overhang.
            auto& first = fixture.add_player(body, PlayerTeam::b);
            auto& second = fixture.add_player(body, PlayerTeam::b);
            auto& friendly = fixture.add_player(body, PlayerTeam::a);
            auto& moving = fixture.mover({ { 100.0f, 300.0f, 16.0f, 100.0f }, 12.0f, 1.0f, 0.25f });
            fixture.build(); fixture.start();
            auto projectile = shot(world, values, ProjectileType::rolling,
                { moving.bounds().left() - 2.0f, 349.5f, 4.0f, 1.0f }, Vector2F::ZERO);
            const RectangleF constructor_volume = projectile->shape()->bounding_box();
            // Clear the floor at launch; the path meets the mover after passing the floor edge.
            projectile->resolve_launch_obstruction({ 80.0f, 340.0f }, fixture.scene->collision_objects());
            const RectangleF published = projectile->shape()->bounding_box();
            require(published != constructor_volume && !projectile->for_deletion(),
                "Real Roller launch must relocate its cover-blocked published volume without consuming it");
            world.paint(moving, moving.bounds(), PlayerTeam::b, repaint_owner);
            require(moving.paint_total().team_a == 0.0f, "Roller launch paint reset did not establish enemy control");
            Projectile* active = fixture.scene->add(std::move(projectile)); fixture.scene->end_tick();
            const float first_health = first.health(), second_health = second.health(), friendly_health = friendly.health();
            fixture.tick();
            require(!active->for_deletion() && active->shape()->bounding_box() == published &&
                active->moving_paint_completed(moving.motion_id()),
                "Actual Scene Roller moving pass changed volume/lifetime or lost completion");
            require(!published.intersects(moving.bounds()) && moving.paint_total().player_areas.at(source_owner) > 0.0f,
                "Actual relative-only Roller launch volume failed to paint crossed moving surface");
            require(bits(first.health()) == bits(first_health - values.details.player_damage) &&
                bits(second.health()) == bits(second_health - values.details.player_damage) &&
                bits(friendly.health()) == bits(friendly_health),
                "Shared Roller volume lost either real enemy contact or friendly-fire mask");
            const auto history = motion_bits(moving.motion_state());
            fixture.tick(input(), 0.0f);
            require(!active->for_deletion() && active->moving_paint_completed(moving.motion_id()) &&
                motion_bits(moving.motion_state()) == history,
                "Whole-Level zero step rearmed or retired the completed Roller");
            active->update(dt); // Actual second update must keep the original short lifetime rule.
            require(active->for_deletion() && !active->moving_paint_completed(moving.motion_id()),
                "Next positive update did not clear pair completion and preserve Roller retirement");
            fixture.scene->end_tick(); // No dereference of active after retirement.
        }

        void check_roller_endpoint_duplicate(World& world)
        {
            const ProjectileTuning values = isolated_shot(world, ProjectileType::rolling, 4.0f, 1.0f);
            auto fixture = course(world, { { 100.0f, 300.0f, 16.0f, 100.0f }, 12.0f, 1.0f, 0.25f });
            auto& moving = *fixture->movers.front();
            auto projectile = shot(world, values, ProjectileType::rolling,
                { moving.bounds().left() + 1.0f, 340.0f, 4.0f, 1.0f }, Vector2F::ZERO);
            Projectile* active = fixture->scene->add(std::move(projectile)); fixture->scene->end_tick();
            fixture->tick();
            require(active->shape()->bounding_box().intersects(moving.bounds()) &&
                active->moving_paint_completed(moving.motion_id()),
                "Endpoint duplicate control did not establish a real overlapping completed pair");
            world.paint(moving, moving.bounds(), PlayerTeam::b, repaint_owner);
            const auto repainted = sample_bits(snapshot(moving));
            active->resolve_movement_contacts(fixture->scene->collision_objects());
            fixture->scene->resolve(); // Real ordinary endpoint callback for this same Roller pair.
            require(sample_bits(snapshot(moving)) == repainted && !active->for_deletion(),
                "Repeated full-span or ordinary endpoint callback painted a completed moving pair twice");
        }
    }

    void check_projectiles(World& world)
    {
        check_crossings(world);
        check_reversal_and_miss(world);
        check_zero_and_rearming(world);
        check_earliest_and_cover_tie(world);
        check_diffusion_and_current_cue(world);
        check_roller_actual_scene(world);
        check_roller_endpoint_duplicate(world);
    }
}
