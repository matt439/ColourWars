#pragma once

#include "game/objects/level_loaded_info.h"
#include "game/objects/level_object_builder.h"
#include "game/objects/paintable_object.h"
#include "game/objects/player.h"
#include "game/objects/projectile.h"
#include "game/objects/projectile_spray.h"
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
#include <functional>
#include <iomanip>
#include <iostream>
#include <optional>
#include <span>
#include <sstream>

namespace press_tower_acceptance
{
    using namespace labrador;
    using namespace mattmath;

    inline const float dt = static_cast<float>(StepTimer::TicksToSeconds(
        StepTimer::SecondsToTicks(1.0 / 60.0)));
    inline constexpr float tolerance = 0.05f;

    void require(bool condition, const std::string& message);
    std::string fixed(float value, int precision = 2);
    bool approximately(float first, float second, float allowed = tolerance);
    bool supported(const Player& player);
    PlayerInputData input(float movement = 0.0f, bool pressed = false,
        bool held = false, bool drop = false);
    PlayerInputData aimed(float angle, PlayerInputData actions = input());
    RectangleF mirror(const RectangleF& rectangle, const RectangleF& bounds);
    // Liang-Barsky. A segment meets the closed rectangle only over a
    // positive length; grazing a corner does not count.
    bool segment_hits(const Vector2F& from, const Vector2F& to, const RectangleF& r);

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
        int paid_events = 0;
        int emitted_objects = 0;
        int takeoffs = 0;
        bool death_expected = false;
        float minimum_ammo = 0.0f;
        // Live-fire observers: launches before they enter the Scene, and the
        // resolved Scene before end_tick can retire a projectile that hit.
        std::function<void(std::span<const std::unique_ptr<Projectile>>)> on_weapon_launch;
        std::function<void()> after_scene_contacts;

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

    // The stage mirrors about the out-of-bounds centre (X 1800). Team A's
    // routes are authored left to right; `right == false` reflects them.
    float reflected_x(const Services& s, bool right, float x);
    Vector2F standing_on(const Services& s, const Surface& surface, float centre_x);
    float steering(const Run& run, float target);
    void drive_to(Run& run, float centre_x, int limit = 900);
    void jump_to(Run& run, const Surface& target, float centre_x, int delay, int hold_ticks = 120);
    // Leaves the current one-way support by holding Down only until the body
    // is airborne, then releases, so the next one-way tier below catches it.
    void drop_once(Run& run, const std::string& expected_support);
    void require_support(const Run& run, const std::string& name, const std::string& context);
    void geometry_checks(Services& s, const std::filesystem::path& output);
    void spawn_checks(Services& s);
    void route_checks(Services& s);
    void tool_checks(Services& s);
    void painting_checks(Services& s);
    void live_fire_checks(Services& s);
}
