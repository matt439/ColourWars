#pragma once

#include "game/objects/level_loaded_info.h"
#include "game/objects/level_object_builder.h"
#include "game/objects/player.h"
#include "game/objects/weapon_builder.h"
#include "game/presentation.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/collision/narrow_phase.h"
#include "engine/core/step_timer.h"
#include "engine/scene/scene.h"

#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>

namespace turbulence_blockout_acceptance
{
    using namespace labrador;
    using namespace mattmath;
    inline const float dt = static_cast<float>(StepTimer::TicksToSeconds(StepTimer::SecondsToTicks(1.0 / 60.0)));
    inline constexpr float tolerance = 0.05f;
    void require(bool value, const std::string& reason);
    bool supported(const Player& player);
    PlayerInputData input(float movement = 0.0f, bool pressed = false, bool held = false, bool drop = false);

    struct Surface
    {
        std::string name;
        std::string type;
        std::string collision;
        RectangleF rectangle;
        bool ramp() const { return this->type == "StructureRamp"; }
        bool one_way() const { return this->collision == "STRUCTURE_JUMP_THROUGH"; }
    };

    struct Services
    {
        RenderResources render;
        AudioResources audio;
        PlaySettings preferences;
        GameTuning tuning;
        Presentation presentation;
        TeamColour colours;
        LevelLoadedInfo info;
        LevelObjectBuilder builder;
        std::vector<Surface> surfaces;
        explicit Services(const std::filesystem::path& content);
        const Surface& named(const std::string& name) const;
    };

    struct Run
    {
        Services& services;
        Scene scene{ nullptr, nullptr };
        Player* player = nullptr;
        std::vector<std::pair<const Surface*, CollisionObject*>> structures;
        int ticks = 0;
        int takeoffs = 0;
        bool death_expected = false;
        Run(Services& services, const Vector2F& position, PlayerTeam team = PlayerTeam::a,
            float initial_velocity = 0.0f);
        void tick(const PlayerInputData& actions = input());
        void settle(int ticks = 45);
        std::string support() const;
        void require_clear() const;
    };

    float reflected(bool right, float centre_x);
    Vector2F standing(const Services& services, const Surface& surface, float centre_x);
    float steering(const Run& run, float centre_x);
    void drive(Run& run, float centre_x, int limit = 1000);
    void jump(Run& run, const std::string& target, float centre_x, int delay = 0, int hold_ticks = 120);
    void support_is(const Run& run, const std::string& support);
    void geometry(Services& services, const std::filesystem::path& output);
    void routes(Services& services);
    void public_load(Services& services, const std::filesystem::path& content);
}
