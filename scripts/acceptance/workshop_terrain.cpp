#include "game/pch.h"
#include "engine/collision/collision_object.h"
#include "engine/render/texture_object.h"
#include "game/objects/collision_object_type.h"

// Inspect the immutable draw records actually consumed by Structure::draw.
// Load its dependencies first so this does not change engine declarations.
#define private public
#include "game/objects/structure.h"
#undef private

#include "game/objects/level_object_builder.h"
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
        if (!condition) { throw std::runtime_error(message); }
    }

    bool approximately_equal(float left, float right)
    {
        return std::fabs(left - right) < 0.001f;
    }

    void check_tiles(const Structure& object, const RectangleI& frame, float density,
        bool check_overlap = false)
    {
        const RectangleF bounds = object.bounds();
        require(!object.texture_tiles_.empty(), "Workshop terrain was not tiled");
        double drawn_area = 0.0;
        float right = bounds.left();
        float bottom = bounds.top();
        for (size_t index = 0; index < object.texture_tiles_.size(); ++index)
        {
            const auto& tile = object.texture_tiles_[index];
            const RectangleF& destination = tile.destination;
            require(frame.contains(tile.source), "A cropped tile samples outside its frame");
            require(destination.width > 0 && destination.height > 0,
                "A terrain tile has no area");
            require(destination.left() >= bounds.left() && destination.top() >= bounds.top() &&
                destination.right() <= bounds.right() && destination.bottom() <= bounds.bottom(),
                "Terrain art extends outside collision geometry");
            require((tile.source.width == 1 && destination.width <= density) ||
                approximately_equal(destination.width, static_cast<float>(tile.source.width) * density),
                "Horizontal multi-texel detail was stretched");
            require((tile.source.height == 1 && destination.height <= density) ||
                approximately_equal(destination.height, static_cast<float>(tile.source.height) * density),
                "Vertical multi-texel detail was stretched");
            drawn_area += static_cast<double>(destination.width) * destination.height;
            right = std::max(right, destination.right());
            bottom = std::max(bottom, destination.bottom());
            if (check_overlap)
            {
                for (size_t other = index + 1; other < object.texture_tiles_.size(); ++other)
                {
                    const RectangleF& other_tile = object.texture_tiles_[other].destination;
                    const float width = std::min(destination.right(), other_tile.right()) -
                        std::max(destination.left(), other_tile.left());
                    const float height = std::min(destination.bottom(), other_tile.bottom()) -
                        std::max(destination.top(), other_tile.top());
                    require(width <= 0 || height <= 0,
                        "Terrain tiles overlap");
                }
            }
        }
        require(std::fabs(drawn_area - static_cast<double>(bounds.width) * bounds.height) < 0.001,
            "Terrain art leaves an area gap or adds area");
        require(approximately_equal(right, bounds.right()) && approximately_equal(bottom, bounds.bottom()),
            "Cropped tiles did not reach the exact authored edge");
    }

    void same_paint(const StructurePaintable& original, const StructurePaintable& workshop)
    {
        const PaintTotal before = original.paint_total();
        const PaintTotal after = workshop.paint_total();
        require(approximately_equal(before.total, after.total) && approximately_equal(before.team_a, after.team_a) &&
            approximately_equal(before.team_b, after.team_b) && before.player_areas == after.player_areas,
            "Workshop terrain changed paint coverage or per-player ownership");
    }
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "Usage: workshop_terrain.exe <content directory>");
        const std::filesystem::path content(argv[1]);
        RenderResources render;
        for (const char* name : { "sprite_sheet_1", "workshop_kit" })
        {
            render.add_sprite_sheet(name, read_sprite_sheet(
                (content / "textures" / (std::string(name) + ".json")).string().c_str(),
                TextureHandle{}));
        }
        const SpriteSheet* kit = render.sprite_sheet("workshop_kit");
        const RectangleI frame = kit->sprite_frame(
            kit->resolve_sprite_frame("platform")).source_rectangle();
        const float density = 4.0f;
        require(frame.width == 32 && frame.height == 12, "Platform source grid changed");

        // Exact modules, partial modules, fractional final texels and geometry
        // smaller than one texel all preserve their authored extent.
        for (const RectangleF& rectangle : {
            RectangleF(0, 0, 256, 96), RectangleF(-3.5f, 2.25f, 261.5f, 101.25f),
            RectangleF(7.25f, -9.5f, 0.75f, 2.5f), RectangleF(0, 0, 120, 40) })
        {
            const Structure structure("workshop_kit", "platform", rectangle,
                std::make_unique<RectangleF>(rectangle), &render,
                CollisionObjectType::structure, Colour::white, 0.0f,
                Vector2F::ZERO, SpriteFlip::none, 0.0f, density);
            check_tiles(structure, frame, density, true);
            require(structure.shape()->bounding_box() == rectangle,
                "Terrain tiling changed collision bounds");
        }

        const auto reject_tile_budget = [&](const RectangleF& rectangle, float tile_density)
        {
            bool rejected = false;
            try
            {
                const Structure structure("workshop_kit", "platform", rectangle,
                    std::make_unique<RectangleF>(rectangle), &render,
                    CollisionObjectType::structure, Colour::white, 0.0f,
                    Vector2F::ZERO, SpriteFlip::none, 0.0f, tile_density);
            }
            catch (const std::invalid_argument& error)
            {
                rejected = std::string(error.what()).find("65536 draw-tile limit") != std::string::npos;
            }
            require(rejected, "Unbounded tiled terrain was not rejected by its content-work budget");
        };
        reject_tile_budget(RectangleF(0, 0, 128, 48), 0.000001f);
        reject_tile_budget(RectangleF(0, 0, 1.0e20f, 1.0e20f), density);
        // 256x256 whole modules exactly fill the budget. Partial final texels
        // increase the actual cross-product beyond that preflight estimate.
        const RectangleF at_limit(0, 0, 128 * 256, 48 * 256);
        const Structure bounded("workshop_kit", "platform", at_limit,
            std::make_unique<RectangleF>(at_limit), &render,
            CollisionObjectType::structure, Colour::white, 0.0f,
            Vector2F::ZERO, SpriteFlip::none, 0.0f, density);
        require(bounded.texture_tiles_.size() == 65536, "Exact tile-budget boundary was rejected");
        reject_tile_budget(RectangleF(0, 0, 128 * 255 + 4.5f, 48 * 255 + 4.5f), density);

        AudioResources audio;
        audio.add_sound_bank("paint_exe", SoundBank::silent());
        const GameTuning tuning((content / "tuning.json").string().c_str());
        const PlaySettings preferences;
        const TeamColour colours;
        const LevelObjectBuilder builder(&render, &audio, &preferences, &tuning.paint());
        size_t paintables = 0;
        size_t untouched = 0;
        // Movement Course is the runtime comparison. Other fixtures additionally
        // prove that the builder preserves ramps and buried-face exclusions.
        for (const char* name : { "movement_course", "king_of_the_hill", "turbulence", "close_quarters" })
        {
            const JsonDocument stage = read_json_file(
                (content / "levels" / (std::string(name) + ".json")).string().c_str());
            const JsonValue objects = stage.root().array("collision_objects");
            const auto original = builder.build_collision_objects(objects, colours);
            const auto workshop = builder.build_collision_objects(objects, colours, density);
            require(original.size() == workshop.size(), "Workshop terrain changed object count");
            for (size_t index = 0; index < original.size(); ++index)
            {
                const CollisionObject& before = *original[index];
                const CollisionObject& after = *workshop[index];
                require(before.bounds() == after.bounds() &&
                    before.shape()->bounding_box() == after.shape()->bounding_box() &&
                    before.shape()->shape_type() == after.shape()->shape_type() &&
                    before.layer() == after.layer() && before.mask() == after.mask() &&
                    before.tag() == after.tag(), "Workshop comparison changed collision behaviour");
                const Structure* original_structure = dynamic_cast<const Structure*>(&before);
                const Structure* workshop_structure = dynamic_cast<const Structure*>(&after);
                require(original_structure && workshop_structure, "Unexpected terrain object");
                require(original_structure->texture_tiles_.empty(), "Original mode gained tiling");
                StructurePaintable* original_paint = dynamic_cast<StructurePaintable*>(original[index].get());
                StructurePaintable* workshop_paint = dynamic_cast<StructurePaintable*>(workshop[index].get());
                require((original_paint == nullptr) == (workshop_paint == nullptr),
                    "Workshop comparison changed a paintable object");
                if (original_paint == nullptr)
                {
                    require(workshop_structure->texture_tiles_.empty(),
                        "Ramp or jump-through diagnostic art was replaced");
                    ++untouched;
                    continue;
                }
                ++paintables;
                check_tiles(*workshop_structure, frame, density);
                same_paint(*original_paint, *workshop_paint);
                for (PlayerTeam team : { PlayerTeam::a, PlayerTeam::b })
                {
                    RectangleF splash = before.bounds();
                    const Vector2F half = tuning.projectile(ProjectileType::spray).details.col_rect_size / 2.0f;
                    splash.offset(half.x, half.y);
                    const ProjectileSpray shot(splash, Vector2F::ZERO, team,
                        team == PlayerTeam::a ? 0 : 1, Colour::white, &render,
                        &tuning.projectile(ProjectileType::spray));
                    original_paint->on_contact(shot, Vector2F::ZERO, 0);
                    workshop_paint->on_contact(shot, Vector2F::ZERO, 0);
                    same_paint(*original_paint, *workshop_paint);
                    const PaintTotal total = workshop_paint->paint_total();
                    require(approximately_equal(team == PlayerTeam::a ? total.team_a : total.team_b, total.total),
                        "A workshop scoring face cannot reach full coverage");
                }
            }
        }
        require(paintables > 0 && untouched > 0, "Terrain fixtures missed the comparison boundary");
        std::cout << "Workshop terrain: cropped native tiles, collision parity and paint ownership passed ("
            << paintables << " paintables, " << untouched << " unchanged structures).\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
