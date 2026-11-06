#include "game/pch.h"
#include "game/input/input_slots.h"
#include "game/objects/idle_drip.h"
#include "game/objects/level_builder.h"
#include "game/objects/structure.h"
#include "game/config/presentation/presentation.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/core/step_timer.h"
#include "engine/math/triangle.h"
#include "engine/render/sprite_font_file.h"
#include "scripts/acceptance/support/content_world.h"
#include "scripts/acceptance/support/json_fixture.h"
#include "scripts/acceptance/support/probe.h"

#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <type_traits>

using namespace labrador;

namespace
{
    using acceptance::require;

    bool nearly_equal(float first, float second)
    {
        return std::abs(first - second) < 0.0001f;
    }

    const float dt = static_cast<float>(StepTimer::TicksToSeconds(
        StepTimer::SecondsToTicks(1.0 / 60.0)));

    auto observation(const IdleDrip& drip)
    {
        return std::tuple{ drip.attached(), drip.bead_age(), drip.interval_remaining(),
            drip.bead_rectangle(), drip.bounds(),
            std::vector<IdleDrip::Drop>(drip.drops().begin(), drip.drops().end()) };
    }

    void attach(IdleDrip& drip, const Vector2F& nozzle = Vector2F::zero,
        float angle = 0.0f)
    {
        drip.advance(true, nozzle, angle, drip.interval_remaining() + 0.001f, {});
        require(drip.attached() && drip.bead_age() == 0.0f,
            "Quiet interval did not begin a fresh attached bead");
    }

    void detach(IdleDrip& drip, const IdleDripPresentation& look,
        const Vector2F& nozzle = Vector2F::zero)
    {
        attach(drip, nozzle);
        drip.advance(true, nozzle, 0.0f, look.grow_time + look.stretch_time + 0.001f, {});
        require(!drip.attached() && !drip.drops().empty() && drip.drops().back().age == 0.0f,
            "Grown and stretched bead did not detach into a fresh falling drop");
    }

    constexpr std::array supported_tools{ WeaponType::sprayer, WeaponType::sniper, WeaponType::bucket };
    constexpr std::array roster{ WeaponType::sprayer, WeaponType::sniper, WeaponType::bucket, WeaponType::roller };

    // Edited copies of one content file, each in its own scratch file.
    class ContentVariant : public acceptance::JsonVariants
    {
    public:
        explicit ContentVariant(const std::filesystem::path& original) :
            acceptance::JsonVariants(original, L"idle-drip-content") {}
    };

    // Only metadata is loaded. These objects never create a device or a save.
    struct World
    {
        RenderResources render;
        AudioResources audio;
        PlaySettings preferences;
        GameTuning tuning;
        Presentation presentation;
        Registry<LevelLoadedInfo> levels{ "Level" };
        ResolutionManager resolution;
        ViewportManager viewports{ &resolution };
        Partitioner partitioner;
        std::unique_ptr<LevelBuilder> builder;

        explicit World(const std::filesystem::path& content,
            const std::filesystem::path& tuning_source = {}) :
            tuning((tuning_source.empty() ? content / "tuning.json" : tuning_source).string().c_str()),
            presentation((content / "presentation.json").string().c_str())
        {
            acceptance::load_cpu_assets(content, this->render, this->audio);
            auto info = std::make_unique<LevelLoadedInfo>(
                (content / "levels/movement_course.json").string().c_str());
            info->validate();
            this->levels.add("movement_course", std::move(info));
            this->builder = std::make_unique<LevelBuilder>(&this->viewports, &this->render,
                &this->audio, &this->preferences, &this->levels, &this->tuning,
                &this->presentation, &this->resolution, nullptr, &this->partitioner);
        }

        std::unique_ptr<Level> build(float duration = 300.0f,
            const std::array<WeaponType, 4>& tools = roster)
        {
            MatchSetup settings;
            settings.set_stage("movement_course");
            settings.set_player_count(4);
            settings.set_screen_layout(ScreenLayout::four_player);
            settings.set_duration(duration);
            for (int index = 0; index < 4; ++index)
            {
                settings.set_player_team(index, PlayerTeam::a);
                settings.set_player_weapon(index, tools.at(static_cast<size_t>(index)));
            }
            settings.begin_input_claims(0);
            for (int slot = 1; slot < 4; ++slot) { require(settings.claim_input_slot(slot), "Could not claim CPU pad slot"); }
            this->viewports.set_layout(settings.screen_layout());
            return this->builder->build_level(settings, TeamColourPair::blue_orange, OpeningMode::off, MatchRandom());
        }

        std::unique_ptr<Structure> structure(std::unique_ptr<Shape> shape,
            CollisionObjectType type = CollisionObjectType::structure)
        {
            const RectangleF rectangle = shape->bounding_box();
            return std::make_unique<Structure>("sprite_sheet_1", "square_white_4", rectangle,
                std::move(shape), &this->render, type);
        }
    };
}

#include "scripts/acceptance/idle_drip/standalone_checks.h"
#include "scripts/acceptance/idle_drip/level_checks.h"

int main(int argc, char** argv)
{
    return acceptance::run_probe("idle drip", [&]
    {
        require(argc == 2, "Usage: idle_drip.exe <content directory>");
        const std::filesystem::path content(argv[1]);
        World world(content);
        check_profiles(world, content);
        check_quiet_intervals(world);
        check_attachment_and_falling(world);
        check_terrain(world);
        check_freeze_disable_and_capacity(world);
        check_live_preference(world);
        check_level_integration(world, content);
        check_empty_held_fire(content);
        check_round_end(content);
        return 0;
    });
}
