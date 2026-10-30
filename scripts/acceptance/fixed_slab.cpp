#include "game/pch.h"
#include "game/objects/fixed_slab_geometry.h"
#include "engine/collision/narrow_phase.h"
#include "scripts/acceptance/support/probe.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <filesystem>
#include <process.h>

namespace fixed_slab_acceptance
{
    void support_checks(const std::filesystem::path& content);
    void runtime_checks(const std::filesystem::path& content);
    void authoring_checks(const std::filesystem::path& content, const std::filesystem::path& evidence);
}

using namespace mattmath;

namespace
{
    using acceptance::require;

    bool approximately(double actual, double expected, double tolerance = 0.000003)
    { return std::abs(actual - expected) <= tolerance; }

    void near_point(const Vector2F& actual, const Vector2F& expected, float tolerance = 0.0005f)
    { require(approximately(actual.x, expected.x, tolerance) && approximately(actual.y, expected.y, tolerance), "Slab point or transform differs"); }

    template<class Function> void rejected(Function&& function)
    {
        bool refused = false;
        try { function(); } catch (const std::invalid_argument&) { refused = true; }
        require(refused, "Invalid fixed-slab input was not refused");
    }

    void geometry_checks()
    {
        const std::array<FixedSlabGeometry, 2> slabs{
            FixedSlabGeometry({ { 1240.0f, 880.0f }, FixedSlabRise::right }),
            FixedSlabGeometry({ { 2600.0f, 800.0f }, FixedSlabRise::left }) };
        for (const auto& slab : slabs)
        {
            const bool right = slab.spec().rise == FixedSlabRise::right;
            near_point(slab.top_start(), slab.spec().top_left);
            near_point(slab.top_end(), slab.spec().top_left + Vector2F(160.0f, right ? -80.0f : 80.0f));
            require(approximately(slab.length(), 80.0 * std::sqrt(5.0), 0.00003), "Slab length is not the selected 160/80 diagonal");
            require(approximately(slab.tangent().length(), 1.0) && approximately(slab.inward_normal().length(), 1.0) &&
                approximately(slab.tangent().dot(slab.inward_normal()), 0.0) && slab.inward_normal().y > 0.0f,
                "Slab axes must be orthonormal with the normal pointing inward/down");
            require(slab.local_rectangle() == RectangleF(0.0f, 0.0f, slab.length(), 24.0f), "Slab local face dimensions changed");
            const auto quad = slab.world_quad(slab.local_rectangle());
            require(quad.point_0() == slab.shape().point_0() && quad.point_1() == slab.shape().point_1() &&
                quad.point_2() == slab.shape().point_2() && quad.point_3() == slab.shape().point_3(),
                "Full local slab rectangle must map to exactly the represented collider corners");
            for (const auto& local : { Vector2F(0.0f, 0.0f), Vector2F(slab.length(), 24.0f), Vector2F(71.0f, 11.0f), Vector2F(-9.0f, 39.0f) })
            { near_point(slab.to_local(slab.to_world(local)), local); }
            const RectangleF shot(1300.0f, 810.0f, 11.0f, 7.0f);
            const Quad local_shot = slab.local_quad(shot);
            near_point(slab.to_world(local_shot.point_0()), shot.top_left());
            near_point(slab.to_world(local_shot.point_1()), shot.top_right());
            near_point(slab.to_world(local_shot.point_2()), shot.bottom_right());
            near_point(slab.to_world(local_shot.point_3()), shot.bottom_left());
            // An inverse AABB is a different, larger footprint; preserving the
            // four actual corners is required for local diagonal paint.
            require(local_shot.point_0().y != local_shot.point_1().y && local_shot.point_1().x != local_shot.point_2().x,
                "Inverse world box was flattened into a local AABB");
            for (const auto& point : slab.shape().points())
            {
                require(point.x >= slab.bounds().left() && point.x <= slab.bounds().right() &&
                    point.y >= slab.bounds().top() && point.y <= slab.bounds().bottom(), "Slab culling bounds omit a corner");
            }
        }
        near_point(slabs[0].top_start(), { 4000.0f - slabs[1].top_end().x, slabs[1].top_end().y });
        near_point(slabs[0].top_end(), { 4000.0f - slabs[1].top_start().x, slabs[1].top_start().y });
    }

    void expect_interval(const FixedSlabGeometry& slab, const RectangleF& before, const RectangleF& after,
        double entry, double exit, bool initial, bool positive = true)
    {
        const auto swept = slab.sweep(before, after);
        require(swept.has_value(), "Expected diagonal sweep interval is absent");
        require(approximately(swept->entry, entry) && approximately(swept->exit, exit), "Diagonal sweep fraction differs from analytic answer");
        require(swept->initially_overlapping == initial && swept->positive_overlap == positive,
            "Diagonal sweep misclassified initial or positive-area overlap");
        require(approximately(swept->normal_to_rectangle.length(), 1.0), "Slab sweep normal must be a unit direction");
    }

    void sweep_checks()
    {
        // Independent ideal-coordinate oracle: on the top edge x+2y=160,
        // the inside normal projection ends at160+24sqrt(5). These fractions
        // use that equation, not the production clipping or shape projections.
        // The tolerance only covers reconstruction of the engine's float OBB.
        const FixedSlabGeometry slab({ { 0.0f, 80.0f }, FixedSlabRise::right });
        expect_interval(slab, { 79.0f, 0.0f, 2.0f, 2.0f }, { 79.0f, 100.0f, 2.0f, 2.0f },
            0.375, (81.0 + 24.0 * std::sqrt(5.0)) / 200.0, false);
        const auto down = slab.sweep({ 79.0f, 0.0f, 2.0f, 2.0f }, { 79.0f, 100.0f, 2.0f, 2.0f });
        require(down && down->normal_to_rectangle.x < 0.0f && down->normal_to_rectangle.y < 0.0f,
            "Top entry normal must point from slab towards the incoming rectangle");
        expect_interval(slab, { 79.0f, -200.0f, 2.0f, 2.0f }, { 79.0f, 200.0f, 2.0f, 2.0f },
            0.59375, (481.0 + 24.0 * std::sqrt(5.0)) / 800.0, false);
        require(!slab.sweep({ 0.0f, 0.0f, 4.0f, 4.0f }, { 100.0f, 0.0f, 4.0f, 4.0f }),
            "Empty AABB corner must not stop a shot before the actual diagonal");
        expect_interval(slab, { 79.0f, 19.0f, 2.0f, 2.0f }, { 59.0f, -1.0f, 42.0f, 42.0f }, 37.0 / 60.0, 1.0, false);
        expect_interval(slab, { 59.0f, -1.0f, 42.0f, 42.0f }, { 79.0f, 19.0f, 2.0f, 2.0f }, 0.0, 23.0 / 60.0, true);
        expect_interval(slab, { 79.0f, 49.0f, 2.0f, 2.0f }, { 79.0f, -51.0f, 2.0f, 2.0f }, 0.0, 0.115, true);
        expect_interval(slab, { 79.0f, 49.0f, 2.0f, 2.0f }, { 79.0f, 49.0f, 2.0f, 2.0f }, 0.0, 1.0, true);
        require(!slab.sweep({ -20.0f, -20.0f, 2.0f, 2.0f }, { -20.0f, -20.0f, 2.0f, 2.0f }), "Separated zero step reported contact");

        const FixedSlabGeometry mirror({ { 40.0f, 0.0f }, FixedSlabRise::left });
        expect_interval(mirror, { 119.0f, 0.0f, 2.0f, 2.0f }, { 119.0f, 100.0f, 2.0f, 2.0f },
            0.375, (81.0 + 24.0 * std::sqrt(5.0)) / 200.0, false);
        const auto mirrored = mirror.sweep({ 119.0f, 0.0f, 2.0f, 2.0f }, { 119.0f, 100.0f, 2.0f, 2.0f });
        require(mirrored && down && approximately(mirrored->normal_to_rectangle.x, -down->normal_to_rectangle.x) &&
            approximately(mirrored->normal_to_rectangle.y, down->normal_to_rectangle.y), "Mirrored sweep changed entry normal handedness");

        // Construct tangency from a represented world-Y extremum. Unlike an
        // ideal top-face equation, this is exactly representable after float
        // corner reconstruction. The whole time interval lies on one vertex.
        const auto lowest = *std::max_element(slab.shape().points().begin(), slab.shape().points().end(),
            [](const auto& a, const auto& b) { return a.y < b.y; });
        const RectangleF tangent_before(lowest.x - 12.0f, lowest.y, 2.0f, 2.0f);
        const RectangleF tangent_after(lowest.x + 8.0f, lowest.y, 2.0f, 2.0f);
        expect_interval(slab, tangent_before, tangent_after, 0.5, 0.6, false, false);
        expect_interval(slab, { lowest.x - 1.0f, lowest.y, 2.0f, 2.0f },
            { lowest.x - 1.0f, lowest.y + 10.0f, 2.0f, 2.0f }, 0.0, 0.0, false, false);

        // An enemy at y20 enters at.18, well before the diagonal at.375.
        // Selecting the slab's bounding-box entry would reverse that ordering.
        require(down && 18.0 / 100.0 < down->entry && down->entry < 0.4,
            "Exact slab entry no longer preserves nearer participant ordering");
    }

    void independent_overlap_checks()
    {
        std::size_t checked = 0;
        for (FixedSlabRise rise : { FixedSlabRise::left, FixedSlabRise::right })
        {
            const FixedSlabGeometry slab({ { 0.0f, rise == FixedSlabRise::right ? 80.0f : 0.0f }, rise });
            for (int x = -10; x <= 180; x += 10)
            {
                for (int y = -10; y <= 120; y += 10)
                {
                    const RectangleF box(static_cast<float>(x) + 0.125f, static_cast<float>(y) + 0.375f, 3.25f, 5.75f);
                    const bool expected = labrador::narrow_phase(slab.shape(), box).has_value();
                    require(slab.overlaps(box) == expected, "Strict slab overlap disagrees with independent engine SAT");
                    const auto stationary = slab.sweep(box, box);
                    require((stationary && stationary->positive_overlap) == expected, "Stationary slab sweep differs from actual overlap");
                    ++checked;
                }
            }
        }
        require(checked == 560, "Independent overlap grid inventory changed");
    }

    void invalid_checks()
    {
        const float nan = std::numeric_limits<float>::quiet_NaN(), infinity = std::numeric_limits<float>::infinity();
        rejected([&] { FixedSlabGeometry geometry({ { nan, 0.0f }, FixedSlabRise::right }); });
        rejected([&] { FixedSlabGeometry geometry({ { 0.0f, infinity }, FixedSlabRise::right }); });
        rejected([] { FixedSlabGeometry geometry({ { 0.0f, 0.0f }, static_cast<FixedSlabRise>(77) }); });
        rejected([] { FixedSlabGeometry geometry({ { 1.0e30f, 80.0f }, FixedSlabRise::right }); });
        rejected([] { FixedSlabGeometry geometry({ { 65537.0f, 80.0f }, FixedSlabRise::right }); });
        rejected([] { FixedSlabGeometry geometry({ { 65500.0f, 80.0f }, FixedSlabRise::right }); });
        rejected([] { FixedSlabGeometry geometry({ { 0.0f, -65500.0f }, FixedSlabRise::right }); });
        const FixedSlabGeometry slab({ { 0.0f, 80.0f }, FixedSlabRise::right });
        rejected([&] { (void)slab.sweep({ nan, 0.0f, 2.0f, 2.0f }, { 0.0f, 0.0f, 2.0f, 2.0f }); });
        rejected([&] { (void)slab.sweep({ 0.0f, 0.0f, 2.0f, 2.0f }, { 0.0f, 0.0f, 0.0f, 2.0f }); });
        rejected([&] { (void)slab.overlaps({ 1.0e30f, 0.0f, 2.0f, 2.0f }); });
        rejected([&] { (void)slab.to_local({ infinity, 0.0f }); });
        rejected([&] { (void)slab.to_world({ 0.0f, nan }); });
    }
}

int main(int argc, char** argv)
{
    return acceptance::run_probe("fixed slab geometry", [&]
    {
        require(argc == 2, "Expected the staged game directory");
        geometry_checks(); sweep_checks(); independent_overlap_checks(); invalid_checks();
        fixed_slab_acceptance::support_checks(std::filesystem::path(argv[1]));
        fixed_slab_acceptance::runtime_checks(std::filesystem::path(argv[1]));
        const auto evidence = std::filesystem::absolute(argv[0]).parent_path() /
            ("fixed-slab-" + std::to_string(_getpid()));
        require(!std::filesystem::exists(evidence), "Refusing earlier fixed-slab receipts");
        std::filesystem::create_directories(evidence);
        std::cout << "Fixed slab receipts: " << evidence.string() << '\n';
        fixed_slab_acceptance::authoring_checks(std::filesystem::path(argv[1]), evidence);
        std::cout << "PASS fixed slab geometry: selected mirrored160/80/normal24 frame, collider/paint transforms, exact four-axis "
            "changing-box sweeps, thin/empty-corner/growth/initial/zero-step/tangent controls and560 independent SAT probes; "
            "runtime object/Player checks reported separately; rendered and human acceptance remain open\n";
        return 0;
    });
}
