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
#include <sstream>

namespace crossfeed_acceptance
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
        // Receives the absolute Run tick before Scene::update. An empty stream
        // retains the earlier idle-opponent behaviour.
        using PlayerInputStream = std::function<PlayerInputData(int)>;
        Services& services;
        Scene scene{ nullptr, nullptr };
        Player* player = nullptr;
        std::vector<std::pair<const Surface*, CollisionObject*>> structures;
        std::vector<CollisionObject*> targets;
        std::vector<Player*> passive_players;
        std::vector<PlayerInputStream> passive_input_streams;
        // Optional scripted tracking at the firing phase, after both players
        // have moved. Empty preserves the ordinary supplied input angle.
        std::function<float()> weapon_aim_after_movement;
        // Observe public collider positions after integration and before any
        // sweep/contact retirement. Empty leaves the ordinary tick unchanged.
        std::function<void()> after_scene_update;
        // Returned launch survivors are still owned here, before Scene takes
        // them. Player has already removed launch-contact retirements.
        std::function<void(std::span<const std::unique_ptr<Projectile>>)> on_weapon_launch;
        // Observe resolved objects before end_tick removes retirements and
        // inserts this tick's pending launch survivors.
        std::function<void()> after_scene_contacts;
        int ticks = 0;
        int paid_events = 0;
        int emitted_objects = 0;
        int paid_events_without_flight = 0;
        std::vector<int> paid_ticks;
        int launch_retired_objects = 0;
        int swept_retired_objects = 0;
        int first_paid_tick = -1;
        Vector2F first_paid_velocity;
        Vector2F first_paid_nozzle;
        int takeoffs = 0;
        bool death_expected = false;
        float minimum_ammo = 0.0f;
        float swept_paint = 0.0f;
        float launch_paint = 0.0f;
        // Empty watches all scored surfaces. A flight station names one
        // target so unrelated flying paint cannot satisfy its assertion.
        std::string swept_surface;
        std::string interception_surface;
        int cover_contacts = 0;
        int first_cover_tick = -1;
        Vector2F first_cover_point;

        Run(Services& with, const Vector2F& top_left,
            PlayerTeam team = PlayerTeam::a, WeaponType weapon = WeaponType::sprayer);
        void tick(const PlayerInputData& actions = input());
        void settle(int count = 60);
        void require_clear(const std::string& context) const;
        std::string support() const;
        float team_area(PlayerTeam team, const std::string& name = {}) const;
        float total_area() const;
        void kill();
        Player* add_opponent(const Vector2F& top_left);
        void set_opponent_input(Player* opponent, PlayerInputStream stream);
    };

    Vector2F standing_on(const Services& s, const Surface& surface, float centre_x);
    void drive_to(Run& run, float centre_x, int limit = 900);
    void jump_to(Run& run, const Surface& target, float centre_x, int delay, int hold_ticks = 120);
    void geometry_checks(Services& s, const std::filesystem::path& output);
    void spawn_checks(Services& s);
    void route_checks(Services& s);
    void tool_checks(Services& s);
    void combat_checks(Services& s);
    void pressure_checks(Services& s);
    void trajectory_checks(Services& s);
    void supply_checks(Services& s);
    void painting_checks(Services& s);
    void repaint_checks(Services& s);
    void home_loop_checks(Services& s);
    void opening_checks(Services& s);
}
