#include "game/pch.h"
#include "game/objects/level_object_builder.h"
#include "game/objects/level_end_info.h"
#include "game/objects/projectile_spray.h"
#include "engine/assets/sprite_sheet_loader.h"

#include <filesystem>
#include <iostream>

using namespace labrador;
using namespace mattmath;

namespace
{
    void require(bool condition, const char* message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    }

    void area_is(float actual, float expected)
    {
        require(std::fabs(actual - expected) < 0.1f, "Unexpected paintable area");
    }
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "Usage: paint.exe <content directory>");
        const std::filesystem::path content(argv[1]);
        RenderResources render;
        render.add_sprite_sheet("sprite_sheet_1", read_sprite_sheet(
            (content / "textures/sprite_sheet_1.json").string().c_str(), TextureHandle{}));
        AudioResources audio;
        audio.add_sound_bank("paint_exe", SoundBank::silent());
        const GameTuning tuning((content / "tuning.json").string().c_str());
        const PlaySettings preferences;
        const TeamColour colours;
        const float thickness = tuning.paint().thickness;
        const RectangleF rectangle(0, 0, 100, 50);
        const PaintableFaces all{ true, true, true, true };
        const PaintableFaces top{ false, true, false, false };
        const auto make_surface = [&](const PaintableFaces& faces,
            std::span<const RectangleF> obstacles)
        {
            return std::make_unique<StructurePaintable>("sprite_sheet_1", "square_white_4",
                rectangle, std::make_unique<RectangleF>(rectangle), &render, &audio,
                &preferences, &tuning.paint(), CollisionObjectType::structure_paintable,
                colours, faces, obstacles);
        };
        const auto paint = [&](StructurePaintable& surface, const RectangleF& splash,
            PlayerTeam team, int owner)
        {
            // Spray construction converts the supplied launch position into
            // a collision origin by subtracting half its authored size.
            RectangleF launch = splash;
            const Vector2F half_size = tuning.projectile(ProjectileType::spray).details.col_rect_size / 2.0f;
            launch.offset(half_size.x, half_size.y);
            const ProjectileSpray shot(launch, Vector2F::ZERO, team, owner,
                Colour::white, &render, &tuning.projectile(ProjectileType::spray));
            surface.on_contact(shot, Vector2F::ZERO, 0);
        };

        // The object itself, interior geometry and a separated neighbor leave
        // the outer surfaces intact. All four outward normals are exercised.
        const RectangleF no_cover[]{ rectangle, {20, 10, 60, 30}, {20, -10, 60, 9} };
        area_is(make_surface(all, no_cover)->paint_total().total, 300 * thickness);
        const RectangleF four_sides[]{ rectangle, {20, -10, 60, 10}, {20, 50, 60, 10},
            {-10, 10, 10, 30}, {100, 10, 10, 30} };
        area_is(make_surface(all, four_sides)->paint_total().total, 120 * thickness);

        // Merge overlapping/duplicate occlusion, preserving fractional edges
        // rather than discarding an entire tile that is only partially covered.
        const RectangleF overlapping[]{ {20.5f, -10, 30.25f, 10}, {40, -10, 30, 10},
            {40, -10, 30, 10} };
        const std::unique_ptr<StructurePaintable> split = make_surface(top, overlapping);
        area_is(split->paint_total().total, 50.5f * thickness);
        paint(*split, {30, 0, 4, thickness}, PlayerTeam::a, 0);
        area_is(split->paint_total().team_a, 0);
        paint(*split, {0, 0, 100, thickness}, PlayerTeam::a, 0);
        area_is(split->paint_total().team_a, split->paint_total().total);
        paint(*split, {0, 0, 100, thickness}, PlayerTeam::a, 1);
        area_is(split->paint_total().player_areas[0], split->paint_total().total);
        paint(*split, {0, 0, 100, thickness}, PlayerTeam::b, 1);
        area_is(split->paint_total().team_a, 0);
        area_is(split->paint_total().player_areas[1], split->paint_total().total);

        // Sparse devices and multiple device-less seats must never share credit.
        const int first_dummy = paint_owners::index(-1, 1);
        const int second_dummy = paint_owners::index(-1, 3);
        const int keyboard = paint_owners::index(input_slots::keyboard_and_mouse, 0);
        require(first_dummy != second_dummy && first_dummy != keyboard &&
            paint_owners::index(3, 1) == 3 && paint_owners::index(-1, 4) == -1,
            "Paint owners alias devices or other dummy seats");
        paint(*split, {0, 0, 100, thickness}, PlayerTeam::a, first_dummy);
        area_is(split->paint_total().player_areas[1], 0);
        area_is(split->paint_total().player_areas[first_dummy], split->paint_total().total);
        paint(*split, {0, 0, 100, thickness}, PlayerTeam::a, second_dummy);
        area_is(split->paint_total().player_areas[first_dummy], split->paint_total().total);
        area_is(split->paint_total().player_areas[second_dummy], 0);
        paint(*split, {0, 0, 100, thickness}, PlayerTeam::b, second_dummy);
        area_is(split->paint_total().player_areas[first_dummy], 0);
        area_is(split->paint_total().player_areas[second_dummy], split->paint_total().total);
        LevelEndInfo dummy_result;
        dummy_result.total_area = split->paint_total().total;
        dummy_result.player_areas = split->paint_total().player_areas;
        require(dummy_result.player_percentage_string(second_dummy) == L"100.0%" &&
            dummy_result.player_percentage_string(keyboard) == L"0.0%",
            "Results omit dummy credit or give it to the keyboard player");
        LevelEndInfo result;
        result.total_area = split->paint_total().total;
        result.team_b_area = split->paint_total().team_b;
        result.team_b_played = true;
        require(std::fabs(result.coverage_ratio() - 1.0f) < 0.000001f,
            "Exposed surface cannot reach full solo coverage");

        const RectangleF enclosing[]{ {-1, -1, 102, 52} };
        const std::unique_ptr<StructurePaintable> hidden = make_surface(all, enclosing);
        paint(*hidden, rectangle, PlayerTeam::a, 0);
        area_is(hidden->paint_total().total, 0);
        area_is(hidden->paint_total().team_a, 0);

        // Use the real authored stage and builder. The lower ledge has two
        // exposed 200-unit top segments and two 50-unit sides, independent of
        // the 1,800-unit band buried beneath the next mountain block.
        const LevelObjectBuilder builder(&render, &audio, &preferences, &tuning.paint());
        for (const char* name : { "king_of_the_hill", "turbulence", "close_quarters", "movement_course" })
        {
            const JsonDocument level = read_json_file(
                (content / "levels" / (std::string(name) + ".json")).string().c_str());
            const auto objects = builder.build_collision_objects(
                level.root().array("collision_objects"), colours);
            float total = 0;
            float perimeter = 0;
            bool checked_ledge = false;
            bool checked_ramps = false;
            for (const auto& object : objects)
            {
                StructurePaintable* surface = dynamic_cast<StructurePaintable*>(object.get());
                if (surface == nullptr)
                {
                    continue;
                }
                total += surface->paint_total().total;
                perimeter += 2.0f * (surface->bounds().width + surface->bounds().height) * thickness;
                if (std::string(name) == "king_of_the_hill" &&
                    surface->bounds().x == 0 && surface->bounds().y == 6000)
                {
                    // Ramps pass paint; only the 2,600-unit solid mountain
                    // centre removes surface from the 6,000-unit floor.
                    area_is(surface->paint_total().total, 3400 * thickness);
                    checked_ramps = true;
                }
                if (std::string(name) == "king_of_the_hill" &&
                    surface->bounds().x == 1900 && surface->bounds().y == 5500)
                {
                    area_is(surface->paint_total().total, 500 * thickness);
                    paint(*surface, {3000, 5500, 4, thickness}, PlayerTeam::a, 0);
                    area_is(surface->paint_total().team_a, 0);
                    paint(*surface, {1900, 5500, 200, thickness}, PlayerTeam::a, 0);
                    require(surface->paint_total().team_a > 0, "Exposed ledge cannot be painted");
                    checked_ledge = true;
                }
            }
            require(total > 0, "Stage lost every paintable surface");
            if (std::string(name) == "king_of_the_hill")
            {
                require(checked_ledge, "Authored mountain regression fixture disappeared");
                require(checked_ramps, "Authored ramp regression fixture disappeared");
                require(total < 318720, "Buried surface still contributes to stage coverage");
            }
            if (std::string(name) == "movement_course")
            {
                // Recorded from the authored course; an intended course edit
                // updates it. Its blocks touch, so the full perimeter is more.
                area_is(total, 194880);
                require(total < perimeter, "Buried course surface still contributes to coverage");
            }
            std::cout << name << " exposed paint area=" << total << " of perimeter " << perimeter << '\n';
        }
        std::cout << "PASS exposed surfaces, partial tiles, ownership and stage coverage\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
