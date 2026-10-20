#include "game/pch.h"
#include "scripts/acceptance/cq_blockout/fixtures.h"

namespace cq_blockout_acceptance
{
    namespace
    {
        bool same_rectangle(const RectangleF& first, const RectangleF& second)
        {
            return approximately(first.x, second.x) && approximately(first.y, second.y) &&
                approximately(first.width, second.width) && approximately(first.height, second.height);
        }

        std::string reflected_type(const Surface& surface)
        {
            if (surface.collision_type == "STRUCTURE_RAMP_LEFT") { return "STRUCTURE_RAMP_RIGHT"; }
            if (surface.collision_type == "STRUCTURE_RAMP_RIGHT") { return "STRUCTURE_RAMP_LEFT"; }
            return surface.collision_type;
        }

        struct Strip
        {
            const Surface* owner;
            const char* face;
            RectangleF rectangle;
        };

        float clipped_area(const RectangleF& first, const RectangleF& region)
        {
            return std::max(0.0f, std::min(first.right(), region.right()) -
                std::max(first.left(), region.left())) *
                std::max(0.0f, std::min(first.bottom(), region.bottom()) -
                    std::max(first.top(), region.top()));
        }
    }

    void geometry_checks(Services& s, const std::filesystem::path& output)
    {
        const RectangleF bounds = s.map.info.out_of_bounds_rectangle();
        size_t paired = 0;
        for (const Surface& surface : s.map.surfaces)
        {
            // World boundaries are not route geometry; scenery never enters
            // this collision list. Every interior playable piece must pair.
            const RectangleF& r = surface.rectangle;
            if (r.right() <= bounds.left() || r.left() >= bounds.right() ||
                (r.left() <= bounds.left() && r.right() >= bounds.right())) { continue; }
            const RectangleF expected = mirror(r, bounds);
            const std::array<bool, 4> faces{ surface.faces[0], surface.faces[1],
                surface.faces[3], surface.faces[2] };
            size_t matches = 0;
            for (const Surface& other : s.map.surfaces)
            {
                if (other.type == surface.type && other.collision_type == reflected_type(surface) &&
                    other.faces == faces && same_rectangle(other.rectangle, expected)) { ++matches; }
            }
            require(matches == 1, "CQ gameplay geometry '" + surface.name +
                "' needs exactly one reflected collision/paint-face counterpart");
            ++paired;
        }
        require(paired >= 10, "CQ symmetry check did not exercise the blockout's gameplay pieces");

        // A one-way collider permits rising through it, but a centred resting
        // body should not remain embedded in another platform's artwork.
        // This is standing clearance only; driven trials establish the routes.
        size_t centred_landings = 0;
        for (const Surface& support : s.map.surfaces)
        {
            if (!support.jump_through()) { continue; }
            const RectangleF body(standing_on(s, support, support.rectangle.center().x),
                s.tuning.player().size);
            for (const Surface& other : s.map.surfaces)
            {
                if (&other == &support || other.ramp()) { continue; }
                const auto overlap = narrow_phase(body, other.rectangle);
                require(!overlap || overlap->penetration <= tolerance,
                    "CQ centred standing body on '" + support.name + "' intersects '" + other.name + "'");
            }
            ++centred_landings;
        }
        require(centred_landings > 0, "CQ standing clearance check exercised no one-way platforms");
        std::cout << "PASS CQ " << centred_landings << " centred standing clearances, including one-way artwork\n";

        const auto a = s.map.info.team_a_spawns();
        const auto b = s.map.info.team_b_spawns();
        require(a.size() == 4 && b.size() == 4, "CQ first blockout needs exactly four authored spawns per team");
        for (size_t index = 0; index < a.size(); ++index)
        {
            const RectangleF reflected = mirror(RectangleF(a[index], s.tuning.player().size), bounds);
            require(same_rectangle(reflected, RectangleF(b[index], s.tuning.player().size)),
                "CQ spawn indices do not reflect complete player bodies");
        }

        // Reconstruct the editor's exposed strips from authored face flags,
        // then reconcile each object with the runtime's actual scored tiles.
        // Ramps do not bury score; jump-through flat structures do.
        std::vector<PaintFaceRectangle> blockers;
        for (const Surface& surface : s.map.surfaces)
        {
            if (surface.ramp()) { continue; }
            const RectangleF& r = surface.rectangle;
            blockers.push_back({ r.left(), r.top(), r.right(), r.bottom() });
        }
        std::vector<Strip> strips;
        const float thickness = s.tuning.paint().thickness;
        for (const Surface& surface : s.map.surfaces)
        {
            const RectangleF& r = surface.rectangle;
            for (int face = 0; face < 4; ++face)
            {
                if (!surface.faces[static_cast<size_t>(face)]) { continue; }
                const bool along_x = face < 2;
                const bool leading = face % 2 == 0;
                const float edge = along_x ? (leading ? r.top() : r.bottom()) :
                    (leading ? r.left() : r.right());
                for (const auto& interval : exposed_paint_intervals(along_x ? r.left() : r.top(),
                    along_x ? r.right() : r.bottom(), edge, along_x, leading, blockers))
                {
                    const RectangleF strip = along_x ?
                        RectangleF(interval.first, leading ? edge : edge - thickness,
                            interval.second - interval.first, thickness) :
                        RectangleF(leading ? edge : edge - thickness, interval.first,
                            thickness, interval.second - interval.first);
                    static constexpr const char* names[]{ "top", "bottom", "left", "right" };
                    strips.push_back({ &surface, names[face], strip });
                }
            }
        }
        auto objects = s.builder.build_collision_objects(s.map.info.collision_objects_json(), s.colours);
        float runtime_total = 0.0f;
        for (size_t index = 0; index < objects.size(); ++index)
        {
            const auto* scoring = dynamic_cast<const PaintableObject*>(objects[index].get());
            if (scoring == nullptr) { continue; }
            const PaintTotal total = scoring->paint_total();
            float expected = 0.0f;
            for (const Strip& strip : strips)
            {
                if (strip.owner == &s.map.surfaces[index])
                {
                    expected += strip.rectangle.width * strip.rectangle.height;
                }
            }
            require(approximately(total.total, expected, std::max(0.1f, expected * 0.00002f)),
                "CQ exposed-strip area differs from actual runtime tiles on '" + s.map.surfaces[index].name + "'");
            require(total.team_a == 0.0f && total.team_b == 0.0f,
                "CQ scoring geometry starts with team-owned paint");
            runtime_total += total.total;
            std::cout << "MEASURE CQ scoring object=" << s.map.surfaces[index].name <<
                " runtime_area=" << fixed(total.total) << '\n';
        }
        require(runtime_total > 0.0f, "CQ has no exposed runtime scoring area");

        std::ofstream faces(output / "cq_blockout-faces.csv");
        faces << "object,face,x,y,width,height,area\n";
        for (const Strip& strip : strips)
        {
            const RectangleF& r = strip.rectangle;
            faces << strip.owner->name << ',' << strip.face << ',' << r.x << ',' << r.y << ',' <<
                r.width << ',' << r.height << ',' << r.width * r.height << '\n';
        }
        std::ofstream regions(output / "cq_blockout-paint.csv");
        regions << "region,left,top,width,height,top_area,bottom_area,vertical_area,exposed_area,share\n";
        const std::array<const char*, 5> names{ "left_home", "left_shoulder", "centre", "right_shoulder", "right_home" };
        const std::array<float, 6> cuts{ bounds.left(), 1500.0f, 2500.0f, 3500.0f, 4500.0f, bounds.right() };
        std::array<float, 5> areas{};
        std::array<float, 3> orientation_totals{};
        float regional_total = 0.0f;
        for (size_t region = 0; region < names.size(); ++region)
        {
            const RectangleF box(cuts[region], bounds.top(), cuts[region + 1] - cuts[region], bounds.height);
            std::array<float, 3> oriented{};
            for (const Strip& strip : strips)
            {
                const float area = clipped_area(strip.rectangle, box);
                const size_t orientation = strip.face[0] == 't' ? 0u : strip.face[0] == 'b' ? 1u : 2u;
                areas[region] += area;
                oriented[orientation] += area;
                orientation_totals[orientation] += area;
            }
            regional_total += areas[region];
            regions << names[region] << ',' << box.x << ',' << box.y << ',' << box.width << ',' << box.height << ',' <<
                oriented[0] << ',' << oriented[1] << ',' << oriented[2] << ',' <<
                areas[region] << ',' << areas[region] / runtime_total << '\n';
            std::cout << "MEASURE CQ paint " << names[region] << " area=" << fixed(areas[region]) <<
                " top=" << fixed(oriented[0]) << " underside=" << fixed(oriented[1]) <<
                " vertical=" << fixed(oriented[2]) <<
                " share=" << fixed(100.0f * areas[region] / runtime_total) << "%\n";
        }
        regions << "TOTAL," << bounds.x << ',' << bounds.y << ',' << bounds.width << ',' << bounds.height << ',' <<
            orientation_totals[0] << ',' << orientation_totals[1] << ',' << orientation_totals[2] << ',' <<
            regional_total << ",1\n";
        std::cout << "MEASURE CQ face orientations top=" << fixed(orientation_totals[0]) <<
            " underside=" << fixed(orientation_totals[1]) << " vertical=" << fixed(orientation_totals[2]) << '\n';
        require(approximately(regional_total, runtime_total, std::max(0.2f, runtime_total * 0.00002f)),
            "CQ region partition does not reconcile with runtime scoring total");
        require(approximately(areas[0], areas[4], 0.1f) && approximately(areas[1], areas[3], 0.1f),
            "CQ reflected regions have different scoring budgets");
        require(areas[0] == 0.0f && areas[4] == 0.0f,
            "CQ sheltered home rooms contain unopposed scoring surface");
        require(faces.good() && regions.good(), "cannot write CQ paint-budget CSV evidence");
        std::cout << "PASS CQ geometry: " << paired << " reflected gameplay pieces, four mirrored full-body spawn pairs; "
            "runtime exposed paint=" << fixed(runtime_total) << "; per-face and regional CSVs written\n";
    }
}
