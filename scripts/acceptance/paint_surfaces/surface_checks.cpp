#include "game/pch.h"
#include "scripts/acceptance/paint_surfaces/fixtures.h"
#include <array>

namespace paint_surfaces_acceptance
{
    namespace
    {
        using namespace mattmath;
        std::vector<std::uint32_t> visible_state(const StructurePaintable& object)
        {
            const PaintTotal total = object.paint_total();
            std::vector<std::uint32_t> state{ bits(total.total), bits(total.team_a), bits(total.team_b) };
            for (float area : total.player_areas) { state.push_back(bits(area)); }
            const auto rectangle = [&](const RectangleF& value)
            { for (float part : { value.x, value.y, value.width, value.height }) { state.push_back(bits(part)); } };
            rectangle(object.cull_bounds(0.5f));
            for (const PaintRunoffVisual& visual : object.runoff_visuals())
            {
                state.push_back(static_cast<std::uint32_t>(visual.team));
                for (float part : { visual.age, visual.source_y, visual.head_y }) { state.push_back(bits(part)); }
                rectangle(visual.owned_band); rectangle(visual.trail); rectangle(visual.bead);
            }
            return state;
        }
        void read_only(const StructurePaintable& object)
        {
            const auto samples = sample_state(snapshot(object));
            const auto before = visible_state(object);
            std::srand(1731u); const int expected = std::rand(); std::srand(1731u);
            for (int read = 0; read < 8; ++read)
            {
                require(sample_state(snapshot(object)) == samples, "Repeated tile snapshot changed values or ordering");
                same_total(grouped_total(snapshot(object)), object.paint_total());
            }
            require(std::rand() == expected && visible_state(object) == before,
                "Tile snapshot mutated scoring, cosmetic extent/runoff or global RNG");
            auto detached = snapshot(object);
            if (!detached.empty())
            {
                detached[0].corners[0].x += 100.0f;
                detached[0].paint_owner = paint_owners::count + 5;
                detached[0].scored_area = 0.0f;
                require(sample_state(snapshot(object)) == samples, "Snapshot values alias mutable tile state");
            }
        }
        void held_by(const StructurePaintable& object, PlayerTeam team, int owner)
        {
            const auto samples = snapshot(object);
            require(!samples.empty(), "Ownership control needs actual score tiles");
            for (const PaintSurfaceSample& sample : samples)
            { require(sample.team == team && sample.paint_owner == owner, "Snapshot lost actual captured/friendly/enemy tile ownership"); }
            same_total(grouped_total(samples), object.paint_total());
            read_only(object);
        }
    }

    void check_surfaces(World& world)
    {
        const RectangleF rectangle(0, 0, 100, 50);
        const float thickness = world.tuning.paint().thickness;
        const PaintableFaces all{ true, true, true, true }, top{ false, true, false, false };
        // Own shape, interior rectangle and a non-touching external neighbor do not bury a face.
        const RectangleF no_cover[]{ rectangle, { 20, 10, 60, 30 }, { 20, -10, 60, 9 } };
        auto object = world.surface(rectangle, all, no_cover);
        const auto neutral = snapshot(*object, 42);
        require(!neutral.empty(), "Four-face fixture did not generate actual tiles");
        std::array<float, 4> areas{};
        for (const PaintSurfaceSample& sample : neutral)
        {
            require(sample.team == PlayerTeam::none && sample.paint_owner == -1 && sample.source_index == 42,
                "Neutral score tile has invented ownership or lost source grouping");
            const RectangleF tile = static_rectangle(sample);
            area_near(sample.scored_area, tile.width * tile.height);
            const auto face = static_cast<std::size_t>(sample.face);
            require(face < areas.size(), "Snapshot invented an authored face");
            areas[face] += sample.scored_area;
            switch (sample.face)
            {
            case PaintSurfaceFace::top: require(tile.top() == rectangle.top(), "Top face identity changed"); break;
            case PaintSurfaceFace::bottom: require(tile.bottom() == rectangle.bottom(), "Bottom face identity changed"); break;
            case PaintSurfaceFace::left: require(tile.left() == rectangle.left(), "Left face identity changed"); break;
            case PaintSurfaceFace::right: require(tile.right() == rectangle.right(), "Right face identity changed"); break;
            }
        }
        area_near(areas[static_cast<std::size_t>(PaintSurfaceFace::top)], 100.0f * thickness);
        area_near(areas[static_cast<std::size_t>(PaintSurfaceFace::bottom)], 100.0f * thickness);
        area_near(areas[static_cast<std::size_t>(PaintSurfaceFace::left)], 50.0f * thickness);
        area_near(areas[static_cast<std::size_t>(PaintSurfaceFace::right)], 50.0f * thickness);
        area_near(object->paint_total().total, 300.0f * thickness); // Face corner areas score independently.
        same_total(grouped_total(neutral), object->paint_total());
        read_only(*object);
        // Appending preserves the caller's prefix and uses its supplied source ordinal.
        std::vector<PaintSurfaceSample> appended{ neutral[0] };
        const auto prefix = sample_state(appended);
        object->append_paint_surfaces(appended, 77);
        require(appended.size() == neutral.size() + 1 &&
            sample_state(std::vector<PaintSurfaceSample>{ appended[0] }) == prefix,
            "Role collection replaced the caller's existing output");
        for (std::size_t index = 1; index < appended.size(); ++index)
        { require(appended[index].source_index == 77, "Appended tile has the wrong source ordinal"); }
        const RectangleF full(-100, -100, 300, 300);
        const int capturing = paint_owners::index(-1, 0), friendly = paint_owners::index(-1, 2), enemy = paint_owners::index(-1, 3);
        world.paint(*object, full, PlayerTeam::a, capturing); held_by(*object, PlayerTeam::a, capturing);
        world.paint(*object, full, PlayerTeam::a, friendly); held_by(*object, PlayerTeam::a, capturing);
        world.paint(*object, full, PlayerTeam::b, enemy); held_by(*object, PlayerTeam::b, enemy);
        world.paint(*object, full, PlayerTeam::a, -1); held_by(*object, PlayerTeam::a, -1);
        for (float area : object->paint_total().player_areas)
        { require(area == 0.0f, "Owned-but-uncredited tile was relabelled neutral or assigned to a player"); }
        require(object->paint_total().team_a == object->paint_total().total,
            "Owned-but-uncredited area disappeared from team coverage");

        // Duplicate/overlapping cover merges; fitted fractional tiles retain exact scored area.
        const RectangleF overlapping[]{ { 20.5f, -10, 30.25f, 10 }, { 40, -10, 30, 10 }, { 40, -10, 30, 10 } };
        auto split = world.surface(rectangle, top, overlapping);
        const auto clipped = snapshot(*split);
        bool fractional = false;
        for (const PaintSurfaceSample& sample : clipped)
        {
            const RectangleF tile = static_rectangle(sample);
            require(sample.face == PaintSurfaceFace::top && tile.top() == rectangle.top() &&
                (tile.right() <= 20.5001f || tile.left() >= 69.9999f), "Snapshot included buried or wrong-face area");
            fractional = fractional || std::floor(tile.width) != tile.width;
        }
        require(fractional, "Fractional clipping control did not retain a fractional scored tile");
        area_near(grouped_total(clipped).total, 50.5f * thickness);
        same_total(grouped_total(clipped), split->paint_total());
        world.paint(*split, RectangleF(30, 0, 4, thickness), PlayerTeam::a, capturing);
        require(split->paint_total().team_a == 0.0f, "Buried-only contact captured phantom tiles");
        world.paint(*split, full, PlayerTeam::a, capturing); held_by(*split, PlayerTeam::a, capturing);
        const RectangleF surrounding[]{ { -10, -10, 120, 70 } };
        auto buried = world.surface(rectangle, all, surrounding);
        require(snapshot(*buried).empty(), "Completely buried object exposed authored capacity as scored samples");
        same_total(grouped_total(snapshot(*buried)), buried->paint_total());
        read_only(*buried);
    }
}
