#include "game/pch.h"
#include "scripts/acceptance/turbulence_slopes/fixtures.h"
#include <array>
#include <iomanip>
#include <iostream>

namespace turbulence_slopes_acceptance
{
    namespace
    {
        using namespace mattmath;
        constexpr std::array<const char*, 4> face_names{ "top", "bottom", "left", "right" };
        struct FaceBudget
        {
            std::string name;
            RectangleF body;
            std::array<double, 4> area{};
        };

        rapidjson::Document json(const fs::path& path)
        {
            const auto bytes = read_text(path);
            rapidjson::Document result; result.Parse(bytes.c_str());
            require(!result.HasParseError() && result.IsObject(), "Cannot parse shoulder geometry receipt input");
            return result;
        }

        RectangleF rectangle(const rapidjson::Value& object)
        {
            const auto& value = object["rectangle"];
            return { value["x"].GetFloat(), value["y"].GetFloat(), value["width"].GetFloat(), value["height"].GetFloat() };
        }

        // Independent integer-world edge census. Every segment is tested at
        // its midpoint just outside the face against actual flat occupancy;
        // this does not call the runtime/editor interval-clipping helper.
        std::vector<FaceBudget> census(const rapidjson::Document& document, float thickness)
        {
            std::vector<RectangleF> obstacles;
            for (const auto& object : document["collision_objects"].GetArray())
            {
                const std::string type = object["type"].GetString();
                if ((type == "Structure" || type == "StructurePaintable") && !object.HasMember("motion"))
                { obstacles.push_back(rectangle(object)); }
            }
            std::vector<FaceBudget> result;
            for (const auto& object : document["collision_objects"].GetArray())
            {
                if (std::string(object["type"].GetString()) != "StructurePaintable") { continue; }
                FaceBudget surface{ object["name"].GetString(), rectangle(object), {} };
                const bool moving = object.HasMember("motion");
                for (std::size_t face = 0; face < face_names.size(); ++face)
                {
                    if (!object["paintable_faces"][face_names[face]].GetBool()) { continue; }
                    const bool along_x = face < 2, leading = face % 2 == 0;
                    const float length = along_x ? surface.body.width : surface.body.height;
                    const int units = static_cast<int>(length);
                    require(length == static_cast<float>(units), "Independent shoulder edge census requires integral authored lengths");
                    const float edge = along_x ? (leading ? surface.body.top() : surface.body.bottom()) :
                        (leading ? surface.body.left() : surface.body.right());
                    const float outside = edge + (leading ? -0.25f : 0.25f);
                    for (int unit = 0; unit < units; ++unit)
                    {
                        const float along = (along_x ? surface.body.x : surface.body.y) + static_cast<float>(unit) + 0.5f;
                        const Vector2F sample = along_x ? Vector2F(along, outside) : Vector2F(outside, along);
                        bool covered = false;
                        if (!moving)
                        {
                            for (const auto& obstacle : obstacles)
                            {
                                if (sample.x > obstacle.left() && sample.x < obstacle.right() &&
                                    sample.y > obstacle.top() && sample.y < obstacle.bottom()) { covered = true; break; }
                            }
                        }
                        if (!covered) { surface.area[face] += thickness; }
                    }
                }
                result.push_back(std::move(surface));
            }
            return result;
        }

        double total(const std::vector<FaceBudget>& budget)
        {
            double result = 0.0;
            for (const auto& source : budget) { for (double area : source.area) { result += area; } }
            return result;
        }

        void actual_budget(const Trial& trial, const std::vector<FaceBudget>& expected)
        {
            std::vector<std::array<double, 4>> actual(expected.size());
            for (const auto& tile : trial.level->paint_surfaces())
            {
                require(tile.source_index < expected.size() && std::isfinite(tile.scored_area) && tile.scored_area > 0.0f &&
                    tile.team == PlayerTeam::none && tile.paint_owner == -1, "Fresh shoulder tiles lost source/area/neutral ownership");
                const auto face = static_cast<std::size_t>(tile.face);
                require(face < face_names.size(), "Shoulder tile has an unknown face");
                actual[tile.source_index][face] += tile.scored_area;
                const auto& source = expected[tile.source_index];
                if (source.name.starts_with("shoulder_shoe"))
                {
                    for (const auto& corner : tile.corners)
                    {
                        require(corner.x >= source.body.left() && corner.x <= source.body.right() &&
                            corner.y >= source.body.top() && corner.y <= source.body.bottom(),
                            "Shoe tile left its authored flat scoring body");
                    }
                    if (tile.face == PaintSurfaceFace::top)
                    { require(tile.corners[0].y == 880.0f, "Reachable shoe top was moved to the unscored ramp surface"); }
                    if (tile.face == PaintSurfaceFace::bottom)
                    { require(tile.corners[2].y == 904.0f, "Shoe underside is not the actual flat scored bottom"); }
                }
            }
            for (std::size_t source = 0; source < expected.size(); ++source)
            {
                for (std::size_t face = 0; face < face_names.size(); ++face)
                { require(actual[source][face] == expected[source].area[face], "Runtime shoulder source/face differs from independent census: " + expected[source].name + "/" + face_names[face]); }
            }
            require(trial.level->level_end_info().total_area == static_cast<float>(total(expected)),
                "Public shoulder Level total differs from its independent edge census");
        }
    }

    void geometry(Context& context)
    {
        auto baseline = json(context.content / "levels/turbulence_mover_trial.json");
        auto candidate = json(context.stage);
        require(baseline["collision_objects"].Size() == 22 && candidate["collision_objects"].Size() == 26 &&
            !candidate["include_in_random"].GetBool(), "Shoulder trial must remain an isolated four-object composite extension");
        for (rapidjson::SizeType index = 0; index < 22; ++index)
        { require(baseline["collision_objects"][index] == candidate["collision_objects"][index], "Shoulder trial changed existing geometry or motion"); }
        const float thickness = context.world.tuning.paint().thickness;
        const auto baseline_budget = census(baseline, thickness), candidate_budget = census(candidate, thickness);
        require(total(baseline_budget) == 165120.0 && total(candidate_budget) == 172800.0 &&
            total(candidate_budget) - total(baseline_budget) == 7680.0, "Independent shoulder paint allocation differs from the four-object contract");
        for (const auto& source : candidate_budget)
        {
            if (!source.name.starts_with("shoulder_shoe")) { continue; }
            require(source.area[0] == 1920.0 && source.area[1] == 1920.0 && source.area[2] + source.area[3] == 288.0,
                "Shoe must retain reachable top/underside and one exposed24-unit end; its abutting end is buried");
        }
        colourwars_editor::LevelDocument document(context.content / "tuning.json"); document.load(context.stage);
        colourwars_editor::BlockoutTuning tuning;
        tuning.player_width = context.world.tuning.player().size.x; tuning.player_height = context.world.tuning.player().size.y;
        tuning.paint_thickness = thickness;
        const auto editor_faces = colourwars_editor::scoring_faces(document, tuning);
        require(colourwars_editor::scoring_area(editor_faces) == total(candidate_budget),
            "Actual editor scoring differs from the independent shoulder census");
        std::ofstream csv(context.evidence / "shoulder-budget.csv");
        require(csv.good(), "Cannot create shoulder budget receipt");
        csv << std::setprecision(std::numeric_limits<double>::max_digits10) << "players,source_ordinal,source,top,bottom,left,right,total,count_in\n";
        for (const int count : { 1, 4 })
        {
            Trial trial(context, context.stage, settings(count));
            actual_budget(trial, candidate_budget);
            const int count_in = trial.count_in(); require(count_in == 181, "Shoulder public load changed the full source count-in");
            for (int update = 0; update < 12; ++update) { trial.tick(); }
            actual_budget(trial, candidate_budget);
            require(trial.level->player_count() == count && trial.level->horizontal_structure_states().size() == 2,
                "Shoulder public load lost requested roster or the two original movers");
            for (std::size_t index = 0; index < candidate_budget.size(); ++index)
            {
                const auto& source = candidate_budget[index];
                csv << count << ',' << index << ',' << source.name;
                double sum = 0.0;
                for (const double area : source.area) { csv << ',' << area; sum += area; }
                csv << ',' << sum << ',' << count_in << '\n';
            }
        }
        csv.close(); require(csv.good(), "Cannot finish shoulder budget receipt");
        std::cout << "PASS shoulder composite public1/4 loads+181countin: independent unit-edge/editor/runtime source-face budgets "
            "agree at172800 (+7680); ramps remain unscored/paint-pass, flat shoes retain exposed top+underside/end credit\n";
    }
}
