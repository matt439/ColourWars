#include "game/pch.h"
#include "scripts/acceptance/turbulence_blockout/fixtures.h"

namespace turbulence_blockout_acceptance
{
    namespace
    {
        struct Expected
        {
            std::string name;
            RectangleF rectangle;
            // Independently counted exposed lengths: top, bottom, left, right.
            std::array<float, 4> lengths{};
        };

        std::vector<Expected> blueprint()
        {
            std::vector<Expected> result{
                { "ground", { 0, 1280, 4000, 400 }, { 3360, 0, 0, 0 } },
                { "boundary_wall_left", { -80, 0, 80, 1680 }, {} },
                { "boundary_wall_right", { 4000, 0, 80, 1680 }, {} },
                { "centre_baffle", { 1940, 720, 120, 160 }, { 120, 120, 80, 80 } }
            };
            const std::array<Expected, 8> side{ {
                { "home_floor", { 0, 1120, 600, 10 }, { 600, 0, 0, 0 } },
                { "home_roof", { 0, 960, 480, 10 }, { 480, 0, 0, 0 } },
                { "side_step", { 600, 1040, 280, 80 }, { 280, 280, 80, 80 } },
                { "upper_approach", { 880, 880, 360, 80 }, { 360, 360, 80, 80 } },
                { "lower_bed", { 1260, 1200, 320, 80 }, { 320, 0, 80, 80 } },
                { "bed_ascent", { 1100, 1200, 160, 80 }, {} },
                { "bed_descent", { 1580, 1200, 160, 80 }, {} },
                { "crossover", { 1400, 800, 540, 80 }, { 540, 540, 80, 0 } }
            } };
            for (const Expected& source : side)
            {
                Expected left = source; left.name += "_a"; result.push_back(left);
                Expected right = source; right.name += "_b";
                right.rectangle.x = 4000.0f - source.rectangle.right();
                std::swap(right.lengths[2], right.lengths[3]); result.push_back(right);
            }
            return result;
        }

        std::size_t region(const Surface& surface, const PaintSurfaceSample& tile)
        {
            // Partition the whole walking floor at home/approach/contest boundaries;
            // give other objects one stable role, including all their scored faces.
            if (surface.name == "ground")
            {
                const float x = (tile.corners[0].x + tile.corners[2].x) / 2.0f;
                return x < 600 ? 0 : x < 1100 ? 2 : x < 2900 ? 4 : x < 3400 ? 3 : 1;
            }
            if (surface.name.starts_with("home_")) { return surface.name.ends_with("_a") ? 0 : 1; }
            if (surface.name.starts_with("lower_bed")) { return 4; }
            if (surface.name.starts_with("crossover") || surface.name == "centre_baffle") { return 5; }
            return surface.name.ends_with("_a") ? 2 : 3;
        }
    }

    void geometry(Services& services, const std::filesystem::path& output)
    {
        require(!services.info.include_in_random(), "Turbulence blockout must remain explicit-only");
        require(services.tuning.player().size == Vector2F(52, 120), "Requalify spawn reflection for changed player dimensions");
        const auto expected = blueprint();
        require(services.surfaces.size() == expected.size(), "Turbulence collision inventory differs from the static brief");
        auto built = services.builder.build_collision_objects(services.info.collision_objects_json(), services.colours);
        std::array<double, 4> faces{};
        std::array<double, 6> regions{};
        double total = 0.0;
        std::ofstream face_csv(output / "turbulence-blockout-faces.csv"), region_csv(output / "turbulence-blockout-regions.csv");
        require(face_csv.good() && region_csv.good(), "Cannot open task-owned paint receipts");
        face_csv << "object,top,bottom,left,right,total\n";
        for (std::size_t index = 0; index < services.surfaces.size(); ++index)
        {
            const Surface& surface = services.surfaces[index];
            const auto found = std::find_if(expected.begin(), expected.end(),
                [&](const Expected& piece) { return piece.name == surface.name; });
            require(found != expected.end() && found->rectangle == surface.rectangle && built[index]->bounds() == surface.rectangle,
                "Authored/runtime geometry differs from blueprint on " + surface.name);
            const auto* scoring = dynamic_cast<const PaintableObject*>(built[index].get());
            std::array<double, 4> actual{};
            if (scoring != nullptr)
            {
                std::vector<PaintSurfaceSample> tiles;
                scoring->append_paint_surfaces(tiles, index);
                for (const PaintSurfaceSample& tile : tiles)
                {
                    const auto face = static_cast<std::size_t>(tile.face);
                    require(face < actual.size() && tile.team == PlayerTeam::none && tile.scored_area > 0.0f,
                        "New static stage contains invalid or already-owned scoring tiles");
                    actual[face] += tile.scored_area;
                    regions[region(surface, tile)] += tile.scored_area;
                }
                const auto owned = scoring->paint_total();
                const double sampled = actual[0] + actual[1] + actual[2] + actual[3];
                require(std::fabs(owned.total - sampled) < 0.01 && owned.team_a == 0.0f && owned.team_b == 0.0f,
                    "Runtime score and sampled surfaces disagree on " + surface.name);
            }
            double area = 0.0;
            face_csv << surface.name;
            for (std::size_t face = 0; face < faces.size(); ++face)
            {
                const double independently_counted = found->lengths[face] * services.tuning.paint().thickness;
                require(std::fabs(actual[face] - independently_counted) < 0.01,
                    "Independent exposed face budget differs on " + surface.name + " face " + std::to_string(face));
                faces[face] += actual[face]; area += actual[face]; face_csv << ',' << actual[face];
            }
            total += area; face_csv << ',' << area << '\n';
        }
        require(std::fabs(total - 12400.0 * services.tuning.paint().thickness) < 0.01,
            "Turbulence total no longer matches independently counted 12400 exposed units");
        constexpr std::array<const char*, 6> names{ "home_a", "home_b", "approach_a", "approach_b", "lower_contest", "upper_crossing" };
        constexpr std::array<double, 6> lengths{ 1680, 1680, 2100, 2100, 2120, 2720 };
        region_csv << "region,area,share\n";
        double regional_total = 0.0;
        for (std::size_t index = 0; index < regions.size(); ++index)
        {
            require(std::fabs(regions[index] - lengths[index] * services.tuning.paint().thickness) < 0.01,
                "Regional paint partition differs on " + std::string(names[index]));
            regional_total += regions[index]; region_csv << names[index] << ',' << regions[index] << ',' << regions[index] / total << '\n';
        }
        require(regional_total == total, "Regional partition lost or duplicated scored area");
        require(regions[0] == regions[1] && regions[2] == regions[3], "Mirrored score roles differ between teams");
        face_csv.close(); region_csv.close();
        require(face_csv.good() && region_csv.good(), "Could not flush paint receipts");
        std::cout << "MEASURE Turbulence paint total=" << total << " top=" << faces[0] << " bottom=" << faces[1] <<
            " vertical=" << faces[2] + faces[3] << "; role areas=";
        for (const double area : regions) { std::cout << area << ' '; }
        std::cout << "\nPASS mirrored static geometry and independent exposed-face/runtime-tile/region reconciliation\n";
    }
}
