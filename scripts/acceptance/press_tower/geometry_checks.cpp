#include "game/pch.h"
#include "scripts/acceptance/press_tower/fixtures.h"
#include <algorithm>

namespace press_tower_acceptance
{
    namespace
    {
        // Elevation tiers from the blueprint's high-ground trade table.
        enum Tier : int { tier_none = -1, tier_floor_1280, tier_bed_1200, tier_rulers_1120, tier_stairs,
            tier_frames_1040, tier_rulers_960, tier_high };
        constexpr std::array<const char*, 7> tier_names{ "floor_1280", "bed_1200", "rulers_1120", "stairs",
            "frames_1040", "rulers_960", "high" };
        // Exposed length per tier, from the blueprint (strip-independent).
        constexpr std::array<float, 7> tier_lengths{ 2640.0f, 640.0f, 2000.0f, 2860.0f, 320.0f, 2000.0f, 1240.0f };

        // One authored piece as the blueprint states it. `exposed` is the
        // expected scoring length per face (top, bottom, left, right) after
        // burial; it is a hand-derived constant, not a runtime reading.
        struct Piece
        {
            std::string name;
            std::string type;
            std::string collision_type;
            RectangleF rectangle;
            std::array<bool, 4> faces{};
            std::array<float, 4> exposed{};
            Tier tier = tier_none;
        };

        constexpr std::array<bool, 4> no_faces{ false, false, false, false };
        constexpr std::array<bool, 4> top_only{ true, false, false, false };
        constexpr std::array<bool, 4> all_faces{ true, true, true, true };

        std::vector<Piece> blueprint_pieces(const RectangleF& bounds)
        {
            const std::string paintable = "StructurePaintable";
            const std::vector<Piece> a_side{
                { "boundary_wall_left", "Structure", "STRUCTURE", { -80, 0, 80, 1680 }, no_faces, {}, tier_none },
                { "floor_home_a", paintable, "STRUCTURE_PAINTABLE", { 0, 1280, 520, 400 }, top_only, { 520, 0, 0, 0 }, tier_floor_1280 },
                { "floor_yard_a", paintable, "STRUCTURE_PAINTABLE", { 520, 1280, 800, 400 }, top_only, { 800, 0, 0, 0 }, tier_floor_1280 },
                { "home_floor_a", paintable, "STRUCTURE_JUMP_THROUGH", { 0, 1120, 520, 10 }, top_only, { 520, 0, 0, 0 }, tier_rulers_1120 },
                { "home_roof_a", paintable, "STRUCTURE_JUMP_THROUGH", { 0, 960, 520, 10 }, top_only, { 520, 0, 0, 0 }, tier_rulers_960 },
                { "catwalk_a", paintable, "STRUCTURE_JUMP_THROUGH", { 520, 1120, 480, 10 }, top_only, { 480, 0, 0, 0 }, tier_rulers_1120 },
                { "gantry_a", paintable, "STRUCTURE_JUMP_THROUGH", { 520, 960, 480, 10 }, top_only, { 480, 0, 0, 0 }, tier_rulers_960 },
                { "stair_1_a", paintable, "STRUCTURE_PAINTABLE", { 1000, 1040, 160, 80 }, all_faces, { 160, 160, 80, 80 }, tier_stairs },
                { "stair_2_a", paintable, "STRUCTURE_PAINTABLE", { 1160, 960, 160, 80 }, all_faces, { 160, 160, 80, 80 }, tier_stairs },
                // The shoulder's left end buries Y 880-890 of the right face.
                { "stair_3_a", paintable, "STRUCTURE_PAINTABLE", { 1320, 880, 160, 80 }, all_faces, { 160, 160, 80, 70 }, tier_stairs },
                { "frame_a", paintable, "STRUCTURE_JUMP_THROUGH", { 1480, 1040, 160, 10 }, top_only, { 160, 0, 0, 0 }, tier_frames_1040 },
                { "shoulder_a", paintable, "STRUCTURE_JUMP_THROUGH", { 1480, 880, 160, 10 }, top_only, { 160, 0, 0, 0 }, tier_high },
                // Triangle (1320,1280)(1480,1280)(1480,1200); bounds are its box.
                { "ramp_a", "StructureRamp", "STRUCTURE_RAMP_RIGHT", { 1320, 1200, 160, 80 }, no_faces, {}, tier_none },
            };
            std::vector<Piece> pieces{
                { "press_base", "Structure", "STRUCTURE", { 1320, 1280, 960, 400 }, no_faces, {}, tier_none },
                { "press_bed", paintable, "STRUCTURE_PAINTABLE", { 1480, 1200, 640, 80 }, top_only, { 640, 0, 0, 0 }, tier_bed_1200 },
                // The crown buries X 1780-1820 of the head's top.
                { "press_head", paintable, "STRUCTURE_PAINTABLE", { 1640, 800, 320, 80 }, all_faces, { 280, 320, 80, 80 }, tier_high },
                { "press_crown", paintable, "STRUCTURE_PAINTABLE", { 1780, 740, 40, 60 },
                    { true, false, true, true }, { 40, 0, 60, 60 }, tier_high },
            };
            for (const Piece& piece : a_side)
            {
                pieces.push_back(piece);
                Piece b = piece;
                b.name = piece.name == "boundary_wall_left" ? "boundary_wall_right" :
                    piece.name.substr(0, piece.name.size() - 2) + "_b";
                b.rectangle = mirror(piece.rectangle, bounds);
                b.faces = { piece.faces[0], piece.faces[1], piece.faces[3], piece.faces[2] };
                b.exposed = { piece.exposed[0], piece.exposed[1], piece.exposed[3], piece.exposed[2] };
                if (piece.collision_type == "STRUCTURE_RAMP_RIGHT") { b.collision_type = "STRUCTURE_RAMP_LEFT"; }
                pieces.push_back(std::move(b));
            }
            return pieces;
        }

        std::string reflected_type(const Surface& surface)
        {
            if (surface.collision_type == "STRUCTURE_RAMP_LEFT") { return "STRUCTURE_RAMP_RIGHT"; }
            if (surface.collision_type == "STRUCTURE_RAMP_RIGHT") { return "STRUCTURE_RAMP_LEFT"; }
            return surface.collision_type;
        }

        struct Strip { size_t owner; int face; RectangleF box; };

        float area_in(const RectangleF& a, const RectangleF& b)
        {
            return std::max(0.0f, std::min(a.right(), b.right()) - std::max(a.left(), b.left())) *
                std::max(0.0f, std::min(a.bottom(), b.bottom()) - std::max(a.top(), b.top()));
        }

        float strip_length(const Strip& strip)
        {
            return strip.face < 2 ? strip.box.width : strip.box.height;
        }

        struct Sightlines
        {
            // Every flat Structure/StructurePaintable blocks paint, one-way
            // rulers included; ramps let it through and visuals are ignored.
            std::vector<RectangleF> blockers;

            explicit Sightlines(const Map& map)
            {
                for (const Surface& surface : map.surfaces)
                {
                    if (!surface.ramp()) { this->blockers.push_back(surface.rectangle); }
                }
            }

            bool clear(const Vector2F& from, const Vector2F& to) const
            {
                return std::none_of(this->blockers.begin(), this->blockers.end(),
                    [&](const RectangleF& r) { return segment_hits(from, to, r); });
            }

            // First clear muzzle/target pair, if any.
            std::optional<std::pair<Vector2F, Vector2F>> first_clear(const std::vector<Vector2F>& muzzles,
                const std::vector<Vector2F>& targets) const
            {
                for (const Vector2F& muzzle : muzzles)
                {
                    for (const Vector2F& target : targets)
                    {
                        if (this->clear(muzzle, target)) { return std::make_pair(muzzle, target); }
                    }
                }
                return std::nullopt;
            }

            int visible_targets(const std::vector<Vector2F>& muzzles, const std::vector<Vector2F>& targets) const
            {
                int visible = 0;
                for (const Vector2F& target : targets)
                {
                    visible += std::any_of(muzzles.begin(), muzzles.end(),
                        [&](const Vector2F& muzzle) { return this->clear(muzzle, target); }) ? 1 : 0;
                }
                return visible;
            }
        };

        // Muzzles every 4 units across a standing span, at the given heights
        // above the feet: the blueprint's fixed-muzzle point model, which
        // checks the geometry rather than any one tool. Real nozzles sit 62.5
        // to 110 from the body's centre along the aim, and real paint is a box
        // that may fall; live_fire_checks fires the actual tools for that.
        std::vector<Vector2F> muzzles(float low, float high, float feet, std::initializer_list<float> rises)
        {
            std::vector<Vector2F> points;
            for (float x = std::min(low, high); x <= std::max(low, high) + 0.001f; x += 4.0f)
            {
                for (const float rise : rises) { points.emplace_back(x, feet - rise); }
            }
            return points;
        }

        // Cell centres no wider than 8, so a surface and its mirror image
        // sample mirrored points and the two teams' claims are comparable.
        std::vector<Vector2F> top_points(const Surface& surface)
        {
            const RectangleF& r = surface.rectangle;
            const int cells = static_cast<int>(std::ceil(r.width / 8.0f));
            std::vector<Vector2F> points;
            for (int cell = 0; cell < cells; ++cell)
            {
                points.emplace_back(r.left() + (cell + 0.5f) * r.width / cells, r.top() - 0.5f);
            }
            return points;
        }

        void append(std::vector<Vector2F>& into, const std::vector<Vector2F>& more)
        {
            into.insert(into.end(), more.begin(), more.end());
        }

        std::string point_text(const Vector2F& point)
        {
            return '(' + fixed(point.x) + ',' + fixed(point.y) + ')';
        }

        void require_blind(const Sightlines& lines, const std::string& claim,
            const std::vector<Vector2F>& from, const std::vector<Vector2F>& to)
        {
            require(!from.empty() && !to.empty(), "Press Tower sightline claim '" + claim + "' sampled nothing");
            const auto pair = lines.first_clear(from, to);
            require(!pair, "Press Tower sightline claim '" + claim + "' fails: muzzle " +
                (pair ? point_text(pair->first) + " sees " + point_text(pair->second) : std::string()));
            std::cout << "MEASURE Press Tower sightline " << claim << " muzzles=" << from.size() <<
                " targets=" << to.size() << " clear_pairs=0\n";
        }

        void sightline_checks(const Services& s)
        {
            const Sightlines lines(s.map);
            const Surface& head = s.map.named("press_head");
            const Surface& crown = s.map.named("press_crown");
            const std::initializer_list<float> standing{ 60.0f, 80.0f };
            // Summit halves are the head's top either side of the crown.
            const std::vector<Vector2F> west = muzzles(head.rectangle.left(), crown.rectangle.left(), head.rectangle.top(), standing);
            const std::vector<Vector2F> east = muzzles(crown.rectangle.right(), head.rectangle.right(), head.rectangle.top(), standing);
            const std::vector<Vector2F> on_crown = muzzles(crown.rectangle.left(), crown.rectangle.right(), crown.rectangle.top(), standing);
            std::vector<Vector2F> summit = west;
            append(summit, east);
            append(summit, on_crown);
            for (const bool right : { true, false })
            {
                const char team = right ? 'a' : 'b';
                const auto side = [&](const std::string& base) { return base + (right ? "_a" : "_b"); };
                const auto points = right ? s.map.info.team_a_spawns() : s.map.info.team_b_spawns();
                float left = points.front().x, top = points.front().y, far_right = left, bottom = top;
                for (const Vector2F& point : points)
                {
                    left = std::min(left, point.x);
                    far_right = std::max(far_right, point.x + s.tuning.player().size.x);
                    top = std::min(top, point.y);
                    bottom = std::max(bottom, point.y + s.tuning.player().size.y);
                }
                // The whole home span the four spawn bodies occupy, not just
                // the bodies, so the claim covers players shuffling in place.
                std::vector<Vector2F> spawn;
                for (float x = left; x <= far_right + 0.001f; x += 4.0f)
                {
                    for (float y = top + 2.0f; y <= bottom - 2.0f + 0.001f; y += 6.0f) { spawn.emplace_back(x, y); }
                }
                const Surface& shoulder = s.map.named(side("shoulder"));
                const Surface& stair = s.map.named(side("stair_3"));
                const std::string to_spawn = std::string("->spawn_") + team;
                require_blind(lines, std::string("summit_west") + to_spawn, west, spawn);
                require_blind(lines, std::string("summit_east") + to_spawn, east, spawn);
                require_blind(lines, std::string("crown") + to_spawn, on_crown, spawn);
                require_blind(lines, side("shoulder") + to_spawn,
                    muzzles(shoulder.rectangle.left(), shoulder.rectangle.right(), shoulder.rectangle.top(), standing), spawn);
                require_blind(lines, side("stair_3") + to_spawn,
                    muzzles(stair.rectangle.left(), stair.rectangle.right(), stair.rectangle.top(), standing), spawn);
                for (const std::string& name : { side("floor_yard"), side("floor_home"), std::string("press_bed"), side("frame") })
                {
                    const std::vector<Vector2F> targets = top_points(s.map.named(name));
                    require_blind(lines, "summit->" + name + "_top", summit, targets);
                    // The blueprint's claim covers a jumping holder too: muzzles
                    // from a tap's height to above a held jump's apex.
                    require_blind(lines, "summit_jumping->" + name + "_top", muzzles(head.rectangle.left(),
                        head.rectangle.right(), head.rectangle.top(), { 150.0f, 250.0f, 320.0f }), targets);
                }
                const Surface& yard = s.map.named(side("floor_yard"));
                // The whole yard top at 4-unit steps, both ends included.
                const std::vector<Vector2F> yard_muzzles = muzzles(yard.rectangle.left(), yard.rectangle.right(),
                    yard.rectangle.top(), standing);
                const std::string far_yard = right ? "floor_yard_b" : "floor_yard_a";
                require_blind(lines, side("floor_yard") + "->" + far_yard + "_top", yard_muzzles,
                    top_points(s.map.named(far_yard)));
                // Recorded visibility from the holder's own half and the yard.
                const std::vector<Vector2F>& own_half = right ? west : east;
                for (const std::string& name : { side("home_roof"), side("gantry"), side("catwalk"),
                    side("home_floor"), side("stair_3"), side("shoulder") })
                {
                    const std::vector<Vector2F> targets = top_points(s.map.named(name));
                    std::cout << "MEASURE Press Tower sightline own_half_team_" << team << "->" << name <<
                        "_top visible=" << lines.visible_targets(own_half, targets) << '/' << targets.size() << '\n';
                }
                const std::vector<Vector2F> bed = top_points(s.map.named("press_bed"));
                std::cout << "MEASURE Press Tower sightline " << side("floor_yard") << "->press_bed_top visible=" <<
                    lines.visible_targets(yard_muzzles, bed) << '/' << bed.size() << '\n';
            }
            std::cout << "PASS Press Tower sightline claims: no summit, crown, shoulder or stair_3 muzzle sees a "
                "spawn body; the summit, standing or jumping, sees no floor, bed or frame top; neither yard sees "
            "the far yard\n";
        }

        void standing_checks(const Services& s)
        {
            size_t checked = 0;
            for (const Surface& support : s.map.surfaces)
            {
                // Perimeter walls are not walking floors; the base is covered.
                if (support.ramp() || support.name == "press_base" ||
                    support.name == "boundary_wall_left" || support.name == "boundary_wall_right") { continue; }
                std::vector<float> centres{ support.rectangle.center().x };
                // The crown occupies the head's centre: test the two halves.
                if (support.name == "press_head") { centres = { 1710.0f, 1890.0f }; }
                if (support.name == "press_bed") { centres = { 1800.0f, 1560.0f, 2040.0f }; }
                for (const float centre : centres)
                {
                    const RectangleF body(standing_on(s, support, centre), s.tuning.player().size);
                    for (const Surface& other : s.map.surfaces)
                    {
                        if (&other == &support || other.ramp()) { continue; }
                        const auto overlap = narrow_phase(body, other.rectangle);
                        require(!overlap || overlap->penetration <= tolerance,
                            "Press Tower standing body on '" + support.name + "' at x=" + fixed(centre) +
                            " intersects '" + other.name + "'");
                    }
                    ++checked;
                }
            }

            // Stations are developer `goto` targets; each must be a resting
            // body on its named support with no solid inside it.
            const RectangleF bounds = s.map.info.out_of_bounds_rectangle();
            struct Station { std::string name; Vector2F position; std::string support; };
            const std::vector<Station> a_side{
                { "press_head_a", { 1684, 680 }, "press_head" }, { "shoulder_a", { 1534, 760 }, "shoulder_a" },
                { "frame_a", { 1534, 920 }, "frame_a" }, { "stair_3_a", { 1374, 760 }, "stair_3_a" },
                { "stair_2_a", { 1214, 840 }, "stair_2_a" }, { "gantry_a", { 734, 840 }, "gantry_a" },
                { "catwalk_a", { 894, 1000 }, "catwalk_a" }, { "yard_a", { 1214, 1160 }, "floor_yard_a" },
            };
            std::vector<Station> expected{ { "crown", { 1774, 620 }, "press_crown" },
                { "press_bed", { 1774, 1080 }, "press_bed" } };
            for (const Station& station : a_side)
            {
                expected.push_back(station);
                const std::string support = station.support == "press_head" ? station.support :
                    station.support.substr(0, station.support.size() - 2) + "_b";
                expected.push_back({ station.name.substr(0, station.name.size() - 2) + "_b",
                    mirror(RectangleF(station.position, s.tuning.player().size), bounds).top_left(), support });
            }
            const auto authored = s.map.info.stations();
            require(authored.size() == expected.size() + 2,
                "Press Tower needs exactly " + std::to_string(expected.size()) + " authored stations");
            for (const Station& station : expected)
            {
                const auto found = std::find_if(authored.begin(), authored.end(),
                    [&](const LevelStation& item) { return item.name == station.name; });
                require(found != authored.end(), "Press Tower lacks station '" + station.name + "'");
                require(approximately(found->position.x, station.position.x) &&
                    approximately(found->position.y, station.position.y),
                    "Press Tower station '" + station.name + "' is not at its blueprint position");
                const RectangleF body(found->position, s.tuning.player().size);
                const Surface& support = s.map.named(station.support);
                require(approximately(body.bottom(), support.rectangle.top()) &&
                    body.right() > support.rectangle.left() && body.left() < support.rectangle.right(),
                    "Press Tower station '" + station.name + "' does not rest on '" + station.support + "'");
                for (const Surface& other : s.map.surfaces)
                {
                    if (other.ramp()) { continue; }
                    const auto overlap = narrow_phase(body, other.rectangle);
                    require(!overlap || overlap->penetration <= tolerance,
                        "Press Tower station '" + station.name + "' intersects '" + other.name + "'");
                }
                ++checked;
            }
            std::cout << "PASS Press Tower " << checked << " standing clearances: every support top, both "
                "summit halves, three bed positions and all " << expected.size() << " stations\n";
        }
    }

    void geometry_checks(Services& s, const std::filesystem::path& output)
    {
        const RectangleF bounds = s.map.info.out_of_bounds_rectangle();
        require(bounds == RectangleF(-100, -100, 3800, 1900), "Press Tower out_of_bounds is not the blueprint's");
        require(s.map.info.camera_bounds_rectangle() == RectangleF(0, 380, 3600, 1000) &&
            s.map.info.zoom_out_start_bounds_rectangle() == RectangleF(0, 380, 1920, 1080) &&
            s.map.info.zoom_out_finish_bounds_rectangle() == RectangleF(0, 380, 3600, 1000),
            "Press Tower camera/overview bounds are not the blueprint's");

        // 1. Validated authored pieces against the blueprint table.
        const std::vector<Piece> pieces = blueprint_pieces(bounds);
        require(pieces.size() == 30 && s.map.surfaces.size() == pieces.size(),
            "Press Tower needs exactly 30 collision objects; found " + std::to_string(s.map.surfaces.size()));
        require(s.map.info.non_collision_objects_json().size() == 17, "Press Tower needs exactly 17 visuals; found " +
            std::to_string(s.map.info.non_collision_objects_json().size()));
        for (const Piece& piece : pieces)
        {
            const Surface& surface = s.map.named(piece.name);
            require(surface.type == piece.type && surface.collision_type == piece.collision_type,
                "Press Tower '" + piece.name + "' has type " + surface.type + '/' + surface.collision_type +
                ", blueprint " + piece.type + '/' + piece.collision_type);
            require(surface.rectangle == piece.rectangle, "Press Tower '" + piece.name + "' is at (" +
                fixed(surface.rectangle.x) + ',' + fixed(surface.rectangle.y) + ',' + fixed(surface.rectangle.width) +
                ',' + fixed(surface.rectangle.height) + "), not its blueprint rectangle");
            require(surface.faces == piece.faces, "Press Tower '" + piece.name + "' has non-blueprint paintable faces");
        }
        auto objects = s.builder.build_collision_objects(s.map.info.collision_objects_json(), s.colours);
        require(objects.size() == s.map.surfaces.size(), "Press Tower builder changed the authored object count");
        for (size_t index = 0; index < objects.size(); ++index)
        {
            require(objects[index]->bounds() == s.map.surfaces[index].rectangle,
                "Press Tower runtime bounds differ from authored '" + s.map.surfaces[index].name + "'");
        }

        // 2. Mirror pairs, by geometry rather than by name.
        for (const Surface& surface : s.map.surfaces)
        {
            const RectangleF expected = mirror(surface.rectangle, bounds);
            const std::array<bool, 4> faces{ surface.faces[0], surface.faces[1], surface.faces[3], surface.faces[2] };
            const auto matches = std::count_if(s.map.surfaces.begin(), s.map.surfaces.end(), [&](const Surface& other)
                {
                    return other.type == surface.type && other.collision_type == reflected_type(surface) &&
                        other.faces == faces && other.rectangle == expected;
                });
            require(matches == 1, "Press Tower '" + surface.name + "' needs exactly one mirrored counterpart");
        }
        for (const char* centre : { "press_base", "press_bed", "press_head", "press_crown" })
        {
            const RectangleF& r = s.map.named(centre).rectangle;
            require(mirror(r, bounds) == r, std::string("Press Tower centre piece '") + centre + "' is not self-mirrored");
        }

        // 3. Spawns: blueprint positions, mirrored bodies, no same-team overlap.
        const auto a = s.map.info.team_a_spawns(), b = s.map.info.team_b_spawns();
        require(a.size() == 4 && b.size() == 4, "Press Tower needs four full-body spawns per team");
        const std::array<float, 4> spawn_x{ 30.0f, 150.0f, 270.0f, 390.0f };
        for (size_t index = 0; index < a.size(); ++index)
        {
            require(approximately(a[index].x, spawn_x[index]) && approximately(a[index].y, 1000.0f),
                "Press Tower team A spawn " + std::to_string(index) + " is not at its blueprint position");
            require(mirror(RectangleF(a[index], s.tuning.player().size), bounds) ==
                RectangleF(b[index], s.tuning.player().size), "Press Tower spawn pairs do not mirror complete bodies");
            for (size_t other = index + 1; other < a.size(); ++other)
            {
                require(!narrow_phase(RectangleF(a[index], s.tuning.player().size), RectangleF(a[other], s.tuning.player().size)) &&
                    !narrow_phase(RectangleF(b[index], s.tuning.player().size), RectangleF(b[other], s.tuning.player().size)),
                    "Press Tower same-team spawn bodies overlap");
            }
        }

        // 4. An independent face partition: split at every authored blocker
        // edge, then probe outward from each segment's midpoint. Do not call
        // the runtime/editor interval-clipping helper to predict its own result.
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
        constexpr std::array<const char*, 4> face_names{ "top", "bottom", "left", "right" };
        float total_length = 0.0f;
        std::array<float, 7> tiers{};
        std::array<float, 3> orientation_lengths{};
        for (const Piece& piece : pieces)
        {
            const Surface& surface = s.map.named(piece.name);
            const size_t owner = static_cast<size_t>(&surface - s.map.surfaces.data());
            for (int face = 0; face < 4; ++face)
            {
                float length = 0.0f;
                for (const Strip& strip : strips)
                {
                    if (strip.owner == owner && strip.face == face) { length += strip_length(strip); }
                }
                require(approximately(length, piece.exposed[static_cast<size_t>(face)], 0.01f),
                    "Press Tower '" + piece.name + "' " + face_names[static_cast<size_t>(face)] + " exposes " +
                    fixed(length) + ", blueprint " + fixed(piece.exposed[static_cast<size_t>(face)]));
                total_length += length;
                orientation_lengths[static_cast<size_t>(std::min(face, 2))] += length;
                if (piece.tier != tier_none) { tiers[static_cast<size_t>(piece.tier)] += length; }
            }
        }
        require(approximately(total_length, 11700.0f, 0.1f),
            "Press Tower exposed length is " + fixed(total_length) + ", blueprint 11700");
        require(approximately(orientation_lengths[0], 9200.0f, 0.1f) && approximately(orientation_lengths[1], 1280.0f, 0.1f) &&
            approximately(orientation_lengths[2], 1220.0f, 0.1f), "Press Tower top/underside/wall lengths differ from the blueprint");
        for (size_t tier = 0; tier < tiers.size(); ++tier)
        {
            require(approximately(tiers[tier], tier_lengths[tier], 0.1f), std::string("Press Tower tier ") +
                tier_names[tier] + " exposes " + fixed(tiers[tier]) + ", blueprint " + fixed(tier_lengths[tier]));
        }
        const float unseen = tiers[static_cast<size_t>(tier_floor_1280)] + tiers[static_cast<size_t>(tier_bed_1200)] +
            tiers[static_cast<size_t>(tier_frames_1040)];
        require(approximately(unseen, 3600.0f, 0.1f), "Press Tower low route (floors, bed, frames) is not 3600 long");
        if (approximately(thickness, 12.0f, 0.0001f))
        {
            require(approximately(total_length * thickness, 140400.0f, 1.0f), "Press Tower area is not 140400 at strip 12");
        }
        else
        {
            std::cout << "NOTE Press Tower paint thickness is " << fixed(thickness, 3) <<
                ", not the blueprint's 12; areas below scale with it\n";
        }

        // 5. Default-floor rule: every ordinary flat's top paints fully,
        // except the head, whose crown buries 40 of its 320.
        for (const Surface& floor : s.map.surfaces)
        {
            if (floor.ramp() || floor.name == "press_base" ||
                floor.name == "boundary_wall_left" || floor.name == "boundary_wall_right") { continue; }
            float covered_width = 0.0f;
            for (const Strip& strip : strips)
            {
                if (&s.map.surfaces[strip.owner] == &floor && strip.face == 0) { covered_width += strip.box.width; }
            }
            const float expected = floor.name == "press_head" ? 280.0f : floor.rectangle.width;
            require(approximately(covered_width, expected),
                "Press Tower ordinary flat support has an unpaintable gap on '" + floor.name + "'");
        }

        // 6. Runtime reconciliation, object by object.
        float total = 0.0f;
        size_t scoring_objects = 0;
        for (size_t index = 0; index < objects.size(); ++index)
        {
            const auto* scoring = dynamic_cast<const PaintableObject*>(objects[index].get());
            const Surface& surface = s.map.surfaces[index];
            if (scoring == nullptr)
            {
                require(surface.type != "StructurePaintable", "Press Tower paintable '" + surface.name + "' does not score");
                continue;
            }
            ++scoring_objects;
            float expected = 0.0f;
            for (const Strip& strip : strips)
            {
                if (strip.owner == index) { expected += strip.box.width * strip.box.height; }
            }
            const PaintTotal actual = scoring->paint_total();
            require(approximately(actual.total, expected, std::max(0.1f, expected * 0.00002f)),
                "Press Tower runtime score differs from independently partitioned faces on '" + surface.name + "'");
            require(actual.team_a == 0.0f && actual.team_b == 0.0f, "Press Tower starts with team-owned paint");
            total += actual.total;
            std::cout << "MEASURE Press Tower scoring object=" << surface.name << " area=" << fixed(actual.total) << '\n';
        }
        require(scoring_objects == 25, "Press Tower needs 25 scoring objects; found " + std::to_string(scoring_objects));
        require(approximately(total, total_length * thickness, std::max(1.0f, total * 0.00002f)),
            "Press Tower runtime total does not equal blueprint length times strip thickness");

        // 7. Bands and tiers, with CSV receipts beside the executable.
        std::ofstream faces(output / "press_tower-faces.csv"), regions(output / "press_tower-paint.csv");
        faces << "object,face,x,y,width,height,area\n";
        for (const Strip& strip : strips)
        {
            const RectangleF& r = strip.box;
            faces << s.map.surfaces[strip.owner].name << ',' << face_names[static_cast<size_t>(strip.face)] << ',' <<
                r.x << ',' << r.y << ',' << r.width << ',' << r.height << ',' << r.width * r.height << '\n';
        }
        regions << "partition,region,top_area,bottom_area,vertical_area,total_area,share,blueprint_area\n";
        constexpr std::array<const char*, 7> band_names{ "home_a", "approach_a", "hoist_a", "crown", "hoist_b",
            "approach_b", "home_b" };
        const std::array<float, 8> cuts{ 0.0f, 520.0f, 1480.0f, 1640.0f, 1960.0f, 2120.0f, 3080.0f, 3600.0f };
        const std::array<float, 7> band_lengths{ 1560.0f, 3190.0f, 480.0f, 1240.0f, 480.0f, 3190.0f, 1560.0f };
        std::array<float, 7> bands{};
        float banded = 0.0f;
        for (size_t band = 0; band < band_names.size(); ++band)
        {
            std::array<float, 3> oriented{};
            const RectangleF box(cuts[band], bounds.top(), cuts[band + 1] - cuts[band], bounds.height);
            for (const Strip& strip : strips)
            {
                oriented[static_cast<size_t>(std::min(strip.face, 2))] += area_in(strip.box, box);
            }
            bands[band] = oriented[0] + oriented[1] + oriented[2];
            banded += bands[band];
            const float blueprint = band_lengths[band] * thickness;
            regions << "x_band," << band_names[band] << ',' << oriented[0] << ',' << oriented[1] << ',' << oriented[2] <<
                ',' << bands[band] << ',' << bands[band] / total << ',' << blueprint << '\n';
            std::cout << "MEASURE Press Tower band=" << band_names[band] << " area=" << fixed(bands[band]) <<
                " top=" << fixed(oriented[0]) << " underside=" << fixed(oriented[1]) << " vertical=" << fixed(oriented[2]) <<
                " share=" << fixed(100.0f * bands[band] / total) << "%\n";
            require(approximately(bands[band], blueprint, 0.5f), std::string("Press Tower band ") + band_names[band] +
                " holds " + fixed(bands[band]) + ", blueprint " + fixed(blueprint));
        }
        require(approximately(bands[0], bands[6]) && approximately(bands[1], bands[5]) && approximately(bands[2], bands[4]),
            "Press Tower mirrored bands hold different paint");
        require(approximately(banded, total, 1.0f), "Press Tower bands do not reconcile with runtime tiles");
        for (size_t tier = 0; tier < tiers.size(); ++tier)
        {
            regions << "tier," << tier_names[tier] << ",,,," << tiers[tier] * thickness << ',' <<
                tiers[tier] * thickness / total << ',' << tier_lengths[tier] * thickness << '\n';
            std::cout << "MEASURE Press Tower tier=" << tier_names[tier] << " area=" << fixed(tiers[tier] * thickness) <<
                " share=" << fixed(100.0f * tiers[tier] * thickness / total) << "%\n";
        }
        regions << "TOTAL,all," << orientation_lengths[0] * thickness << ',' << orientation_lengths[1] * thickness << ',' <<
            orientation_lengths[2] * thickness << ',' << total << ",1," << 11700.0f * thickness << '\n';
        require(faces.good() && regions.good(), "Cannot write Press Tower measured face/region receipts");
        std::cout << "MEASURE Press Tower total=" << fixed(total) << " top=" << fixed(orientation_lengths[0] * thickness) <<
            " underside=" << fixed(orientation_lengths[1] * thickness) << " vertical=" << fixed(orientation_lengths[2] * thickness) <<
            " unseen_low_route=" << fixed(unseen * thickness) << " (" << fixed(100.0f * unseen * thickness / total) << "%)\n";
        std::cout << "PASS Press Tower geometry: 30 blueprint pieces and 17 visuals, mirrored pairs and spawns, "
            "independent per-face partition reconciled with runtime tiles; band and tier CSVs written\n";

        // 8 and 9.
        standing_checks(s);
        sightline_checks(s);
    }
}
