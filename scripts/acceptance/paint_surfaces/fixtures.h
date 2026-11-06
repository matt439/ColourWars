#pragma once

#include "game/debug/dev_tools.h"
#include "game/input/input_slots.h"
#include "game/objects/level_builder.h"
#include "game/objects/structure_paintable.h"
#include "game/objects/projectile_spray.h"
#include "game/config/presentation/presentation.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/core/file_path.h"
#include "engine/core/step_timer.h"
#include "engine/render/sprite_font_file.h"
#include "scripts/acceptance/support/content_world.h"
#include "scripts/acceptance/support/paint_samples.h"
#include "scripts/acceptance/support/probe.h"
#include "engine/input/mouse.h"
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
    using acceptance::require;

    using acceptance::bits;
    using acceptance::sample_state;
    using acceptance::same_total;
    using acceptance::snapshot;
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
            acceptance::load_cpu_assets(content, render, audio);
            for (const char* stage : { "crossfeed", "press_tower" })
            {
                auto info = std::make_unique<LevelLoadedInfo>(path_to_utf8(content / "levels" /
                    path_from_utf8(std::string(stage) + ".json")).c_str());
                info->validate();
                levels_.add(stage, std::move(info));
            }
            builder_ = std::make_unique<LevelBuilder>(&viewports_, &render, &audio, &preferences,
                &levels_, &tuning, &presentation, &resolution_, nullptr, &partitioner_);
        }
        std::unique_ptr<StructurePaintable> surface(const labrador::RectangleF& rectangle,
            const PaintableFaces& faces, std::span<const labrador::RectangleF> obstacles = {})
        {
            return std::make_unique<StructurePaintable>("sprite_sheet_1", "square_white_4", rectangle,
                std::make_unique<labrador::RectangleF>(rectangle), &render, &preferences,
                &tuning.paint(), CollisionObjectType::structure_paintable, TeamColour{}, faces, obstacles);
        }
        void paint(StructurePaintable& surface, const labrador::RectangleF& splash, PlayerTeam team, int owner)
        {
            const ProjectileTuning& spray = tuning.projectile(ProjectileType::spray);
            labrador::RectangleF launch = splash;
            const labrador::Vector2F half = spray.details.col_rect_size / 2.0f;
            launch.offset(half.x, half.y);
            const ProjectileSpray shot(launch, labrador::Vector2F::zero, team, owner, labrador::Colour::white,
                &render, resolve_projectile_strip(spray, render), &spray);
            surface.on_contact(shot, labrador::Vector2F::zero, 0.0f);
        }
        std::unique_ptr<Level> level(const std::string& stage, unsigned int seed)
        {
            MatchSetup settings;
            settings.set_player_count(2);
            settings.set_screen_layout(labrador::ScreenLayout::two_player);
            settings.set_stage(stage);
            settings.set_duration(10.0f);
            settings.set_player_team(0, PlayerTeam::a); settings.set_player_team(1, PlayerTeam::b);
            settings.set_player_weapon(0, WeaponType::sprayer); settings.set_player_weapon(1, WeaponType::sprayer);
            settings.begin_input_claims(0);
            require(settings.claim_input_slot(1), "Cannot claim the second real input slot");
            // Kept for the probes that fold the next rand() into their state.
            std::srand(seed);
            viewports_.set_layout(settings.screen_layout());
            return builder_->build_level(settings, TeamColourPair::blue_orange, OpeningMode::off,
                MatchRandom(seed));
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
    inline PaintTotal from_end_info(const LevelEndInfo& info)
    {
        PaintTotal result;
        result.total = info.total_area; result.team_a = info.team_a_area; result.team_b = info.team_b_area;
        result.player_areas = info.player_areas;
        return result;
    }
    inline labrador::RectangleF static_rectangle(const PaintSurfaceSample& sample)
    {
        const auto& p = sample.corners;
        for (const labrador::Vector2F& corner : p)
        { require(std::isfinite(corner.x) && std::isfinite(corner.y), "Snapshot corner is not finite"); }
        require(p[0].y == p[1].y && p[1].x == p[2].x && p[2].y == p[3].y && p[3].x == p[0].x &&
            p[1].x > p[0].x && p[2].y > p[1].y, "Current static sample lost its ordered exact rectangle corners");
        return labrador::RectangleF(p[0].x, p[0].y, p[1].x - p[0].x, p[3].y - p[0].y);
    }

    void check_surfaces(World& world);
    void check_levels(World& world);
}
