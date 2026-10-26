#include "game/pch.h"
#include "scripts/acceptance/moving_structure/fixtures.h"

namespace moving_structure_acceptance
{
    namespace
    {
        mattmath::RectangleF sample_box(const PaintSurfaceSample& sample)
        {
            return { sample.corners[0].x, sample.corners[0].y,
                sample.corners[2].x - sample.corners[0].x,
                sample.corners[2].y - sample.corners[0].y };
        }
        void translated_samples(const std::vector<PaintSurfaceSample>& local,
            const std::vector<PaintSurfaceSample>& world, float offset, std::size_t source)
        {
            require(local.size() == world.size() && !local.empty(), "Moving scoring snapshot lost actual tiles");
            for (std::size_t index = 0; index < local.size(); ++index)
            {
                const auto& a = local[index]; const auto& b = world[index];
                require(a.face == b.face && a.team == b.team && a.paint_owner == b.paint_owner &&
                    bits(a.scored_area) == bits(b.scored_area) && b.source_index == source,
                    "Translation changed local face, credit, scored float or source grouping");
                for (std::size_t corner = 0; corner < a.corners.size(); ++corner)
                {
                    require(bits(b.corners[corner].x) == bits(a.corners[corner].x + offset) &&
                        bits(b.corners[corner].y) == bits(a.corners[corner].y), "Snapshot corners did not move exactly once");
                }
            }
        }
        void direct_face_ownership(World& world)
        {
            const HorizontalMotionSpec spec{ { 512.0f, 500.0f, 81.5f, 50.75f }, 64.0f, 2.0f, 0.25f };
            const PaintableFaces faces{ true, true, true, true };
            auto moving = world.mover(spec, faces);
            auto stationary = world.surface(spec.anchor, faces);
            const auto local_initial = snapshot(*stationary);
            std::vector<PaintSurfaceSample> anchored;
            moving->append_anchor_paint_surfaces(anchored);
            require(sample_bits(anchored) == sample_bits(local_initial), "Moving neutral anchor tiles differ from static control");
            same_total(moving->paint_total(), stationary->paint_total());
            bool fractional = false;
            for (const auto& sample : anchored)
            {
                require(sample.team == PlayerTeam::none && sample.paint_owner == -1,
                    "Neutral moving geometry acquired fabricated ownership");
                require(std::isfinite(sample.scored_area) && sample.scored_area > 0.0f,
                    "Actual paint tile has invalid scored area");
                fractional |= std::floor(sample.scored_area) != sample.scored_area;
            }
            require(fractional, "Face ownership control has no actual fractional scoring tile");
            for (PaintSurfaceFace face : { PaintSurfaceFace::top, PaintSurfaceFace::bottom,
                PaintSurfaceFace::left, PaintSurfaceFace::right })
            {
                std::vector<PaintSurfaceSample> candidates;
                for (const auto& sample : local_initial) { if (sample.face == face) { candidates.push_back(sample); } }
                require(!candidates.empty(), "A requested actual moving scoring face is missing");
                const auto tile = sample_box(candidates[candidates.size() / 2]);
                const auto center = tile.center();
                const mattmath::RectangleF local(center.x - 0.25f, center.y - 0.25f, 0.5f, 0.5f);
                auto actual = local; actual.offset(moving->translation());
                for (const auto& capture : { std::pair{PlayerTeam::a, 5}, std::pair{PlayerTeam::a, 6},
                    std::pair{PlayerTeam::b, 7} })
                {
                    world.paint(*stationary, local, capture.first, capture.second);
                    world.paint(*moving, actual, capture.first, capture.second);
                    anchored.clear(); moving->append_anchor_paint_surfaces(anchored);
                    require(sample_bits(anchored) == sample_bits(snapshot(*stationary)), "Moved real paint contact differs from static local control");
                    const int expected_owner = capture.first == PlayerTeam::a ? 5 : 7;
                    bool held = false;
                    for (const auto& sample : anchored)
                    {
                        if (sample.face == face && sample_box(sample).intersects(local))
                        {
                            held = true;
                            require(sample.team == capture.first && sample.paint_owner == expected_owner,
                                "Friendly retention or enemy transfer lost original projectile credit");
                        }
                    }
                    require(held, "Typed paint contact did not reach its actual face tiles");
                    same_total(moving->paint_total(), stationary->paint_total());
                }
            }
            anchored.clear(); moving->append_anchor_paint_surfaces(anchored);
            translated_samples(anchored, snapshot(*moving, 77), moving->translation().x, 77);
            std::vector<PaintSurfaceSample> appended(1);
            appended[0].source_index = 931;
            const auto sentinel = sample_bits(appended);
            moving->append_paint_surfaces(appended, 77);
            require(appended[0].source_index == 931 && sample_bits({ appended[0] }) == sentinel,
                "Append snapshot modified a preexisting source group");
            const auto total = moving->paint_total(); const auto stable = sample_bits(snapshot(*moving));
            std::srand(113); const int expected = std::rand(); std::srand(113);
            std::ignore = moving->top_paint_team(moving->bounds().center().x);
            std::ignore = moving->top_paint_colour(moving->bounds().center().x);
            std::ignore = moving->runoff_visuals(); std::ignore = moving->cull_bounds(4.0f);
            std::ignore = snapshot(*moving); std::ignore = moving->motion_state();
            require(std::rand() == expected, "Read-only moving snapshot/query consumed gameplay RNG");
            require(sample_bits(snapshot(*moving)) == stable, "Read-only queries repainted moving geometry");
            same_total(total, moving->paint_total());
            require(moving->top_paint_team(moving->bounds().right()) == PlayerTeam::none,
                "Half-open final top edge acquired an outside tile");
        }
        void half_open_world_edge(World& world)
        {
            // This actual float offset crosses the1024 binade. The published
            // leading edge rounds to1032, but inverse subtraction rounds below
            //1000; comparing transformed tile edges is the independent oracle.
            const float phase = std::bit_cast<float>(std::uint32_t{ 0x3e800009 });
            const HorizontalMotionSpec spec{ { 1000.0f, 500.0f, 81.5f, 50.75f }, 64.0f, 2.0f, phase };
            auto moving = world.mover(spec);
            const auto bounds = moving->bounds();
            require(bounds.left() - moving->translation().x < spec.anchor.left(),
                "Precision control no longer exercises a noninvertible float translation");
            world.paint(*moving, bounds, PlayerTeam::a, 5);
            require(moving->paint_total().team_a > 0.0f &&
                moving->top_paint_team(bounds.left()) == PlayerTeam::a &&
                moving->top_paint_colour(bounds.left()).has_value(),
                "Published half-open leading edge lost its actual painted tile");
            require(moving->top_paint_team(bounds.right()) == PlayerTeam::none,
                "Published half-open trailing edge acquired an outside tile");
        }
        void cycle_ownership(World& world)
        {
            Fixture fixture(world);
            fixture.plain({ -512.0f, 1200.0f, 4000.0f, 80.0f });
            auto& moving = fixture.mover({ { 512.0f, 500.0f, 128.5f, 50.75f }, 64.0f, 2.0f, 0.25f });
            fixture.add_player(100.0f, 1200.0f); fixture.build(); fixture.start();
            const auto rectangle = moving.bounds();
            world.paint(moving, rectangle, PlayerTeam::a, 5);
            const auto total = moving.paint_total();
            std::vector<PaintSurfaceSample> local; moving.append_anchor_paint_surfaces(local);
            const auto held = sample_bits(local); bool moved = false;
            for (int step = 0; step < 300; ++step)
            {
                fixture.tick();
                std::vector<PaintSurfaceSample> current; moving.append_anchor_paint_surfaces(current);
                require(sample_bits(current) == held, "Motion changed local tile order, ownership or scored area");
                same_total(total, moving.paint_total());
                translated_samples(current, snapshot(moving, 9), moving.translation().x, 9);
                moved |= moving.bounds().x != rectangle.x;
            }
            require(moved && total.team_a > 0.0f && total.player_areas[5] > 0.0f,
                "Ownership cycle control did not exercise painted moving state");
            auto uncredited = moving.bounds();
            world.paint(moving, uncredited, PlayerTeam::b, -1);
            bool ownerless = false;
            for (const auto& sample : snapshot(moving))
            {
                if (sample.team == PlayerTeam::b)
                { ownerless = true; require(sample.paint_owner == -1, "Unknown paint owner was fabricated during transfer"); }
            }
            require(ownerless && moving.paint_total().team_b > 0.0f,
                "Actual uncredited enemy paint did not retain its team scoring");
        }
    }
    void check_paint(World& world)
    {
        direct_face_ownership(world);
        half_open_world_edge(world);
        cycle_ownership(world);
    }
}
