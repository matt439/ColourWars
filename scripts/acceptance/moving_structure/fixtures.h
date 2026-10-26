#pragma once

#include "game/objects/level.h"
#include "game/objects/structure.h"
#include "game/objects/structure_paintable.h"
#include "game/objects/translating_structure_paintable.h"
#include "game/objects/translated_airbrush_shadow.h"
#include "game/objects/projectile_jet.h"
#include "game/objects/weapon_builder.h"
#include "game/paint_owners.h"
#include "game/input_slots.h"
#include "game/presentation.h"
#include "game/simulation.h"
#include "engine/assets/json.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/core/file_path.h"
#include "engine/core/step_timer.h"
#include "engine/render/sprite_font_file.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace moving_structure_acceptance
{
    inline void require(bool value, const std::string& reason)
    { if (!value) { throw std::runtime_error(reason); } }
    // A shortened test countdown must retain the schema's contained push beat.
    inline void set_countdown(rapidjson::Document& document, float seconds)
    {
        auto& match = document["match"];
        match["countdown"].SetFloat(seconds);
        auto& push = match["opening"]["push"];
        push.SetFloat((std::min)(seconds, push.GetFloat()));
    }
    inline std::uint32_t bits(float value) { return std::bit_cast<std::uint32_t>(value); }
    inline std::uint64_t double_bits(double value) { return std::bit_cast<std::uint64_t>(value); }
    inline void require_near(float actual, float expected, float tolerance = 0.002f)
    { require(std::isfinite(actual) && std::fabs(actual - expected) <= tolerance, "Unexpected actual moving-structure value"); }
    inline const float dt = static_cast<float>(labrador::StepTimer::TicksToSeconds(
        labrador::StepTimer::SecondsToTicks(1.0 / static_cast<double>(SIMULATION_HZ))));
    inline PlayerInputData input(float movement = 0.0f, bool jump = false,
        bool fire = false, bool drop = false)
    {
        PlayerInputData value;
        value.connected = true; value.x_movement = movement;
        value.jump_pressed = jump; value.jump_held = jump;
        value.primary_shoot = fire; value.drop_through = drop;
        value.shoot_direction_requested = true; value.shoot_direction = { 1.0f, 0.0f };
        value.shoot_angle = 0.0f;
        return value;
    }
    inline PlayerInputData aimed_input(const mattmath::Vector2F& direction, float angle,
        bool fire = true, float movement = 0.0f)
    {
        PlayerInputData value = input(movement, false, fire);
        value.shoot_direction = direction; value.shoot_angle = angle;
        return value;
    }
    template<class Action> void throws_with(Action action, const std::string& fragment)
    {
        bool rejected = false;
        try { action(); }
        catch (const std::exception& error)
        {
            rejected = true;
            require(std::string(error.what()).find(fragment) != std::string::npos,
                "Refusal diagnostic did not identify the expected cause: " + std::string(error.what()));
        }
        require(rejected, "Unsupported moving input was accepted: " + fragment);
    }
    inline void rectangle_bits(std::vector<std::uint64_t>& out, const mattmath::RectangleF& rectangle)
    {
        for (float value : { rectangle.x, rectangle.y, rectangle.width, rectangle.height })
        { out.push_back(bits(value)); }
    }
    inline std::vector<std::uint64_t> motion_bits(const HorizontalMotionState& state)
    {
        std::vector<std::uint64_t> out;
        rectangle_bits(out, state.spec.anchor);
        out.push_back(bits(state.spec.travel_x)); out.push_back(bits(state.spec.period_seconds));
        out.push_back(bits(state.spec.initial_phase)); out.push_back(double_bits(state.phase));
        out.push_back(static_cast<std::uint32_t>(state.direction));
        for (float value : { state.previous_offset.x, state.previous_offset.y,
            state.current_offset.x, state.current_offset.y }) { out.push_back(bits(value)); }
        return out;
    }
    inline std::vector<std::uint64_t> step_bits(const HorizontalMotionStep& step)
    {
        std::vector<std::uint64_t> out{ bits(step.dt), step.segment_count };
        rectangle_bits(out, step.before); rectangle_bits(out, step.after);
        for (std::uint32_t index = 0; index < step.segment_count; ++index)
        {
            const HorizontalMotionSegment& segment = step.segments.at(index);
            out.push_back(double_bits(segment.begin)); out.push_back(double_bits(segment.end));
            out.push_back(bits(segment.from_x)); out.push_back(bits(segment.to_x));
        }
        return out;
    }
    inline std::vector<PaintSurfaceSample> snapshot(const PaintableObject& source, std::size_t index = 0)
    { std::vector<PaintSurfaceSample> out; source.append_paint_surfaces(out, index); return out; }
    inline std::vector<std::uint64_t> sample_bits(const std::vector<PaintSurfaceSample>& samples)
    {
        std::vector<std::uint64_t> out{ static_cast<std::uint64_t>(samples.size()) };
        for (const PaintSurfaceSample& sample : samples)
        {
            for (const auto& point : sample.corners) { out.push_back(bits(point.x)); out.push_back(bits(point.y)); }
            out.push_back(static_cast<std::uint32_t>(sample.face)); out.push_back(static_cast<std::uint32_t>(sample.team));
            out.push_back(static_cast<std::uint32_t>(sample.paint_owner));
            out.push_back(bits(sample.scored_area)); out.push_back(sample.source_index);
        }
        return out;
    }
    inline void same_total(const PaintTotal& left, const PaintTotal& right)
    {
        require(bits(left.total) == bits(right.total) && bits(left.team_a) == bits(right.team_a) &&
            bits(left.team_b) == bits(right.team_b), "Moving read changed exact paint totals");
        for (std::size_t owner = 0; owner < left.player_areas.size(); ++owner)
        { require(bits(left.player_areas[owner]) == bits(right.player_areas[owner]), "Moving read changed raw-owner total"); }
    }

    // Resources and source variants only: no Application/window/device/player save.
    class World final
    {
    public:
        explicit World(const std::filesystem::path& content) :
            tuning(labrador::path_to_utf8(content / "tuning.json").c_str()),
            presentation(labrador::path_to_utf8(content / "presentation.json").c_str())
        {
            using namespace labrador;
            std::ifstream stream(content / "tuning.json", std::ios::binary);
            require(stream.good(), "Cannot read source tuning for moving fixtures");
            source.assign(std::istreambuf_iterator<char>(stream), {});
            const JsonDocument manifest = read_json_file(path_to_utf8(content / "manifest.json").c_str());
            const JsonValue assets = manifest.root().array("assets");
            for (std::size_t index = 0; index < assets.size(); ++index)
            {
                const JsonValue asset = assets.at(index);
                const std::string kind = asset.identifier("kind");
                const std::filesystem::path directory = content / path_from_utf8(asset.identifier("directory"));
                const JsonValue names = asset.array("names");
                for (std::size_t entry = 0; entry < names.size(); ++entry)
                {
                    const std::string name = names.at(entry).as_identifier();
                    if (kind == "sprite_sheet")
                    {
                        render.add_sprite_sheet(name, read_sprite_sheet(path_to_utf8(
                            directory / path_from_utf8(name + ".json")).c_str(), TextureHandle{}));
                    }
                    else if (kind == "font")
                    {
                        const SpriteFontFile file = read_sprite_font_file(path_to_utf8(directory /
                            path_from_utf8(name + ".spritefont")));
                        auto font = std::make_unique<Font>(TextureHandle{}, file.glyphs, file.line_spacing);
                        if (font->contains(U'?')) { font->set_stand_in(U'?'); }
                        render.add_font(name, std::move(font));
                    }
                    else if (kind == "sound_bank") { audio.add_sound_bank(name, SoundBank::silent()); }
                    else { require(kind == "level", "Unknown manifest resource kind in moving fixture"); }
                }
            }
            tuning.validate_resources(render, audio); presentation.validate_resources(render, audio);
            palette = TeamColourTools::team_colours(TeamColourPair::blue_orange, presentation.team_palette());
        }
        template<class Edit> GameTuning variant(Edit edit) const
        {
            rapidjson::Document document; document.Parse(source.c_str());
            require(!document.HasParseError(), "Cannot parse source moving fixture tuning");
            edit(document); rapidjson::StringBuffer buffer; rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            require(document.Accept(writer), "Cannot encode isolated moving fixture tuning");
            return GameTuning(labrador::parse_json(buffer.GetString(), "moving-fixture-variant.json"));
        }
        std::unique_ptr<Weapon> weapon(const GameTuning& values, WeaponType tool,
            PlayerTeam team, int owner, const mattmath::Vector2F& center = mattmath::Vector2F::ZERO, int view = 0)
        {
            return WeaponBuilder::build_weapon(&values, team, owner, view, palette.team_colour(team),
                tool, center, &render, &audio, &preferences, &presentation.tool_motion(), &presentation.weapon_feedback());
        }
        std::unique_ptr<Weapon> weapon(WeaponType tool, PlayerTeam team = PlayerTeam::a,
            int owner = 5, const mattmath::Vector2F& center = mattmath::Vector2F::ZERO)
        { return weapon(tuning, tool, team, owner, center); }
        std::unique_ptr<StructurePaintable> surface(const GameTuning& values,
            const mattmath::RectangleF& rectangle, const PaintableFaces& faces,
            std::span<const mattmath::RectangleF> obstacles = {},
            CollisionObjectType type = CollisionObjectType::structure_paintable)
        {
            return std::make_unique<StructurePaintable>("sprite_sheet_1", "square_white_4", rectangle,
                std::make_unique<mattmath::RectangleF>(rectangle), &render, &audio, &preferences,
                &values.paint(), type, palette, faces, obstacles);
        }
        std::unique_ptr<StructurePaintable> surface(const mattmath::RectangleF& rectangle,
            const PaintableFaces& faces, std::span<const mattmath::RectangleF> obstacles = {},
            CollisionObjectType type = CollisionObjectType::structure_paintable)
        { return surface(tuning, rectangle, faces, obstacles, type); }
        std::unique_ptr<TranslatingStructurePaintable> mover(const GameTuning& values,
            const HorizontalMotionSpec& spec, const PaintableFaces& faces = { true, true, true, true },
            CollisionObjectType type = CollisionObjectType::structure_paintable,
            std::unique_ptr<const TerrainMaterial> material = nullptr)
        {
            return std::make_unique<TranslatingStructurePaintable>("sprite_sheet_1", "square_white_4", spec,
                &render, &audio, &preferences, &values.paint(), type, palette, faces,
                labrador::Colour::white, mattmath::Vector2F::ZERO, labrador::SpriteFlip::none, 0.0f, std::move(material));
        }
        std::unique_ptr<TranslatingStructurePaintable> mover(const HorizontalMotionSpec& spec,
            const PaintableFaces& faces = { true, true, true, true },
            CollisionObjectType type = CollisionObjectType::structure_paintable,
            std::unique_ptr<const TerrainMaterial> material = nullptr)
        { return mover(tuning, spec, faces, type, std::move(material)); }
        void paint(StructurePaintable& surface, const mattmath::RectangleF& splash,
            PlayerTeam team, int owner = -1)
        {
            const ProjectileTuning& jet = tuning.projectile(ProjectileType::jet);
            auto launch = splash; const auto half = jet.details.col_rect_size / 2.0f; launch.offset(half.x, half.y);
            const ProjectileJet shot(launch, mattmath::Vector2F::ZERO, team, owner, palette.team_colour(team),
                &render, resolve_projectile_strip(jet, render), &jet);
            surface.on_contact(shot, mattmath::Vector2F::ZERO, 0.0f);
        }
        GameTuning tuning;
        Presentation presentation;
        labrador::RenderResources render;
        labrador::AudioResources audio;
        PlaySettings preferences;
        TeamColour palette;
        std::string source;
    };

    // Manual real Level construction. Geometry is fixed before build(); only
    // controlled projectile insertion may use the retained Scene after transfer.
    class Fixture final
    {
    public:
        explicit Fixture(World& resources, int player_count = 1,
            const mattmath::RectangleF& bounds = { -1024.0f, -1024.0f, 8192.0f, 4096.0f },
            unsigned int seed = 17) : world(resources), tuning(resources.tuning),
            viewports(&resolution), pending_(std::make_unique<labrador::Scene>(nullptr, nullptr)),
            expected_players_(player_count), bounds_(bounds)
        {
            require(player_count >= 1 && player_count <= 4, "Fixture player count must be1..4");
            viewports.set_layout(player_count == 1 ? labrador::ScreenLayout::one_player :
                player_count == 2 ? labrador::ScreenLayout::two_player :
                player_count == 3 ? labrador::ScreenLayout::three_player : labrador::ScreenLayout::four_player);
            std::srand(seed); scene = pending_.get(); scene->set_bounds(bounds);
        }
        StructurePaintable& surface(const mattmath::RectangleF& rectangle,
            const PaintableFaces& faces = { true, true, true, true },
            std::span<const mattmath::RectangleF> obstacles = {},
            CollisionObjectType type = CollisionObjectType::structure_paintable)
        {
            require(pending_ != nullptr, "Terrain cannot be added after Level admission");
            return *scene->add(world.surface(tuning, rectangle, faces, obstacles, type));
        }
        Structure& plain(const mattmath::RectangleF& rectangle)
        {
            require(pending_ != nullptr, "Terrain cannot be added after Level admission");
            return *scene->add(std::make_unique<Structure>("sprite_sheet_1", "square_white_4", rectangle,
                std::make_unique<mattmath::RectangleF>(rectangle), &world.render, CollisionObjectType::structure));
        }
        TranslatingStructurePaintable& mover(const HorizontalMotionSpec& spec,
            const PaintableFaces& faces = { true, true, true, true },
            CollisionObjectType type = CollisionObjectType::structure_paintable,
            std::unique_ptr<const TerrainMaterial> material = nullptr)
        {
            require(pending_ != nullptr, "Mover cannot be added after Level admission");
            auto* result = scene->add(world.mover(tuning, spec, faces, type, std::move(material)));
            movers.push_back(result); return *result;
        }
        Player& add_player(const mattmath::RectangleF& body, PlayerTeam team = PlayerTeam::a,
            WeaponType tool = WeaponType::sprayer, int slot = -1,
            const mattmath::Vector2F& initial_velocity = mattmath::Vector2F::ZERO)
        {
            require(pending_ != nullptr && players.size() < static_cast<std::size_t>(expected_players_),
                "Players must be constructed before Level admission");
            const int view = static_cast<int>(players.size());
            auto primary = world.weapon(tuning, tool, team, paint_owners::index(slot, view), body.center(), view);
            auto* result = scene->add(std::make_unique<Player>(body, &tuning,
                &world.presentation.movement_feedback(), &world.presentation.combat_feedback(),
                &world.presentation.respawn_redraw(), &world.render, &world.audio, &world.preferences,
                slot, team, tool, std::move(primary), WeaponType::grenade_std, world.palette.team_colour(team),
                viewports.player_viewport(view), body.top_left(), initial_velocity,
                0.0f, mattmath::Vector2F::ZERO, labrador::SpriteFlip::none, 0.0f, view,
                &world.presentation.figure(), &world.presentation.camera().shake));
            players.push_back(result); return *result;
        }
        Player& add_player(float center_x, float feet_y, PlayerTeam team = PlayerTeam::a,
            WeaponType tool = WeaponType::sprayer, int slot = -1,
            const mattmath::Vector2F& initial_velocity = mattmath::Vector2F::ZERO)
        {
            const auto size = tuning.player().size;
            return add_player({ center_x - size.x / 2.0f, feet_y - size.y, size.x, size.y }, team, tool, slot, initial_velocity);
        }
        void build(OpeningMode opening = OpeningMode::off)
        {
            require(pending_ != nullptr && players.size() == static_cast<std::size_t>(expected_players_),
                "Fixture Level requires its complete real roster");
            level = std::make_unique<Level>(std::move(pending_), players,
                std::vector<std::unique_ptr<labrador::GameObject>>{}, world.palette,
                bounds_, bounds_, bounds_, &tuning, &world.presentation, L"Moving capability fixture",
                "paint_exe", "", 0.0f, "", 0.0f, &resolution, &viewports, &world.render,
                &world.audio, &world.preferences, &mouse, std::vector<LevelStation>{}, opening, 120.0f);
        }
        void tick(const std::vector<PlayerInputData>& actions = {}, float seconds = dt,
            SeatInputFilter* next = nullptr)
        {
            require(level != nullptr, "Fixture Level was not built");
            Inputs filter(actions.empty() ? std::vector<PlayerInputData>(players.size(), input()) : actions, next);
            level->update({}, seconds, &filter);
        }
        void tick(const PlayerInputData& action, float seconds = dt, SeatInputFilter* next = nullptr)
        { tick(std::vector<PlayerInputData>(players.size(), action), seconds, next); }
        void start(int maximum_steps = 500)
        {
            require(level != nullptr, "Fixture Level was not built");
            for (int step = 0; !level->match_started() && step < maximum_steps; ++step) { tick(); }
            require(level->match_started(), "Moving fixture did not finish bounded normal count-in");
        }
        void settle(int steps = 3) { for (int step = 0; step < steps; ++step) { tick(); } }
        World& world;
        GameTuning tuning;
        labrador::ResolutionManager resolution;
        labrador::ViewportManager viewports;
        labrador::Mouse mouse;
        labrador::Scene* scene = nullptr; // Borrowed actual owner; no terrain mutation after build.
        std::vector<Player*> players;
        std::vector<TranslatingStructurePaintable*> movers;
        std::unique_ptr<Level> level;
    private:
        class Inputs final : public SeatInputFilter
        {
        public:
            Inputs(std::vector<PlayerInputData> values, SeatInputFilter* next) : values_(std::move(values)), next_(next) {}
            void filter_seats(std::vector<PlayerInputData>& values, const Level& source) override
            {
                require(values.size() == values_.size(), "Per-view fixture input does not match roster");
                values = values_;
                if (next_ != nullptr) { next_->filter_seats(values, source); }
            }
        private:
            std::vector<PlayerInputData> values_;
            SeatInputFilter* next_ = nullptr;
        };
        std::unique_ptr<labrador::Scene> pending_;
        int expected_players_;
        mattmath::RectangleF bounds_;
    };

    void check_motion(World& world);
    void check_loader(World& world);
    void check_paint(World& world);
    void check_support(World& world);
    void check_projectiles(World& world);
    void check_draw_cpu(World& world);
    void check_replay(World& world);
    void check_public_replay(World& world);
    void check_editor_save(World& world);
}
