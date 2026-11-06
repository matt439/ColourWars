#pragma once
#include "game/debug/dev_tools.h"
#include "game/contracts/simulation.h"
#include "game/objects/player.h"
#include "game/objects/structure.h"
#include "game/objects/structure_paintable.h"
#include "game/objects/projectile_jet.h"
#include "game/objects/weapon_builder.h"
#include "game/config/presentation/presentation.h"
#include "engine/assets/json.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/core/file_path.h"
#include "engine/scene/scene.h"
#include "scripts/acceptance/support/player_course.h"
#include "scripts/acceptance/support/probe.h"
#include <bit>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace paint_underfoot_acceptance
{
    using acceptance::require;
    inline std::uint32_t bits(float value) { return std::bit_cast<std::uint32_t>(value); }
    inline const float dt = SECONDS_PER_STEP;
    inline PlayerInputData input(float movement = 0.0f, bool jump = false, bool fire = false, bool drop = false)
    {
        PlayerInputData value;
        value.connected = true; value.x_movement = movement;
        value.jump_pressed = jump; value.jump_held = jump; value.primary_shoot = fire; value.drop_through = drop;
        value.shoot_direction_requested = true; value.shoot_direction = { 1.0f, 0.0f };
        return value;
    }

    class World final
    {
    public:
        explicit World(const std::filesystem::path& content) :
            tuning(labrador::path_to_utf8(content / "tuning.json").c_str()),
            presentation(labrador::path_to_utf8(content / "presentation.json").c_str())
        {
            std::ifstream stream(content / "tuning.json", std::ios::binary);
            require(stream.good(), "Cannot read source tuning for isolated underfoot variants");
            source.assign(std::istreambuf_iterator<char>(stream), {});
            for (const char* name : { "sprite_sheet_1", "painter", "weapon_paint", "paint_fx", "terrain" })
            {
                render.add_sprite_sheet(name, labrador::read_sprite_sheet(labrador::path_to_utf8(
                    content / "textures" / labrador::path_from_utf8(std::string(name) + ".json")).c_str(),
                    labrador::TextureHandle{}));
            }
            audio.add_sound_bank("paint_exe", labrador::SoundBank::silent());
        }
        template<class Edit> GameTuning variant(Edit edit) const
        {
            rapidjson::Document document;
            document.Parse(source.c_str());
            require(!document.HasParseError(), "Cannot parse isolated underfoot tuning");
            edit(document);
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            require(document.Accept(writer), "Cannot encode isolated underfoot tuning");
            return GameTuning(labrador::parse_json(buffer.GetString(), "paint-underfoot-variant.json"));
        }
        void configure(bool enabled, float speed = 1.10f, float recovery = 1.25f,
            float enemy_speed = 0.85f, float enemy_recovery = 0.0f, bool block_regen = false)
        {
            tuning = variant([&](rapidjson::Document& document)
            {
                auto& block = document["player"]["paint_underfoot"];
                block["enabled"].SetBool(enabled);
                block["own_speed_multiplier"].SetFloat(speed);
                block["own_ammo_regen_multiplier"].SetFloat(recovery);
                block["enemy_speed_multiplier"].SetFloat(enemy_speed);
                block["enemy_ammo_regen_multiplier"].SetFloat(enemy_recovery);
                block["enemy_blocks_health_regen"].SetBool(block_regen);
            });
        }
        std::unique_ptr<Weapon> weapon(WeaponType tool, PlayerTeam team = PlayerTeam::a, int owner = 5)
        {
            return WeaponBuilder::build_weapon(&tuning, team, owner, 0, labrador::Colour::white,
                tool, labrador::Vector2F::zero, &render, &audio, &preferences);
        }
        void paint(StructurePaintable& surface, const labrador::RectangleF& splash,
            PlayerTeam team, int owner = -1)
        {
            const auto& projectile = tuning.projectile(ProjectileType::jet);
            auto launch = splash;
            const auto half = projectile.details.col_rect_size / 2.0f;
            launch.offset(half.x, half.y);
            ProjectileJet shot(launch, labrador::Vector2F::zero, team, owner,
                labrador::Colour::white, &render, resolve_projectile_strip(projectile, render), &projectile);
            surface.on_contact(shot, labrador::Vector2F::zero, 0.0f);
        }
        labrador::RenderResources render;
        labrador::AudioResources audio;
        PlaySettings preferences;
        GameTuning tuning;
        Presentation presentation;
        std::string source;
    };

    // The actual Scene/Player/Weapon contact workflow, without Application/devices.
    // New shots are counted then retired by this fixture, so they cannot add unrelated damage.
    class Course final : public acceptance::PlayerCourse
    {
    public:
        Course(World& resources, PlayerTeam team = PlayerTeam::a, WeaponType tool = WeaponType::sprayer,
            float center_x = 0.0f, float feet_y = 0.0f, int device_slot = -1) : world(resources)
        {
            const labrador::RectangleF body(center_x - world.tuning.player().size.x / 2.0f,
                feet_y - world.tuning.player().size.y, world.tuning.player().size.x, world.tuning.player().size.y);
            auto primary = world.weapon(tool, team, paint_owners::index(device_slot, 0));
            player = scene.add(std::make_unique<Player>(body, &world.tuning,
                &world.presentation.movement_feedback(), &world.presentation.combat_feedback(),
                &world.presentation.respawn_redraw(), &world.render, &world.audio, &world.preferences,
                device_slot, team, tool, std::move(primary), WeaponType::none,
                labrador::Colour::white, labrador::Viewport{}, body.top_left(), labrador::Vector2F::zero,
                0.0f, labrador::Vector2F::zero, labrador::SpriteFlip::none, 0.0f, 0));
            scene.end_tick();
        }
        StructurePaintable& surface(const labrador::RectangleF& bounds = { -10000, 0, 20000, 100 },
            const PaintableFaces& faces = { false, true, false, false },
            std::span<const labrador::RectangleF> obstacles = {},
            CollisionObjectType type = CollisionObjectType::structure_paintable)
        {
            auto* result = scene.add(std::make_unique<StructurePaintable>("sprite_sheet_1", "square_white_4",
                bounds, std::make_unique<labrador::RectangleF>(bounds), &world.render,
                &world.preferences, &world.tuning.paint(), type,
                TeamColour{ labrador::Colour::white, labrador::Colour::white }, faces, obstacles));
            scene.end_tick(); return *result;
        }
        void plain(const labrador::RectangleF& bounds)
        {
            add_structure(bounds, world.render);
            scene.end_tick();
        }
        void begin(const PlayerInputData& actions = input())
        {
            emitted += begin_step(actions).size();
        }
        void finish() { finish_step(); }
        void tick(const PlayerInputData& actions = input()) { begin(actions); scene.resolve(); finish(); }
        void settle() { tick(); tick(); }
        World& world;
        std::size_t emitted = 0;
    };
    void check_movement(World& world);
    void check_support(World& world);
    void check_recovery(World& world);
    void check_schema_and_bounds(World& world);
    void check_health_regen(World& world);
}
