#pragma once

#include "game/debug/dev_tools.h"
#include "game/input_slots.h"
#include "game/objects/level_builder.h"
#include "game/objects/structure_paintable.h"
#include "game/objects/projectile_spray.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/core/file_path.h"
#include "engine/core/step_timer.h"
#include "engine/render/sprite_font_file.h"
#include <bit>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace paint_surfaces_acceptance
{
    inline void require(bool condition, const char* reason)
    { if (!condition) { throw std::runtime_error(reason); } }

    inline std::uint32_t bits(float value) { return std::bit_cast<std::uint32_t>(value); }
    inline void area_near(float actual, float expected)
    { require(std::fabs(actual - expected) < 0.02f, "Snapshot disagrees with analytical exposed area"); }

    // CPU objects only: null atlas handles, silent audio, no Application/device/save.
    class World final
    {
    public:
        explicit World(const std::filesystem::path& content) :
            tuning(labrador::path_to_utf8(content / "tuning.json").c_str()),
            presentation(labrador::path_to_utf8(content / "presentation.json").c_str())
        {
            using namespace labrador;
            for (const char* sheet : { "sprite_sheet_1", "painter", "menu_paint", "weapon_paint", "paint_fx", "terrain" })
            {
                render.add_sprite_sheet(sheet, read_sprite_sheet(path_to_utf8(content / "textures" /
                    path_from_utf8(std::string(sheet) + ".json")).c_str(), TextureHandle{}));
            }
            for (const char* font : { "bowlby_one_144", "lato_black_24", "lato_black_36", "lato_black_48",
                "lato_black_72", "lato_black_144", "courier_prime_bold_16" })
            {
                const SpriteFontFile file = read_sprite_font_file(path_to_utf8(content / "fonts" /
                    path_from_utf8(std::string(font) + ".spritefont")));
                auto loaded = std::make_unique<Font>(TextureHandle{}, file.glyphs, file.line_spacing);
                if (loaded->contains(U'?')) { loaded->set_stand_in(U'?'); }
                render.add_font(font, std::move(loaded));
            }
            audio.add_sound_bank("paint_exe", SoundBank::silent());
            for (const char* stage : { "crossfeed", "press_tower" })
            {
                auto info = std::make_unique<LevelLoadedInfo>(path_to_utf8(content / "levels" /
                    path_from_utf8(std::string(stage) + ".json")).c_str());
                info->validate();
                levels_.add(stage, std::move(info));
            }
            builder_ = std::make_unique<LevelBuilder>(&viewports_, &render, &audio, &preferences,
                &levels_, &tuning, &presentation, &resolution_, nullptr, &partitioner_, &mouse_);
        }
        std::unique_ptr<StructurePaintable> surface(const mattmath::RectangleF& rectangle,
            const PaintableFaces& faces, std::span<const mattmath::RectangleF> obstacles = {})
        {
            return std::make_unique<StructurePaintable>("sprite_sheet_1", "square_white_4", rectangle,
                std::make_unique<mattmath::RectangleF>(rectangle), &render, &audio, &preferences,
                &tuning.paint(), CollisionObjectType::structure_paintable, TeamColour{}, faces, obstacles);
        }
        void paint(StructurePaintable& surface, const mattmath::RectangleF& splash, PlayerTeam team, int owner)
        {
            const ProjectileTuning& spray = tuning.projectile(ProjectileType::spray);
            mattmath::RectangleF launch = splash;
            const mattmath::Vector2F half = spray.details.col_rect_size / 2.0f;
            launch.offset(half.x, half.y);
            const ProjectileSpray shot(launch, mattmath::Vector2F::ZERO, team, owner, labrador::Colour::white,
                &render, resolve_projectile_strip(spray, render), &spray);
            surface.on_contact(shot, mattmath::Vector2F::ZERO, 0.0f);
        }
        std::unique_ptr<Level> level(const std::string& stage, unsigned int seed)
        {
            MenuLevelSettings settings;
            settings.set_player_count(2);
            settings.set_screen_layout(labrador::ScreenLayout::two_player);
            settings.set_stage(stage);
            settings.set_duration(10.0f);
            settings.set_player_team(0, PlayerTeam::a); settings.set_player_team(1, PlayerTeam::b);
            settings.set_player_weapon(0, WeaponType::sprayer); settings.set_player_weapon(1, WeaponType::sprayer);
            settings.begin_input_claims(0);
            require(settings.claim_input_slot(1), "Cannot claim the second real input slot");
            std::srand(seed);
            viewports_.set_layout(settings.screen_layout());
            return builder_->build_level(settings, TeamColourPair::blue_orange, OpeningMode::off);
        }
        GameTuning tuning;
        Presentation presentation;
        labrador::RenderResources render;
        labrador::AudioResources audio;
        PlaySettings preferences;
    private:
        labrador::Registry<LevelLoadedInfo> levels_{ "Level" };
        labrador::ResolutionManager resolution_;
        labrador::ViewportManager viewports_{ &resolution_ };
        labrador::Mouse mouse_;
        labrador::Partitioner partitioner_;
        std::unique_ptr<LevelBuilder> builder_;
    };

    inline std::vector<std::uint64_t> sample_state(const std::vector<PaintSurfaceSample>& samples)
    {
        std::vector<std::uint64_t> result{ static_cast<std::uint64_t>(samples.size()) };
        for (const PaintSurfaceSample& sample : samples)
        {
            for (const mattmath::Vector2F& corner : sample.corners)
            { result.push_back(bits(corner.x)); result.push_back(bits(corner.y)); }
            result.push_back(static_cast<std::uint64_t>(sample.face));
            result.push_back(static_cast<std::uint64_t>(sample.team));
            result.push_back(static_cast<std::uint32_t>(sample.paint_owner));
            result.push_back(bits(sample.scored_area)); result.push_back(sample.source_index);
        }
        return result;
    }
    inline void add_total(PaintTotal& result, const PaintTotal& part)
    {
        result.total += part.total; result.team_a += part.team_a; result.team_b += part.team_b;
        for (std::size_t owner = 0; owner < result.player_areas.size(); ++owner)
        { result.player_areas[owner] += part.player_areas[owner]; }
    }
    inline PaintTotal grouped_total(const std::vector<PaintSurfaceSample>& samples)
    {
        PaintTotal result, group;
        std::size_t source = 0;
        bool started = false;
        for (const PaintSurfaceSample& sample : samples)
        {
            require(std::isfinite(sample.scored_area) && sample.scored_area > 0.0f, "Snapshot has invalid scored area");
            require(!started || sample.source_index >= source, "Snapshot source groups are not in scoring order");
            if (started && sample.source_index != source) { add_total(result, group); group = {}; }
            source = sample.source_index; started = true;
            group.total += sample.scored_area;
            if (sample.team == PlayerTeam::a) { group.team_a += sample.scored_area; }
            else if (sample.team == PlayerTeam::b) { group.team_b += sample.scored_area; }
            if (sample.paint_owner >= 0 && sample.paint_owner < paint_owners::count)
            { group.player_areas[static_cast<std::size_t>(sample.paint_owner)] += sample.scored_area; }
        }
        if (started) { add_total(result, group); }
        return result;
    }
    inline void same_total(const PaintTotal& actual, const PaintTotal& expected)
    {
        require(bits(actual.total) == bits(expected.total) && bits(actual.team_a) == bits(expected.team_a) &&
            bits(actual.team_b) == bits(expected.team_b), "Grouped snapshot totals changed exact scoring arithmetic");
        for (std::size_t owner = 0; owner < actual.player_areas.size(); ++owner)
        { require(bits(actual.player_areas[owner]) == bits(expected.player_areas[owner]), "Snapshot owner area changed exact scoring arithmetic"); }
    }
    inline PaintTotal from_end_info(const LevelEndInfo& info)
    {
        PaintTotal result;
        result.total = info.total_area; result.team_a = info.team_a_area; result.team_b = info.team_b_area;
        result.player_areas = info.player_areas;
        return result;
    }
    inline mattmath::RectangleF static_rectangle(const PaintSurfaceSample& sample)
    {
        const auto& p = sample.corners;
        for (const mattmath::Vector2F& corner : p)
        { require(std::isfinite(corner.x) && std::isfinite(corner.y), "Snapshot corner is not finite"); }
        require(p[0].y == p[1].y && p[1].x == p[2].x && p[2].y == p[3].y && p[3].x == p[0].x &&
            p[1].x > p[0].x && p[2].y > p[1].y, "Current static sample lost its ordered exact rectangle corners");
        return mattmath::RectangleF(p[0].x, p[0].y, p[1].x - p[0].x, p[3].y - p[0].y);
    }
    inline std::vector<PaintSurfaceSample> snapshot(const PaintableObject& object, std::size_t source = 0)
    { std::vector<PaintSurfaceSample> samples; object.append_paint_surfaces(samples, source); return samples; }

    void check_surfaces(World& world);
    void check_levels(World& world);
}
