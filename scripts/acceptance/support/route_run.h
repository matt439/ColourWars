#pragma once

// The route-run fixture the Crossfeed, Close Quarters and Press Tower cases
// share: one shipped stage's authored objects built by the production
// LevelObjectBuilder into a bare Scene, and one real Player (plus optional
// scripted opponents) stepped in Level's order. Each case names its stage and
// adds only its own checks. Defined in route_run.cpp, which those cases list
// as a source in cases.psd1.

#include "game/objects/level_loaded_info.h"
#include "game/objects/level_object_builder.h"
#include "game/objects/paintable_object.h"
#include "game/objects/player.h"
#include "game/objects/projectile.h"
#include "game/objects/projectile_spray.h"
#include "game/objects/weapon_builder.h"
#include "game/config/presentation/presentation.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/collision/narrow_phase.h"
#include "engine/core/step_timer.h"
#include "engine/scene/scene.h"
#include "scripts/acceptance/support/probe.h"

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
#include <string>
#include <vector>

namespace acceptance::route_run
{
    using namespace labrador;
    using acceptance::require;

    inline const float dt = static_cast<float>(StepTimer::TicksToSeconds(
        StepTimer::SecondsToTicks(1.0 / 60.0)));
    inline constexpr float tolerance = 0.05f;

    std::string fixed(float value, int precision = 2);
    bool approximately(float first, float second, float allowed = tolerance);
    bool supported(const Player& player);
    PlayerInputData input(float movement = 0.0f, bool pressed = false,
        bool held = false, bool drop = false);
    PlayerInputData aimed(float angle, PlayerInputData actions = input());
    RectangleF mirror(const RectangleF& rectangle, const RectangleF& bounds);

    // Full left or right until `player` can stop on `goal`, braking from its
    // current speed at the tuned deceleration; 0 once it is within 5 units or
    // already coasting to a stop there.
    float steer(const Player& player, float goal, const PlayerTuning& tuning);

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

    // One level file's authored objects. `label` names the stage in failures.
    struct Map
    {
        std::string label;
        LevelLoadedInfo info;
        std::vector<Surface> surfaces;
        Map(const std::filesystem::path& path, std::string label);
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
        // Run::tick totals the subject's paint around the sweep and launch
        // phases (swept_paint, launch_paint). A case that reads neither turns
        // this off; the scans cost a Press Tower run about half again.
        bool paint_accounting = true;
        // `stage` is the level file's stem under content/levels.
        Services(const std::filesystem::path& content, const std::string& stage, std::string label);
    };

    // One real Player, constructed once at the trial's start. Every later
    // position comes from Player input, Scene contact resolution or respawn.
    struct Run
    {
        struct OpponentWeaponStats
        {
            int paid_events = 0;
            int emitted_objects = 0;
            int launch_retired_objects = 0;
            int first_paid_tick = -1;
            std::vector<int> paid_ticks;
            float minimum_ammo = 0.0f;
        };

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
        std::vector<std::function<float()>> passive_weapon_aims;
        std::vector<OpponentWeaponStats> passive_weapon_stats;
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
        void require_clear(const Player& subject, const std::string& context) const;
        std::string support() const;
        float team_area(PlayerTeam team, const std::string& name = {}) const;
        float total_area() const;
        void kill();
        Player* add_opponent(const Vector2F& top_left, WeaponType weapon = WeaponType::sprayer);
        void set_opponent_input(Player* opponent, PlayerInputStream stream);
        void set_opponent_weapon_aim(Player* opponent, std::function<float()> aim);
        const OpponentWeaponStats& opponent_weapon_stats(const Player* opponent) const;
    };

    // How long a jump holds its button by default, and long enough to hold it
    // through jump_to's whole landing window.
    inline constexpr int default_jump_hold = 120;
    inline constexpr int jump_held_throughout = 240;

    Vector2F standing_on(const Services& s, const Surface& surface, float centre_x);
    float steering(const Run& run, float target);
    void drive_to(Run& run, float centre_x, int limit = 900);
    // `single_takeoff` also requires the landing to have taken exactly one
    // takeoff, so a second jump cannot pass for the first.
    void jump_to(Run& run, const Surface& target, float centre_x, int delay,
        int hold_ticks = default_jump_hold, bool single_takeoff = false);
}
