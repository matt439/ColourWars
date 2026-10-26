#include "game/pch.h"
#include "scripts/acceptance/moving_structure/fixtures.h"
#include "game/objects/fixed_slab_paintable.h"
#include "game/objects/projectile_spray.h"
#include "game/objects/projectile_mist.h"
#include "engine/collision/narrow_phase.h"
#include "game/fixed_slab_paint_geometry.h"
#include <array>
#include <iostream>

namespace fixed_slab_acceptance
{
    namespace
    {
        using namespace mattmath;
        using namespace labrador;
        using moving_structure_acceptance::World;
        using moving_structure_acceptance::Fixture;
        using moving_structure_acceptance::require;
        using moving_structure_acceptance::snapshot;
        using moving_structure_acceptance::sample_bits;
        using moving_structure_acceptance::same_total;
        constexpr int owner = 4, friendly_owner = 5, enemy_owner = 6;

        void close(double actual, double expected, double tolerance, const char* reason)
        { require(std::isfinite(actual) && std::abs(actual - expected) <= tolerance, reason); }

        std::unique_ptr<FixedSlabPaintable> slab(World& world, FixedSlabRise rise,
            std::span<const RectangleF> obstacles = {}, bool paintable = true)
        {
            return std::make_unique<FixedSlabPaintable>(
                FixedSlabSpec{ { 0.0f, rise == FixedSlabRise::right ? 80.0f : 0.0f }, rise },
                &world.render, &world.audio, &world.preferences, &world.tuning.paint(), world.palette,
                PaintableFaces{ paintable, paintable, paintable, paintable }, obstacles);
        }

        ProjectileTuning shot_values(World& world, ProjectileType type)
        {
            auto values = world.tuning.projectile(type);
            values.details.col_rect_size = { 2.0f, 2.0f };
            values.details.gravity = values.details.wind_resistance = 0.0f;
            values.details.delete_timer = 10.0f;
            values.max_velocity = { 100000.0f, 100000.0f };
            values.diffusion.type = DiffusionType::no_diffusion;
            return values;
        }

        std::unique_ptr<Projectile> shot(World& world, const ProjectileTuning& values,
            ProjectileType type, RectangleF desired, const Vector2F& velocity,
            PlayerTeam team = PlayerTeam::a, int paint_owner = owner)
        {
            desired.offset(values.details.col_rect_size / 2.0f);
            const auto strip = resolve_projectile_strip(values, world.render);
            const auto colour = world.palette.team_colour(team);
            switch (type)
            {
            case ProjectileType::jet:
                return std::make_unique<ProjectileJet>(desired, velocity, team, paint_owner,
                    colour, &world.render, strip, &values);
            case ProjectileType::spray:
                return std::make_unique<ProjectileSpray>(desired, velocity, team, paint_owner,
                    colour, &world.render, strip, &values);
            case ProjectileType::mist:
                return std::make_unique<ProjectileMist>(desired, velocity, team, paint_owner,
                    colour, &world.render, strip, &values);
            default: throw std::runtime_error("Unsupported fixed-slab acceptance projectile");
            }
        }

        void paint(World& world, FixedSlabPaintable& surface, const RectangleF& footprint,
            PlayerTeam team, int paint_owner)
        {
            const auto values = shot_values(world, ProjectileType::jet);
            const auto projectile = shot(world, values, ProjectileType::jet, footprint,
                Vector2F::ZERO, team, paint_owner);
            surface.on_contact(*projectile, Vector2F::ZERO, 0.0f);
        }

        void local_score_and_credit(World& world)
        {
            const double length = 80.0 * std::sqrt(5.0);
            const double thickness = world.tuning.paint().thickness;
            require(thickness == 12.0, "Fixed-slab analytic score fixture expects the preserved twelve-unit paint depth");
            for (const auto rise : { FixedSlabRise::right, FixedSlabRise::left })
            {
                auto open = slab(world, rise);
                const auto neutral = snapshot(*open, 23);
                std::array<double, 4> faces{};
                for (const auto& sample : neutral)
                {
                    require(sample.team == PlayerTeam::none && sample.paint_owner == -1 && sample.source_index == 23,
                        "New fixed slab did not publish neutral local tiles and caller source ordinals");
                    require(sample.scored_area > 0.0f && std::isfinite(sample.scored_area), "Slab tile has an invalid authoritative area");
                    faces.at(static_cast<std::size_t>(sample.face)) += sample.scored_area;
                    const auto a = open->geometry().to_local(sample.corners[0]);
                    const auto b = open->geometry().to_local(sample.corners[1]);
                    const auto d = open->geometry().to_local(sample.corners[3]);
                    close((b - a).length() * (d - a).length(), sample.scored_area, 0.002,
                        "World paint sample does not preserve its authoritative local tile area");
                }
                close(faces[0], length * thickness, 0.005, "Diagonal top face score differs from independent length times depth");
                close(faces[1], length * thickness, 0.005, "Diagonal underside score differs from independent length times depth");
                close(faces[2], 24.0 * thickness, 0.001, "Left normal cap score differs");
                close(faces[3], 24.0 * thickness, 0.001, "Right normal cap score differs");
                close(open->paint_total().total, thickness * (2.0 * length + 48.0), 0.025,
                    "Open slab score differs from independent four-face formula");

                // Normal extrusion overlaps the fixed crossover. Its buried
                // normal cap and twelve units of the underside must disappear.
                const std::array<RectangleF, 1> obstacle{ rise == FixedSlabRise::right
                    ? RectangleF(160.0f, 0.0f, 160.0f, 80.0f)
                    : RectangleF(-160.0f, 0.0f, 160.0f, 80.0f) };
                auto clipped = slab(world, rise, obstacle);
                const auto clipped_samples = snapshot(*clipped);
                std::array<double, 4> clipped_faces{};
                for (const auto& sample : clipped_samples)
                { clipped_faces.at(static_cast<std::size_t>(sample.face)) += sample.scored_area; }
                close(clipped_faces[0], length * thickness, 0.005, "Crossover clipping removed the exposed diagonal top");
                close(clipped_faces[1], (length - 12.0) * thickness, 0.005, "Crossover clipping lost the analytic underside intersection");
                const auto buried_face = rise == FixedSlabRise::right ? 3u : 2u;
                close(clipped_faces[buried_face], 0.0, 0.0, "Buried normal cap still scores");
                close(clipped_faces[buried_face == 3u ? 2u : 3u], 24.0 * thickness, 0.001,
                    "Exposed normal cap was removed by an obstacle's bounding box");
                close(clipped->paint_total().total, thickness * (2.0 * length + 12.0), 0.025,
                    "Clipped slab total differs from independent diagonal-edge intersection");
                require(clipped->exposed_bottom().size() == 1, "Clipped underside shadow interval inventory differs");
                close(clipped->exposed_bottom().front().second - clipped->exposed_bottom().front().first,
                    length - 12.0, 0.0005, "Published underside interval differs from scoring exposure");

                // This real authored gap is smaller than the four-ULP bound
                // used to check OBB reconstruction. Obstacles themselves are
                // never snapped, so its exposed normal-cap strip must survive.
                constexpr float real_gap = 0.00004f;
                const std::array<RectangleF, 1> gapped_obstacle{ rise == FixedSlabRise::right
                    ? RectangleF(160.0f, real_gap, 160.0f, 80.0f)
                    : RectangleF(-160.0f, real_gap, 160.0f, 80.0f) };
                const auto gapped = slab(world, rise, gapped_obstacle);
                double gap_cap_area = 0.0;
                for (const auto& sample : snapshot(*gapped))
                {
                    if (static_cast<std::size_t>(sample.face) == buried_face)
                    { gap_cap_area += sample.scored_area; }
                }
                require(gap_cap_area > 0.0, "Canonical seam correction erased a real authored small gap");
                close(gap_cap_area, thickness * real_gap * std::sqrt(5.0) / 2.0, 0.0000001,
                    "Real authored small-gap face differs from its independent normal projection");

                const auto& represented = open->geometry().shape().points();
                PaintPolygon broken{ PaintPoint{ represented[0].x, represented[0].y },
                    PaintPoint{ represented[1].x, represented[1].y },
                    PaintPoint{ represented[2].x, represented[2].y },
                    PaintPoint{ represented[3].x, represented[3].y } };
                broken[1].x += 0.25;
                const auto normal = open->geometry().inward_normal();
                bool refused = false;
                try
                {
                    (void)canonical_slab_paint_polygon(broken,
                        { 0.0, rise == FixedSlabRise::right ? 80.0 : 0.0 },
                        { 160.0, rise == FixedSlabRise::right ? 0.0 : 80.0 }, { normal.x, normal.y });
                }
                catch (const std::invalid_argument&) { refused = true; }
                require(refused, "Canonical seam correction accepted geometry outside its checked ULP bound");

                const auto correct = canonical_slab_paint_polygon(
                    PaintPolygon{ PaintPoint{ represented[0].x, represented[0].y },
                        PaintPoint{ represented[1].x, represented[1].y },
                        PaintPoint{ represented[2].x, represented[2].y },
                        PaintPoint{ represented[3].x, represented[3].y } },
                    { 0.0, rise == FixedSlabRise::right ? 80.0 : 0.0 },
                    { 160.0, rise == FixedSlabRise::right ? 0.0 : 80.0 }, { normal.x, normal.y });
                auto fractional_error = correct;
                const float coordinate_ulp = std::nextafter(160.0f, std::numeric_limits<float>::infinity()) - 160.0f;
                fractional_error[1].x += 4.25 * coordinate_ulp;
                refused = false;
                try
                {
                    (void)canonical_slab_paint_polygon(fractional_error,
                        { 0.0, rise == FixedSlabRise::right ? 80.0 : 0.0 },
                        { 160.0, rise == FixedSlabRise::right ? 0.0 : 80.0 }, { normal.x, normal.y });
                }
                catch (const std::invalid_argument&) { refused = true; }
                require(refused, "Canonical seam gate rounded down a raw error larger than four ULPs");

                paint(world, *open, open->bounds(), PlayerTeam::a, owner);
                auto total = open->paint_total();
                close(total.team_a, total.total, 0.0, "Full actual rectangle contact did not paint every local face");
                close(total.player_areas.at(owner), total.total, 0.0, "Slab did not retain raw slot-four paint credit");
                paint(world, *open, open->bounds(), PlayerTeam::a, friendly_owner);
                total = open->paint_total();
                close(total.player_areas.at(owner), total.total, 0.0, "Friendly repaint stole sloped-face credit");
                close(total.player_areas.at(friendly_owner), 0.0, 0.0, "Friendly repaint gained false credit");
                paint(world, *open, open->bounds(), PlayerTeam::b, enemy_owner);
                total = open->paint_total();
                close(total.team_a, 0.0, 0.0, "Enemy repaint left the old team on a local face");
                close(total.team_b, total.total, 0.0, "Enemy repaint missed a local face");
                close(total.player_areas.at(enemy_owner), total.total, 0.0, "Enemy capture did not transfer raw-owner area");
                const float middle_x = (open->geometry().top_start().x + open->geometry().top_end().x) / 2.0f;
                require(open->top_paint_team(middle_x) == PlayerTeam::b && open->top_paint_colour(middle_x).has_value(),
                    "Actual sloping top contact cannot read its paint ownership");
                require(open->top_paint_team(open->geometry().top_end().x) == PlayerTeam::none,
                    "Sloping top paint query lost its half-open final edge");

                const auto before = sample_bits(snapshot(*open));
                const auto total_before = open->paint_total();
                std::srand(7319); const int expected_random = std::rand(); std::srand(7319);
                for (int repeat = 0; repeat < 3; ++repeat)
                {
                    require(sample_bits(snapshot(*open)) == before, "Slab read mutated public tile geometry or credit");
                    same_total(total_before, open->paint_total());
                    (void)open->bounds(); (void)open->cull_bounds(3.0f);
                    (void)open->top_paint_team(middle_x); (void)open->local_material().quads();
                }
                require(std::rand() == expected_random, "Read-only slab observations consumed gameplay RNG");
            }
        }

        void inverse_quad_contact(World& world)
        {
            auto surface = slab(world, FixedSlabRise::right);
            const RectangleF footprint(65.125f, 0.25f, 4.5f, 80.5f);
            const auto before = snapshot(*surface);
            const auto inverse = surface->geometry().local_quad(footprint);
            const RectangleF conservative = inverse.bounding_box();
            paint(world, *surface, footprint, PlayerTeam::a, owner);
            const auto after = snapshot(*surface);
            require(before.size() == after.size(), "Painting replaced slab's local tile inventory");
            std::size_t actual_hits = 0, false_box_hits = 0;
            for (std::size_t index = 0; index < before.size(); ++index)
            {
                const auto& sample = before[index];
                const Quad world_tile(sample.corners[0], sample.corners[1], sample.corners[2], sample.corners[3]);
                const bool expected = narrow_phase(world_tile, footprint).has_value();
                require((after[index].team == PlayerTeam::a) == expected,
                    "Slab painted an inverse-AABB-only tile or missed a true world-quad overlap");
                if (expected) { ++actual_hits; }
                const auto a = surface->geometry().to_local(sample.corners[0]);
                const auto c = surface->geometry().to_local(sample.corners[2]);
                const RectangleF local_tile(a, c - a);
                if (!expected && narrow_phase(local_tile, conservative))
                {
                    ++false_box_hits;
                    require(after[index].paint_owner == -1, "Inverse AABB gained raw paint credit outside the true footprint");
                }
            }
            require(actual_hits > 0 && false_box_hits > 0, "Inverse-quad control did not exercise real paint and false bounding-box candidates");
        }

        void projectile_flight_and_growth(World& world)
        {
            const float dt = moving_structure_acceptance::dt;
            for (const auto rise : { FixedSlabRise::right, FixedSlabRise::left })
            {
                auto surface = slab(world, rise);
                const auto values = shot_values(world, ProjectileType::jet);
                auto projectile = shot(world, values, ProjectileType::jet,
                    { 79.0f, 0.0f, 2.0f, 2.0f }, { 0.0f, 100.0f / dt });
                std::array<CollisionObject*, 1> targets{ surface.get() };
                projectile->update(dt); projectile->resolve_movement_contacts(targets);
                require(projectile->for_deletion() && surface->paint_total().player_areas.at(owner) > 0.0f,
                    "Actual projectile crossed the whole normal-thickness slab without painting and retiring");
                require(projectile->shape()->bounding_box().top() >= 37.49f && projectile->shape()->bounding_box().top() < 38.0f,
                    "Actual projectile selected the slab AABB or endpoint instead of first diagonal contact");
                require(surface->geometry().overlaps(projectile->shape()->bounding_box()),
                    "Accepted diagonal flight has no representable actual overlap");

                auto clear = slab(world, rise);
                const RectangleF empty = rise == FixedSlabRise::right
                    ? RectangleF(0.0f, 0.0f, 4.0f, 4.0f) : RectangleF(156.0f, 0.0f, 4.0f, 4.0f);
                const Vector2F travel(rise == FixedSlabRise::right ? 100.0f / dt : -100.0f / dt, 0.0f);
                auto miss = shot(world, values, ProjectileType::jet, empty, travel);
                std::array<CollisionObject*, 1> clear_targets{ clear.get() };
                miss->update(dt); miss->resolve_movement_contacts(clear_targets);
                require(!miss->for_deletion() && clear->paint_total().team_a == 0.0f,
                    "An actual projectile was blocked or painted in an empty corner of the slab AABB");

                for (const auto type : { ProjectileType::spray, ProjectileType::mist })
                {
                    auto growing_surface = slab(world, rise);
                    auto growth = shot_values(world, type);
                    growth.diffusion.type = DiffusionType::linear_size_and_col_rect;
                    growth.diffusion.start_time = 0.0f; growth.diffusion.end_time = dt;
                    growth.diffusion.end_scale = { 21.0f, 21.0f };
                    auto cloud = shot(world, growth, type, { 79.0f, 19.0f, 2.0f, 2.0f }, Vector2F::ZERO);
                    std::array<CollisionObject*, 1> growth_targets{ growing_surface.get() };
                    cloud->update(dt); cloud->resolve_movement_contacts(growth_targets);
                    require(cloud->for_deletion() && growing_surface->paint_total().player_areas.at(owner) > 0.0f,
                        "Real growing projectile failed to reach diagonal paint with stationary centre");
                    require(cloud->shape()->bounding_box().width > 2.0f && cloud->shape()->bounding_box().width < 42.0f,
                        "Growing diagonal impact reused its initial or final footprint rather than interpolated edges");
                }
            }
        }

        void launch_and_participant_order(World& world)
        {
            const auto values = shot_values(world, ProjectileType::jet);
            for (const auto rise : { FixedSlabRise::right, FixedSlabRise::left })
            {
                for (const bool crosses : { false, true })
                {
                    auto surface = slab(world, rise);
                    auto projectile = shot(world, values, ProjectileType::jet,
                        { 79.0f, crosses ? 89.0f : 9.0f, 2.0f, 2.0f }, Vector2F::ZERO);
                    std::array<CollisionObject*, 1> targets{ surface.get() };
                    projectile->resolve_launch_obstruction({ 80.0f, 39.5f }, targets);
                    require(projectile->for_deletion() == crosses,
                        "Diagonal launch departure exemption allowed a crossing or blocked a same-side exit");
                    require((surface->paint_total().team_a > 0.0f) == crosses,
                        "Diagonal launch paint disagrees with actual solid traversal");
                }
                for (const bool launch : { false, true })
                {
                    for (const bool nearer_enemy : { false, true })
                    {
                        for (const bool reverse : { false, true })
                        {
                            auto surface = slab(world, rise);
                            Fixture actors(world, 1);
                            Player& enemy = actors.add_player({ 76.0f, nearer_enemy ? 20.0f : 80.0f, 8.0f, 8.0f }, PlayerTeam::b);
                            const float health = enemy.health();
                            auto projectile = shot(world, values, ProjectileType::jet,
                                { 79.0f, launch ? 100.0f : 0.0f, 2.0f, 2.0f },
                                launch ? Vector2F::ZERO : Vector2F(0.0f, 100.0f / moving_structure_acceptance::dt));
                            std::array<CollisionObject*, 2> targets{ surface.get(), &enemy };
                            if (reverse) { std::reverse(targets.begin(), targets.end()); }
                            if (launch) { projectile->resolve_launch_obstruction({ 80.0f, 1.0f }, targets); }
                            else
                            {
                                projectile->update(moving_structure_acceptance::dt);
                                projectile->resolve_movement_contacts(targets);
                            }
                            require(projectile->for_deletion(), "Nearest actual participant failed to retire flying paint");
                            require((enemy.health() < health) == nearer_enemy,
                                "Diagonal sweep selected an AABB-first cover or a farther real enemy");
                            require((surface->paint_total().team_a > 0.0f) == !nearer_enemy,
                                "Nearest participant ordering painted through a closer enemy or through cover");
                        }
                    }
                }
            }
        }
    }

    void runtime_checks(const std::filesystem::path& content)
    {
        World world(content);
        local_score_and_credit(world);
        inverse_quad_contact(world);
        projectile_flight_and_growth(world);
        launch_and_participant_order(world);
        std::cout << "PASS actual fixed slab: mirrored analytic local-face/clipped score, neutral/friendly/enemy/raw-slot4 credit, "
            "true inverse-quad paint, frozen read/RNG observations, diagonal Jet traversal and empty-corner misses, "
            "real Spray/Mister growth, launch same-side/crossing and nearest enemy/cover ordering in both target orders; "
            "rendered, Player-route and human acceptance remain separate\n";
    }
}
