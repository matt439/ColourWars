#include "game/pch.h"
#include "scripts/acceptance/crossfeed/fixtures.h"
#include <algorithm>

namespace crossfeed_acceptance
{
    namespace
    {
        struct Strip { size_t owner; int face; RectangleF box; };
        float area_in(const RectangleF& a, const RectangleF& b)
        {
            return std::max(0.0f, std::min(a.right(), b.right()) - std::max(a.left(), b.left())) *
                std::max(0.0f, std::min(a.bottom(), b.bottom()) - std::max(a.top(), b.top()));
        }
    }

    void geometry_checks(Services& s, const std::filesystem::path& output)
    {
        const RectangleF bounds = s.map.info.out_of_bounds_rectangle();
        const auto a = s.map.info.team_a_spawns(), b = s.map.info.team_b_spawns();
        require(a.size() == 4 && b.size() == 4, "Crossfeed needs four full-body spawns per team");
        for (size_t index = 0; index < a.size(); ++index)
        {
            require(mirror(RectangleF(a[index], s.tuning.player().size), bounds) ==
                RectangleF(b[index], s.tuning.player().size), "Crossfeed spawn pairs do not mirror complete bodies");
        }
        for (const Surface& support : s.map.surfaces)
        {
            if (!support.jump_through()) { continue; }
            const RectangleF body(standing_on(s, support, support.rectangle.center().x), s.tuning.player().size);
            for (const Surface& other : s.map.surfaces)
            {
                if (&other == &support || other.ramp()) { continue; }
                const auto overlap = narrow_phase(body, other.rectangle);
                require(!overlap || overlap->penetration <= tolerance,
                    "Crossfeed standing body on '" + support.name + "' intersects '" + other.name + "'");
            }
        }

        // An independent face partition: split at every authored blocker edge,
        // then probe outward from each segment's midpoint. Do not call the
        // runtime/editor interval-clipping helper to predict its own result.
        std::vector<Strip> strips;
        const float thickness = s.tuning.paint().thickness;
        for (size_t owner = 0; owner < s.map.surfaces.size(); ++owner)
        {
            const Surface& surface = s.map.surfaces[owner];
            const RectangleF& r = surface.rectangle;
            for (int face = 0; face < 4; ++face)
            {
                if (!surface.faces[static_cast<size_t>(face)]) { continue; }
                const bool horizontal = face < 2, leading = face % 2 == 0;
                const float low = horizontal ? r.left() : r.top(), high = horizontal ? r.right() : r.bottom();
                const float edge = horizontal ? (leading ? r.top() : r.bottom()) : (leading ? r.left() : r.right());
                std::vector<float> cuts{ low, high };
                for (const Surface& blocker : s.map.surfaces)
                {
                    if (blocker.ramp()) { continue; }
                    for (const float value : { horizontal ? blocker.rectangle.left() : blocker.rectangle.top(),
                        horizontal ? blocker.rectangle.right() : blocker.rectangle.bottom() })
                    { if (value > low && value < high) { cuts.push_back(value); } }
                }
                std::sort(cuts.begin(), cuts.end());
                cuts.erase(std::unique(cuts.begin(), cuts.end()), cuts.end());
                for (size_t cut = 1; cut < cuts.size(); ++cut)
                {
                    const float middle = (cuts[cut - 1] + cuts[cut]) / 2.0f;
                    const float outward = edge + (leading ? -0.001f : 0.001f);
                    const Vector2F probe = horizontal ? Vector2F(middle, outward) : Vector2F(outward, middle);
                    const bool buried = std::any_of(s.map.surfaces.begin(), s.map.surfaces.end(),
                        [&](const Surface& blocker)
                        {
                            const RectangleF& box = blocker.rectangle;
                            return !blocker.ramp() && probe.x >= box.left() && probe.x <= box.right() &&
                                probe.y >= box.top() && probe.y <= box.bottom();
                        });
                    if (buried) { continue; }
                    const float start = cuts[cut - 1], length = cuts[cut] - start;
                    strips.push_back({ owner, face, horizontal ?
                        RectangleF(start, leading ? edge : edge - thickness, length, thickness) :
                        RectangleF(leading ? edge : edge - thickness, start, thickness, length) });
                }
            }
        }
        auto objects = s.builder.build_collision_objects(s.map.info.collision_objects_json(), s.colours);
        require(objects.size() == s.map.surfaces.size(), "Crossfeed score builder changed the authored object count");
        float total = 0.0f;
        for (size_t index = 0; index < objects.size(); ++index)
        {
            const auto* scoring = dynamic_cast<const PaintableObject*>(objects[index].get());
            if (scoring == nullptr) { continue; }
            float expected = 0.0f;
            for (const Strip& strip : strips)
            { if (strip.owner == index) { expected += strip.box.width * strip.box.height; } }
            const PaintTotal actual = scoring->paint_total();
            require(approximately(actual.total, expected, std::max(0.1f, expected * 0.00002f)),
                "Crossfeed runtime score differs from independently partitioned faces on '" + s.map.surfaces[index].name + "'");
            require(actual.team_a == 0.0f && actual.team_b == 0.0f, "Crossfeed starts with team-owned paint");
            total += actual.total;
            std::cout << "MEASURE Crossfeed scoring object=" << s.map.surfaces[index].name << " area=" << fixed(actual.total) << '\n';
        }
        require(total > 0.0f, "Crossfeed has no exposed scoring area");
        std::ofstream faces(output / "crossfeed-faces.csv"), regions(output / "crossfeed-paint.csv");
        faces << "object,face,x,y,width,height,area\n";
        constexpr std::array<const char*, 4> face_names{ "top", "bottom", "left", "right" };
        for (const Strip& strip : strips)
        {
            const RectangleF& r = strip.box;
            faces << s.map.surfaces[strip.owner].name << ',' << face_names[static_cast<size_t>(strip.face)] << ',' <<
                r.x << ',' << r.y << ',' << r.width << ',' << r.height << ',' << r.width * r.height << '\n';
        }
        regions << "region,top_area,bottom_area,vertical_area,total_area,share\n";
        constexpr std::array<const char*, 5> names{ "left_home", "left_contest", "centre", "right_contest", "right_home" };
        const std::array<float, 6> cuts{ bounds.left(), 920.0f, 1400.0f, 2200.0f, 2680.0f, bounds.right() };
        std::array<float, 5> areas{};
        std::array<float, 3> directions{};
        for (size_t region = 0; region < names.size(); ++region)
        {
            std::array<float, 3> oriented{};
            const RectangleF box(cuts[region], bounds.top(), cuts[region + 1] - cuts[region], bounds.height);
            for (const Strip& strip : strips)
            { oriented[static_cast<size_t>(std::min(strip.face, 2))] += area_in(strip.box, box); }
            for (size_t direction = 0; direction < oriented.size(); ++direction)
            { areas[region] += oriented[direction]; directions[direction] += oriented[direction]; }
            regions << names[region] << ',' << oriented[0] << ',' << oriented[1] << ',' << oriented[2] << ',' <<
                areas[region] << ',' << areas[region] / total << '\n';
            std::cout << "MEASURE Crossfeed region=" << names[region] << " area=" << fixed(areas[region]) <<
                " top=" << fixed(oriented[0]) << " underside=" << fixed(oriented[1]) << " vertical=" << fixed(oriented[2]) << '\n';
        }
        require(areas.front() == 0.0f && areas.back() == 0.0f, "Crossfeed homes contain safe scoring surface");
        require(approximately(areas[1], areas[3]), "Crossfeed left/right shared score differs");
        require(approximately(directions[0] + directions[1] + directions[2], total, 1.0f),
            "Crossfeed regional score does not reconcile with runtime tiles");
        require(faces.good() && regions.good(), "Cannot write Crossfeed measured face/region receipts");
        std::cout << "MEASURE Crossfeed total=" << fixed(total) << " top=" << fixed(directions[0]) <<
            " underside=" << fixed(directions[1]) << " vertical=" << fixed(directions[2]) << '\n';
    }
}
