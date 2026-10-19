#pragma once

// What the movement_course trials share: the fixed step and contact
// tolerances, the input, aim and pad helpers, the authored course and its
// Gap routes, the Services main builds, the Run that steps one real Player
// through a fresh Scene, the placement and drive helpers, and the trials
// main calls. structure_paintable.h is included plainly here; tool_trials.cpp
// includes it first with private opened, so this include is then a no-op.
#include "game/play_settings.h"
#include "game/tuning.h"
#include "game/objects/paintable_object.h"
#include "game/objects/structure.h"
#include "game/objects/paint_tile.h"
#include "game/objects/team_colour.h"
#include "engine/audio/audio_resources.h"
#include "engine/audio/sound_bank.h"
#include "engine/render/colour.h"
#include "game/objects/structure_paintable.h"
#include "game/objects/level_loaded_info.h"
#include "game/objects/level_object_builder.h"
#include "game/objects/player.h"
#include "game/objects/player_input.h"
#include "game/objects/projectile.h"
#include "game/objects/projectile_spray.h"
#include "game/objects/weapon_builder.h"
#include "game/presentation.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/core/step_timer.h"
#include "engine/input/gamepad.h"
#include "engine/scene/scene.h"

#include <filesystem>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>

namespace movement_course_acceptance
{
    using namespace labrador;
    using namespace mattmath;

    void require(bool condition, const char* message);
    [[noreturn]] void fail(const std::string& message);

    inline const float dt = static_cast<float>(StepTimer::TicksToSeconds(
        StepTimer::SecondsToTicks(1.0 / 60.0)));

    // Contact separation is exact; these allow only float noise.
    inline constexpr float overlap_tolerance = 0.01f;
    inline constexpr float surface_tolerance = 0.05f;

    std::string fixed(float value, int precision = 1);
    PlayerInputData input(bool pressed = false, bool held = false,
        float movement = 0.0f, bool drop = false);
    PlayerInputData aimed(float angle, PlayerInputData actions = input());
    float pad_movement(float raw_x, const GameTuning& tuning);
    bool pad_drop(float raw_y, const GameTuning& tuning);
    PlayerInputData pad_aim(const Vector2F& raw, const PlaySettings& preferences,
        PlayerInputData actions = input());
    bool window_open(float window, int elapsed);
    bool supported(const Player& player);

    struct Surface
    {
        std::string name;
        RectangleF rectangle;
        bool jump_through = false;
    };

    // The authored stage through the game's own loader. Trials name objects
    // rather than coordinates, so a renamed or deleted object fails loudly.
    struct CourseData
    {
        LevelLoadedInfo info;
        std::vector<Surface> surfaces;

        explicit CourseData(const std::filesystem::path& path) : info(path.string().c_str())
        {
            this->info.validate();
            const JsonValue objects = this->info.collision_objects_json();
            for (size_t index = 0; index < objects.size(); ++index)
            {
                const JsonValue object = objects.at(index);
                const JsonValue shape = object.object("rectangle");
                this->surfaces.push_back({ object.string("name"),
                    RectangleF(shape.number("x"), shape.number("y"),
                        shape.number("width"), shape.number("height")),
                    object.string("collision_type") == "STRUCTURE_JUMP_THROUGH" });
            }
            for (const char* name : { "recovery_floor", "spawn_a_runway",
                "short_landing_long_takeoff", "long_landing_running_takeoff",
                "running_landing", "spawn_b_runway", "short_step", "held_step",
                "high_step", "high_step_return", "short_step_return", "low_ceiling",
                "drop_through_platform", "short_gap_recovery_low", "short_gap_recovery_high",
                "long_gap_recovery_low", "long_gap_recovery_high",
                "running_gap_recovery_low", "running_gap_recovery_high" })
            {
                std::ignore = this->named(name);
            }
        }

        const Surface& named(const std::string& name) const
        {
            for (const Surface& surface : this->surfaces)
            {
                if (surface.name == name) { return surface; }
            }
            fail("Movement Course object '" + name + "' is missing; the route trials depend on it");
        }
    };

    struct Services
    {
        const GameTuning& tuning;
        const MovementFeedbackPresentation& feedback;
        const CombatFeedbackPresentation& combat;
        const RespawnRedrawPresentation& redraw;
        RenderResources& render;
        const AudioResources& audio;
        const PlaySettings& preferences;
        const LevelObjectBuilder& builder;
        const TeamColour& colours;
        const CourseData& course;
    };

    // The authored course and one actual Player in a Scene, stepped in the
    // order of Level::update_level_logic, including Level's swept projectile
    // resolution and weapon update. No renderer or audio device is created.
    struct Run
    {
        struct Event
        {
            MovementFeedbackKind kind;
            Vector2F position;
            int tick;
        };

        struct Landing
        {
            int tick;
            std::string surface;
        };

        const Services& services;
        Scene scene{ nullptr, nullptr };
        Player* player = nullptr;
        std::vector<std::pair<const Surface*, CollisionObject*>> objects;
        std::vector<CollisionObject*> targets;
        std::vector<Event> events;
        std::vector<Landing> landings;
        int tick_count = 0;
        int shots = 0;
        bool grounded = false;
        // Only kill() may retire the player; any other death fails the tick.
        bool death_expected = false;
        // When set, paint area that rises while flying projectiles sweep
        // (Projectile::resolve_movement_contacts) accumulates in swept_paint,
        // apart from launch-obstruction paint later in the same tick.
        std::function<float()> watched_paint;
        float swept_paint = 0.0f;

        Run(const Services& with, const Vector2F& top_left,
            const Vector2F& velocity = Vector2F::ZERO,
            WeaponType weapon = WeaponType::sprayer, PlayerTeam team = PlayerTeam::a,
            const std::optional<Vector2F>& respawn = std::nullopt) : services(with)
        {
            const CourseData& course = with.course;
            this->scene.set_bounds(course.info.out_of_bounds_rectangle());
            auto built = with.builder.build_collision_objects(
                course.info.collision_objects_json(), with.colours);
            require(built.size() == course.surfaces.size(),
                "the builder changed the authored object count");
            for (size_t index = 0; index < built.size(); ++index)
            {
                const Surface& surface = course.surfaces[index];
                CollisionObject* object = this->scene.add(std::move(built[index]));
                if (!(object->bounds() == surface.rectangle) ||
                    (to_collision_type(object->tag()) ==
                        CollisionObjectType::structure_jump_through) != surface.jump_through)
                {
                    fail("built object '" + surface.name + "' differs from its authored entry");
                }
                this->objects.emplace_back(&surface, object);
            }
            const GameTuning& tuning = with.tuning;
            const RectangleF body(top_left, tuning.player().size);
            auto primary = WeaponBuilder::build_weapon(&tuning, team, 0, 0, Colour::white,
                weapon, body.center(), &with.render, &with.audio, &with.preferences);
            this->player = this->scene.add(std::make_unique<Player>(body, &tuning,
                &with.feedback, &with.combat, &with.redraw, &with.render, &with.audio, &with.preferences, 0, team,
                weapon, std::move(primary), WeaponType::none, Colour::white, Viewport{},
                respawn.value_or(top_left), velocity));
            this->scene.end_tick();
            for (CollisionObject* object : this->scene.collision_objects())
            {
                if (!is_projectile(to_collision_type(object->tag())))
                {
                    this->targets.push_back(object);
                }
            }
        }

        void tick(const PlayerInputData& actions = input())
        {
            ++this->tick_count;
            this->player->set_player_input(actions);
            this->scene.update(dt);
            const float before_sweep = this->watched_paint ? this->watched_paint() : 0.0f;
            for (CollisionObject* object : this->scene.collision_objects())
            {
                if (is_projectile(to_collision_type(object->tag())))
                {
                    dynamic_cast<Projectile&>(*object).resolve_movement_contacts(this->targets);
                }
            }
            if (this->watched_paint) { this->swept_paint += this->watched_paint() - before_sweep; }
            for (std::unique_ptr<Projectile>& shot :
                this->player->update_weapon_and_get_projectiles(dt, this->targets))
            {
                ++this->shots;
                this->scene.add(std::move(shot));
            }
            this->scene.resolve();
            this->player->end_contacts();
            this->player->update_weapon_position();
            this->player->update_prev_rectangle();
            this->scene.end_tick();

            require(this->scene.in_bounds(*this->player),
                "the player left the authored course bounds");
            require(this->death_expected || this->player->state() == PlayerState::alive,
                "the player died without the damage fixture");
            for (const auto& effect : this->player->movement_feedback().effects())
            {
                if (effect.age == 0.0f)
                {
                    this->events.push_back({ effect.kind, effect.position, this->tick_count });
                }
            }
            const RectangleF& body = this->player->rectangle();
            for (const auto& [surface, object] : this->objects)
            {
                const RectangleF& solid = surface->rectangle;
                if (surface->jump_through) { continue; }
                const float width = std::min(body.right(), solid.right()) -
                    std::max(body.left(), solid.left());
                const float height = std::min(body.bottom(), solid.bottom()) -
                    std::max(body.top(), solid.top());
                if (width > overlap_tolerance && height > overlap_tolerance)
                {
                    fail("the player intersects '" + surface->name + "' at tick " +
                        std::to_string(this->tick_count) + " (" + fixed(width, 3) + " x " +
                        fixed(height, 3) + ")");
                }
            }
            const bool now_grounded = supported(*this->player);
            if (now_grounded && !this->grounded)
            {
                this->landings.push_back({ this->tick_count, this->support() });
            }
            this->grounded = now_grounded;
        }

        // Admit a constructed or respawned player onto its support, then
        // forget that settling contact so trials count only their own events.
        void settle(int ticks = 1, const PlayerInputData& actions = input())
        {
            for (int tick = 0; tick < ticks; ++tick) { this->tick(actions); }
            this->landings.clear();
            this->events.clear();
        }

        size_t count(MovementFeedbackKind kind) const
        {
            return static_cast<size_t>(std::count_if(this->events.begin(), this->events.end(),
                [kind](const Event& event) { return event.kind == kind; }));
        }

        // The authored surface under the feet, or "air".
        std::string support() const
        {
            const RectangleF& body = this->player->rectangle();
            for (const auto& [surface, object] : this->objects)
            {
                const RectangleF& top = surface->rectangle;
                if (std::fabs(top.top() - body.bottom()) < surface_tolerance &&
                    body.right() > top.left() && body.left() < top.right())
                {
                    return surface->name;
                }
            }
            return "air";
        }

        float front(float direction) const
        {
            return direction > 0.0f ? this->player->rectangle().right() :
                this->player->rectangle().left();
        }

        StructurePaintable& paintable(const std::string& name) const
        {
            for (const auto& [surface, object] : this->objects)
            {
                if (surface->name == name)
                {
                    StructurePaintable* result = dynamic_cast<StructurePaintable*>(object);
                    if (result == nullptr) { fail("'" + name + "' is not paintable"); }
                    return *result;
                }
            }
            fail("'" + name + "' was not built");
        }

        // Lethal contact through the player's real damage path, as movement.cpp.
        void kill()
        {
            this->death_expected = true;
            const PlayerTeam enemy = this->player->team() == PlayerTeam::a ?
                PlayerTeam::b : PlayerTeam::a;
            const ProjectileSpray shot(RectangleF(0, 0, 10, 10), Vector2F::ZERO, enemy, 1,
                Colour::white, &this->services.render,
                &this->services.tuning.projectile(ProjectileType::spray));
            for (int hit = 0; hit < 1000 && this->player->health() > 0.0f; ++hit)
            {
                this->player->on_contact(shot, Vector2F::ZERO, 0);
            }
            require(this->player->health() <= 0.0f, "damage fixture did not eliminate the player");
        }
    };

    Vector2F standing_on(const Services& s, const Surface& surface, float centre_x);
    Vector2F front_at(const Services& s, const Surface& surface, float front, float direction);
    void walk_to(Run& run, float target);
    void brake(Run& run);
    std::string jump_and_land(Run& run, float movement,
        const std::optional<float>& lip = std::nullopt);

    struct Gap
    {
        const char* label;
        const Surface* left;
        const Surface* right;
        const Surface* low;
        const Surface* high;

        float width() const { return this->right->rectangle.left() - this->left->rectangle.right(); }
    };

    // The trials, in the order main runs them.
    void from_rest_travel(const Services& s);
    void analog_movement(const Services& s);
    void gaps(const Services& s, const std::vector<Gap>& routes);
    void recovery(const Services& s, const std::vector<Gap>& routes);
    void staircases(const Services& s);
    void ceiling(const Services& s);
    void drop_through(const Services& s);
    void lip_timing(const Services& s);
    void spawns(const Services& s);
    void aim(const Services& s);
    void painting(const Services& s);
}
