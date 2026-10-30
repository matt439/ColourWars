#pragma once

#include "game/objects/level_loaded_info.h"
#include "game/objects/level_object_builder.h"
#include "game/objects/paintable_object.h"
#include "game/objects/player.h"
#include "game/objects/projectile.h"
#include "game/objects/projectile_spray.h"
#include "game/objects/weapon_builder.h"
#include "game/paint_face_geometry.h"
#include "game/presentation.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/collision/narrow_phase.h"
#include "engine/core/step_timer.h"
#include "engine/scene/scene.h"
#include "scripts/acceptance/support/probe.h"

#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>

namespace cq_blockout_acceptance
{
    using namespace labrador;
    using namespace mattmath;

    inline const float dt = static_cast<float>(StepTimer::TicksToSeconds(
        StepTimer::SecondsToTicks(1.0 / 60.0)));
    inline constexpr float tolerance = 0.05f;

    using acceptance::require;
    std::string fixed(float value, int precision = 2);
    bool approximately(float first, float second, float allowed = tolerance);
    bool supported(const Player& player);
    PlayerInputData input(float movement = 0.0f, bool pressed = false,
        bool held = false, bool drop = false);
    PlayerInputData aimed(float angle, PlayerInputData actions = input());
    RectangleF mirror(const RectangleF& rectangle, const RectangleF& bounds);

    struct Surface
    {
        std::string name;
        std::string type;
        std::string collision_type;
        RectangleF rectangle;
        // top, bottom, left, right; empty for non-scoring structures.
        std::array<bool, 4> faces{};
        bool jump_through() const { return this->collision_type == "STRUCTURE_JUMP_THROUGH"; }
        bool ramp() const { return this->type == "StructureRamp"; }
    };

    struct Map
    {
        LevelLoadedInfo info;
        std::vector<Surface> surfaces;
        explicit Map(const std::filesystem::path& path);
        const Surface& named(const std::string& name) const;
    };

    struct Services
    {
        RenderResources render;
        AudioResources audio;
        PlaySettings preferences;
        GameTuning tuning;
        Presentation presentation;
        TeamColour colours;
        Map map;
        LevelObjectBuilder builder;
        explicit Services(const std::filesystem::path& content);
    };

    // One real Player, constructed once at the trial's start. Every later
    // position comes from Player input, Scene contact resolution or respawn.
    struct Run
    {
        Services& services;
        Scene scene{ nullptr, nullptr };
        Player* player = nullptr;
        std::vector<std::pair<const Surface*, CollisionObject*>> structures;
        std::vector<CollisionObject*> targets;
        int ticks = 0;
        int shots = 0;
        int takeoffs = 0;
        bool death_expected = false;
        float minimum_ammo = 0.0f;
        float swept_paint = 0.0f;
        // Empty watches all scored surfaces. A flight station names one
        // target so unrelated flying paint cannot satisfy its assertion.
        std::string swept_surface;

        Run(Services& with, const Vector2F& top_left,
            PlayerTeam team = PlayerTeam::a, WeaponType weapon = WeaponType::sprayer);
        void tick(const PlayerInputData& actions = input());
        void settle(int count = 60);
        void require_clear(const std::string& context) const;
        std::string support() const;
        float team_area(PlayerTeam team, const std::string& name = {}) const;
        float total_area() const;
        void kill();
    };

    Vector2F standing_on(const Services& s, const Surface& surface, float centre_x);
    void drive_to(Run& run, float centre_x, int limit = 900);
    void jump_to(Run& run, const Surface& target, float centre_x, int delay);
    void geometry_checks(Services& s, const std::filesystem::path& output);
    void spawn_checks(Services& s);
    void route_checks(Services& s);
    void tool_checks(Services& s);
}
