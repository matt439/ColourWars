#include "game/pch.h"
#include "game/play_settings.h"
#include "game/tuning.h"
#include "game/objects/paintable_object.h"
#include "game/objects/structure.h"
#include "game/objects/paint_tile.h"
#include "game/objects/team_colour.h"
#include "engine/audio/audio_resources.h"
#include "engine/audio/sound_bank.h"
#include "engine/render/colour.h"

// Attribute paint to one authored face through the tiles the surface scores.
// Load its dependencies first so this does not change any other declaration.
#define private public
#include "game/objects/structure_paintable.h"
#undef private

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

// Route trials on the authored Movement Course geometry. Every trial builds a
// fresh Scene from movement_course.json through LevelObjectBuilder and places
// the real Player by construction or respawn; nothing is teleported. MEASURE
// lines record candidate tuning; assertions cover only intended outcomes.

using namespace labrador;
using namespace mattmath;

namespace
{
    void require(bool condition, const char* message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    [[noreturn]] void fail(const std::string& message)
    {
        throw std::runtime_error(message);
    }

    const float dt = static_cast<float>(StepTimer::TicksToSeconds(
        StepTimer::SecondsToTicks(1.0 / 60.0)));

    // Contact separation is exact; these allow only float noise.
    constexpr float overlap_tolerance = 0.01f;
    constexpr float surface_tolerance = 0.05f;

    std::string fixed(float value, int precision = 1)
    {
        std::ostringstream stream;
        stream << std::fixed << std::setprecision(precision) << value;
        return stream.str();
    }

    PlayerInputData input(bool pressed = false, bool held = false,
        float movement = 0.0f, bool drop = false)
    {
        PlayerInputData result;
        result.connected = true;
        result.jump_pressed = pressed;
        result.jump_held = held;
        result.x_movement = movement;
        result.drop_through = drop;
        return result;
    }

    PlayerInputData aimed(float angle, PlayerInputData actions = input())
    {
        actions.shoot_direction_requested = true;
        actions.shoot_angle = angle;
        actions.shoot_direction = Vector2F::unit_vec_from_angle(angle);
        return actions;
    }

    // COPIED from PlayerInput::pad_input (game/objects/player_input.cpp). It
    // reads the engine's concrete XInput Gamepads, which has no seam for raw
    // stick values, so these three mappings must be kept in step by hand.
    float pad_movement(float raw_x, const GameTuning& tuning)
    {
        return apply_deadzone(raw_x, tuning.input().movement_deadzone);
    }

    bool pad_drop(float raw_y, const GameTuning& tuning)
    {
        return raw_y > tuning.player().drop_down_analog_threshold;
    }

    PlayerInputData pad_aim(const Vector2F& raw, const PlaySettings& preferences,
        PlayerInputData actions = input())
    {
        const Vector2F aim = apply_deadzone(raw, preferences.aim_deadzone());
        actions.shoot_direction_requested = aim != Vector2F::ZERO;
        if (actions.shoot_direction_requested)
        {
            actions.shoot_direction = aim.normalized();
            actions.shoot_angle = actions.shoot_direction.angle();
        }
        return actions;
    }

    // Player::update_movement counts the jump buffer and ledge grace down once
    // per update. True when a window opened on one update is still positive
    // after `elapsed` further updates, using the player's own arithmetic.
    bool window_open(float window, int elapsed)
    {
        float remaining = window;
        for (int tick = 0; tick < elapsed; ++tick)
        {
            remaining = std::max(0.0f, remaining - dt);
        }
        return remaining > 0.0f;
    }

    bool supported(const Player& player)
    {
        return player.move_state() == PlayerMoveState::on_ground ||
            player.move_state() == PlayerMoveState::on_drop_down_ground;
    }

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

    Vector2F standing_on(const Services& s, const Surface& surface, float centre_x)
    {
        const Vector2F size = s.tuning.player().size;
        return { centre_x - size.x / 2.0f, surface.rectangle.top() - size.y };
    }

    // The top-left that puts the leading edge, in `direction`, at `front`.
    Vector2F front_at(const Services& s, const Surface& surface, float front, float direction)
    {
        const Vector2F size = s.tuning.player().size;
        return { direction > 0.0f ? front - size.x : front, surface.rectangle.top() - size.y };
    }

    // Walk without jumping, braking with the player's own deceleration.
    void walk_to(Run& run, float target)
    {
        const PlayerTuning& tuning = run.services.tuning.player();
        for (int tick = 0; tick < 1200; ++tick)
        {
            const float error = target - run.player->center().x;
            const float speed = run.player->velocity().x;
            if (std::fabs(error) < 6.0f && speed == 0.0f) { return; }
            const float stopping = speed * speed / (2.0f * tuning.x_deacceleration) +
                std::fabs(speed) * dt + 4.0f;
            const bool toward = speed == 0.0f || (speed > 0.0f) == (error > 0.0f);
            run.tick(input(false, false, toward && std::fabs(error) > stopping ?
                (error > 0.0f ? 1.0f : -1.0f) : 0.0f));
        }
        fail("the player never settled at x=" + fixed(target));
    }

    void brake(Run& run)
    {
        for (int tick = 0; tick < 240; ++tick)
        {
            if (supported(*run.player) && run.player->velocity().x == 0.0f) { return; }
            run.tick();
        }
        fail("the player never came to rest");
    }

    // Release, press and hold one jump until the next landing. With a lip,
    // stop steering once the back edge is 20 units past it.
    std::string jump_and_land(Run& run, float movement,
        const std::optional<float>& lip = std::nullopt)
    {
        run.tick(input());
        const size_t landings = run.landings.size();
        run.tick(input(true, true, movement));
        require(run.player->move_state() == PlayerMoveState::jumping,
            "a grounded press did not take off");
        for (int tick = 0; tick < 240 && run.landings.size() == landings; ++tick)
        {
            const float back = movement > 0.0f ? run.player->rectangle().left() :
                run.player->rectangle().right();
            if (lip.has_value() && movement * (back - *lip) > 20.0f) { movement = 0.0f; }
            run.tick(input(false, true, movement));
        }
        require(run.landings.size() > landings, "a held jump never landed");
        return run.landings.back().surface;
    }

    void from_rest_travel(const Services& s)
    {
        const PlayerTuning& tuning = s.tuning.player();
        const CourseData& course = s.course;
        struct Lane
        {
            const char* label;
            Vector2F spawn;
            PlayerTeam team;
            const Surface* runway;
            float direction;
        };
        for (const Lane& lane : {
            Lane{ "team_a spawn 1 toward the short gap", course.info.team_a_spawns().front(),
                PlayerTeam::a, &course.named("spawn_a_runway"), 1.0f },
            Lane{ "team_b spawn 4 toward the return step", course.info.team_b_spawns().back(),
                PlayerTeam::b, &course.named("spawn_b_runway"), -1.0f } })
        {
            const float d = lane.direction;
            Run run(s, lane.spawn, Vector2F::ZERO, WeaponType::sprayer, lane.team, lane.spawn);
            run.settle(30);
            require(run.support() == lane.runway->name, "an authored spawn did not settle on its runway");
            const float end = d > 0.0f ? lane.runway->rectangle.right() : lane.runway->rectangle.left();
            const float start = run.player->center().x;
            const float start_margin = d * (end - run.front(d));
            int ticks_to_max = -1;
            float distance_to_max = 0.0f;
            float margin_at_max = 0.0f;
            float previous = 0.0f;
            for (int tick = 1; tick <= 240 && ticks_to_max < 0 && d * (end - run.front(d)) > 0.0f; ++tick)
            {
                run.tick(input(false, false, d));
                const float speed = d * run.player->velocity().x;
                require(speed >= previous && speed <= tuning.max_velocity.x,
                    "held movement from rest decelerated or exceeded its cap");
                previous = speed;
                if (speed == tuning.max_velocity.x)
                {
                    ticks_to_max = tick;
                    distance_to_max = d * (run.player->center().x - start);
                    margin_at_max = d * (end - run.front(d));
                }
            }
            const float released = run.player->center().x;
            int stop_ticks = 0;
            for (; run.player->velocity().x != 0.0f && stop_ticks < 120; ++stop_ticks) { run.tick(); }
            require(run.player->velocity().x == 0.0f, "releasing movement did not stop the player");
            for (int tick = 0; tick < 10; ++tick)
            {
                run.tick();
                require(run.player->velocity().x == 0.0f, "a released stop crept afterwards");
            }
            std::cout << "MEASURE from-rest " << lane.label << ": runway_ahead=" << fixed(start_margin)
                << " ticks_to_max=" << ticks_to_max << " (" << fixed(static_cast<float>(ticks_to_max) * dt, 3)
                << " s) distance_to_max=" << fixed(distance_to_max)
                << " runway_left_at_max=" << fixed(margin_at_max)
                << " | release: ticks_to_stop=" << stop_ticks
                << " stop_distance=" << fixed(d * (run.player->center().x - released))
                << " runway_left_at_stop=" << fixed(d * (end - run.front(d)))
                << " support=" << run.support() << '\n';
        }
    }

    void analog_movement(const Services& s)
    {
        const Surface& lane = s.course.named("recovery_floor");
        const float max_speed = s.tuning.player().max_velocity.x;
        std::cout << "NOTE analog magnitudes use a COPY of PlayerInput::pad_input's mapping: "
            "apply_deadzone(raw, stick_deadzone=" << s.tuning.input().movement_deadzone
            << "), drop when raw y > drop_down_analog_threshold.\n";
        std::vector<std::vector<float>> traces;
        const std::vector<float> raws{ 0.45f, 0.55f, 0.65f, 0.75f, 0.9f, 1.0f };
        for (const float raw : raws)
        {
            const float movement = pad_movement(raw, s.tuning);
            Run run(s, standing_on(s, lane, lane.rectangle.left() + 100.0f));
            run.settle();
            const float start = run.player->center().x;
            std::vector<float> trace;
            int ticks_to_max = -1;
            float distance_to_max = 0.0f;
            for (int tick = 1; tick <= 900; ++tick)
            {
                run.tick(input(false, false, movement));
                trace.push_back(run.player->velocity().x);
                if (ticks_to_max < 0 && run.player->velocity().x == max_speed)
                {
                    ticks_to_max = tick;
                    distance_to_max = run.player->center().x - start;
                }
                if ((ticks_to_max >= 0 && tick >= 60) ||
                    run.front(1.0f) > lane.rectangle.right() - 100.0f) { break; }
            }
            traces.push_back(trace);
            if (movement == 0.0f)
            {
                require(std::all_of(trace.begin(), trace.end(), [](float v) { return v == 0.0f; }),
                    "stick travel inside the deadzone moved the player");
            }
            const auto at = [&trace](size_t tick) { return tick <= trace.size() ? trace[tick - 1] : 0.0f; };
            std::cout << "MEASURE analog raw=" << fixed(raw, 2) << " x_movement=" << fixed(movement, 2)
                << " speed@10=" << fixed(at(10)) << " speed@30=" << fixed(at(30))
                << " speed@60=" << fixed(at(60)) << " ticks_to_max=" << ticks_to_max
                << " distance_to_max=" << fixed(distance_to_max) << '\n';
        }
        for (size_t index = 0; index + 1 < traces.size(); ++index)
        {
            const size_t length = std::min(traces[index].size(), traces[index + 1].size());
            for (size_t tick = 0; tick < length; ++tick)
            {
                require(traces[index][tick] <= traces[index + 1][tick] + 0.001f,
                    "a larger stick magnitude was slower than a smaller one");
            }
        }

        for (const float d : { 1.0f, -1.0f })
        {
            int release_ticks = 0;
            float release_distance = 0.0f;
            int previous_ticks = 300;
            float previous_distance = std::numeric_limits<float>::max();
            for (const float raw : { 0.0f, 0.55f, 0.65f, 0.75f, 0.9f, 1.0f })
            {
                Run run(s, standing_on(s, lane, lane.rectangle.center().x), { d * max_speed, 0.0f });
                run.settle(1, input(false, false, d));
                require(d * run.player->velocity().x == max_speed,
                    "braking fixture did not start at full speed");
                const float movement = -d * pad_movement(raw, s.tuning);
                const float start = run.player->center().x;
                float previous_speed = max_speed;
                int ticks = 0;
                for (; d * run.player->velocity().x > 0.0f && ticks < 300; ++ticks)
                {
                    run.tick(input(false, false, movement));
                    const float speed = d * run.player->velocity().x;
                    require(speed <= previous_speed && std::fabs(speed) <= max_speed,
                        "braking accelerated forward or exceeded the speed cap");
                    previous_speed = speed;
                }
                require(d * run.player->velocity().x <= 0.0f, "braking never reached zero speed");
                const float after = run.player->velocity().x;
                const float distance = d * (run.player->center().x - start);
                require(ticks <= previous_ticks && distance <= previous_distance + 0.01f,
                    "a stronger opposite stick took longer or farther to stop");
                previous_ticks = ticks;
                previous_distance = distance;
                if (movement == 0.0f)
                {
                    release_ticks = ticks;
                    release_distance = distance;
                    for (int tick = 0; tick < 10; ++tick)
                    {
                        run.tick();
                        require(run.player->velocity().x == 0.0f, "a released stick did not hold a stop");
                    }
                }
                else
                {
                    require(ticks <= release_ticks && distance <= release_distance + 0.01f,
                        "opposite stick braking was weaker than release braking");
                    require(std::fabs(after) <= s.tuning.player().x_acceleration *
                        std::fabs(movement) * dt + 0.001f,
                        "braking propelled the player past the available reverse acceleration");
                    float reverse_speed = -d * after;
                    const float crossing_speed = reverse_speed;
                    for (int tick = 0; tick < 90; ++tick)
                    {
                        run.tick(input(false, false, movement));
                        const float speed = -d * run.player->velocity().x;
                        require(speed >= reverse_speed && speed <= max_speed,
                            "held reversal oscillated, lost reverse speed or exceeded its cap");
                        reverse_speed = speed;
                    }
                    require(reverse_speed > crossing_speed,
                        "held opposite input did not build speed in the new direction");
                }
                std::cout << "MEASURE brake-from-800 dir=" << (d > 0.0f ? "right" : "left")
                    << ' ' << (movement == 0.0f ? "release" : "reverse")
                    << " raw=" << fixed(-d * raw, 2) << " x_movement=" << fixed(movement, 2)
                    << " ticks_to_zero=" << ticks << " distance=" << fixed(distance)
                    << " velocity_on_crossing=" << fixed(after) << '\n';
            }
        }

        const Surface& platform = s.course.named("drop_through_platform");
        const float threshold = s.tuning.player().drop_down_analog_threshold;
        for (const float raw : { threshold - 0.01f, threshold + 0.01f })
        {
            Run run(s, standing_on(s, platform, platform.rectangle.center().x));
            run.settle(2);
            require(run.player->move_state() == PlayerMoveState::on_drop_down_ground,
                "drop fixture did not stand on the authored platform");
            const bool drop = pad_drop(raw, s.tuning);
            run.tick(input(false, false, 0.0f, drop));
            const bool dropped = run.player->move_state() == PlayerMoveState::dropping_down;
            for (int tick = 0; tick < 90 && run.landings.empty(); ++tick)
            {
                run.tick(input(false, false, 0.0f, drop));
            }
            require(dropped == (raw > threshold), "the drop stick threshold was not respected");
            if (dropped)
            {
                require(!run.landings.empty() && run.landings.front().surface == "spawn_a_runway" &&
                    run.count(MovementFeedbackKind::takeoff) == 0,
                    "an analog drop did not land on the runway without a jump");
            }
            else
            {
                require(run.support() == platform.name, "a stick below the threshold dropped");
            }
            std::cout << "MEASURE analog-drop raw_y=" << fixed(raw, 2) << " threshold=" << fixed(threshold, 2)
                << " dropped=" << dropped << " support=" << run.support() << '\n';
        }
        std::cout << "PASS analog magnitudes: deadzone, monotonic speed, release stop, stronger opposite braking "
            "and bounded reversal both ways, drop threshold\n";
    }

    struct Gap
    {
        const char* label;
        const Surface* left;
        const Surface* right;
        const Surface* low;
        const Surface* high;

        float width() const { return this->right->rectangle.left() - this->left->rectangle.right(); }
    };

    enum class Takeoff { rest, running, grace };

    struct Start
    {
        Takeoff kind;
        float value; // run-up distance before the lip, or ticks after leaving it
        const char* label;
    };

    struct GapResult
    {
        std::string first;
        std::string last;
        size_t takeoffs = 0;
        float rise = 0.0f;
        int airtime = 0;
        float landed_past_lip = 0.0f;
        float stopped_past_lip = 0.0f;
    };

    // Hold direction until the back edge is 20 past the far lip, then brake in
    // the air; braking on landing otherwise. hold==0 holds jump to landing.
    GapResult jump_gap(const Services& s, const Gap& gap, float d, const Start& start, int hold)
    {
        const float speed = s.tuning.player().max_velocity.x;
        const Surface& from = d > 0.0f ? *gap.left : *gap.right;
        const Surface& to = d > 0.0f ? *gap.right : *gap.left;
        const float lip = d > 0.0f ? from.rectangle.right() : from.rectangle.left();
        const float far_lip = d > 0.0f ? to.rectangle.left() : to.rectangle.right();
        const bool moving = start.kind != Takeoff::rest;
        const float run_up = start.kind == Takeoff::running ? start.value :
            (start.kind == Takeoff::grace ? 100.0f : 0.0f);
        // The settling tick advances a running player by one step.
        Run run(s, front_at(s, from, lip - d * (run_up + (moving ? speed * dt : 0.0f)), d),
            { moving ? d * speed : 0.0f, 0.0f });
        run.settle(1, input(false, false, moving ? d : 0.0f));
        require(run.support() == from.name, "a gap trial did not start on its takeoff platform");
        const float start_feet = run.player->rectangle().bottom();
        float highest = start_feet;
        int press = start.kind == Takeoff::grace ? -1 : 0;
        int landed = -1;
        bool steering = true;
        GapResult result;
        for (int tick = 0; tick < 600; ++tick)
        {
            if (press < 0 && run.player->move_state() == PlayerMoveState::in_air)
            {
                press = tick + static_cast<int>(start.value) - 1;
            }
            const RectangleF& body = run.player->rectangle();
            if (landed >= 0 || (d > 0.0f ? body.left() > far_lip + 20.0f : body.right() < far_lip - 20.0f))
            {
                steering = false;
            }
            const bool started = press >= 0 && tick >= press;
            const bool holding = started && (hold == 0 ? landed < 0 : tick < press + hold);
            run.tick(input(tick == press, holding, steering ? d : 0.0f));
            highest = std::min(highest, run.player->rectangle().bottom());
            if (landed < 0 && started && !run.landings.empty())
            {
                landed = tick;
                result.first = run.landings.front().surface;
                result.airtime = tick - press;
                result.landed_past_lip = d * (run.player->center().x - far_lip);
            }
            if (landed >= 0 && supported(*run.player) && run.player->velocity().x == 0.0f) { break; }
        }
        require(landed >= 0, "a gap trial never landed");
        result.last = run.support();
        result.takeoffs = run.count(MovementFeedbackKind::takeoff);
        result.rise = start_feet - highest;
        result.stopped_past_lip = d * (run.player->center().x - far_lip);
        return result;
    }

    void gaps(const Services& s, const std::vector<Gap>& routes)
    {
        const std::vector<Start> starts{ { Takeoff::rest, 0.0f, "rest-at-lip" },
            { Takeoff::running, 120.0f, "run800-lip-120" }, { Takeoff::running, 70.0f, "run800-lip-70" },
            { Takeoff::running, 18.0f, "run800-lip-18" }, { Takeoff::grace, 1.0f, "run800-grace+1" } };
        const std::vector<int> holds{ 1, 6, 12, 21, 30, 0 };
        std::cout << "TABLE gap outcomes (a landing back on the takeoff platform fails): F far platform, H/L recovery step "
            "high/low, G lower floor (Y=1400), D drop-through platform. Columns are jump-held ticks; 'held' holds to landing.\n";
        for (const Gap& gap : routes)
        {
            for (const float d : { 1.0f, -1.0f })
            {
                const Surface& from = d > 0.0f ? *gap.left : *gap.right;
                const Surface& to = d > 0.0f ? *gap.right : *gap.left;
                for (const Start& start : starts)
                {
                    std::string row;
                    for (const int hold : holds)
                    {
                        const GapResult result = jump_gap(s, gap, d, start, hold);
                        std::string code;
                        if (result.first == to.name) { code = "F"; }
                        else if (result.first == from.name)
                        {
                            fail(std::string("a ") + gap.label + " jump fell back onto its takeoff platform");
                        }
                        else if (result.first == gap.high->name) { code = "H"; }
                        else if (result.first == gap.low->name) { code = "L"; }
                        else if (result.first == "recovery_floor") { code = "G"; }
                        else if (result.first == "drop_through_platform") { code = "D"; }
                        else { fail(std::string("a missed ") + gap.label + " jump landed on '" + result.first + "'"); }
                        require(result.takeoffs == 1, "a gap trial did not take off exactly once");
                        if (gap.left->name == "spawn_a_runway")
                        {
                            // Holding a leftward jump reaches the platform above
                            // the runway; shorter presses choose the runway itself.
                            const bool upper_landing = d < 0.0f && (hold == 0 || hold >= 21);
                            require(result.first == (upper_landing ? "drop_through_platform" : to.name),
                                "a short-gap jump did not reach its chosen runway or drop-through landing");
                        }
                        else
                        {
                            require(code != "D", "a different gap reached the short-gap drop-through platform");
                        }
                        row += " h" + (hold == 0 ? std::string("eld") : std::to_string(hold)) + "=" + code;
                        if (hold == 0 && start.kind == Takeoff::running)
                        {
                            std::cout << "MEASURE gap=" << gap.label << " width=" << fixed(gap.width(), 0)
                                << " dir=" << (d > 0.0f ? "right" : "left") << " start=" << start.label
                                << " held: landed=" << result.first << " rise=" << fixed(result.rise)
                                << " airtime_ticks=" << result.airtime
                                << " landed_past_far_lip=" << fixed(result.landed_past_lip)
                                << " stopped_past_far_lip=" << fixed(result.stopped_past_lip)
                                << " stopped_on=" << result.last << '\n';
                        }
                        if (hold == 0 && start.kind == Takeoff::running)
                        {
                            // The scratch probe's case, from authored geometry without
                            // teleports, from every running lip: across the gap, at or
                            // above the far platform. Leftward over the short gap that
                            // can be the drop platform.
                            const float far_lip = d > 0.0f ? to.rectangle.left() : to.rectangle.right();
                            for (const std::string& name : { result.first, result.last })
                            {
                                const RectangleF& landing = s.course.named(name).rectangle;
                                require((d > 0.0f ? landing.left() >= far_lip : landing.right() <= far_lip) &&
                                    landing.top() <= to.rectangle.top(),
                                    "a held full-speed jump did not land and brake beyond the far lip");
                            }
                        }
                    }
                    std::cout << "MEASURE gap=" << gap.label << " width=" << fixed(gap.width(), 0)
                        << " dir=" << (d > 0.0f ? "right" : "left") << " start=" << start.label
                        << " |" << row << '\n';
                }
            }
        }
        std::cout << "PASS gaps: every trial takes off exactly once; held full-speed jumps from every "
            "running lip land and brake beyond the far lip both ways; none falls back onto its takeoff "
            "platform, and every other landing is a recovery step, the lower floor or the drop-through platform\n";
    }

    // Floor to low step, low to high step, high step sideways to a main platform.
    std::vector<std::string> climb_pair(const Services& s, const Gap& gap, float start_centre,
        float exit, bool steer_throughout = false)
    {
        const Surface& floor = s.course.named("recovery_floor");
        Run run(s, standing_on(s, floor, start_centre));
        run.settle();
        require(run.support() == floor.name, "recovery trial did not start on the lower floor");
        walk_to(run, gap.low->rectangle.center().x);
        require(run.count(MovementFeedbackKind::takeoff) == 0 && run.landings.empty(),
            "the lower lane needed a jump to reach a recovery pair");
        std::vector<std::string> path;
        path.push_back(jump_and_land(run, 0.0f));
        path.push_back(jump_and_land(run, 0.0f));
        const Surface& platform = exit > 0.0f ? *gap.right : *gap.left;
        path.push_back(jump_and_land(run, exit, steer_throughout ? std::nullopt :
            std::optional<float>(exit > 0.0f ? platform.rectangle.left() : platform.rectangle.right())));
        brake(run);
        path.push_back(run.support());
        return path;
    }

    void recovery(const Services& s, const std::vector<Gap>& routes)
    {
        for (const Gap& gap : routes)
        {
            const float centre = gap.low->rectangle.center().x;
            const float offset = gap.low->rectangle.width / 2.0f + 150.0f;
            for (const float approach : { 1.0f, -1.0f })
            {
                for (const float exit : { 1.0f, -1.0f })
                {
                    const auto path = climb_pair(s, gap, centre - approach * offset, exit);
                    const std::string& platform = exit > 0.0f ? gap.right->name : gap.left->name;
                    if (path[0] != gap.low->name || path[1] != gap.high->name ||
                        path[2] != platform || path[3] != platform)
                    {
                        fail(std::string("the ") + gap.label + " recovery pair climbed " + path[0] + " -> " +
                            path[1] + " -> " + path[2] + " -> " + path[3] + ", not low, high, " + platform);
                    }
                    // Continuous steering can carry through the short right
                    // landing. Releasing early above still makes every exit safe.
                    const auto steered = climb_pair(s, gap, centre - approach * offset, exit, true);
                    const bool short_slide = gap.left->name == "spawn_a_runway" && exit > 0.0f;
                    require(steered[0] == gap.low->name && steered[1] == gap.high->name &&
                        steered[2] == platform && steered[3] == (short_slide ? "recovery_floor" : platform),
                        "a continuously steered recovery exit did not reach its accepted landing and support");
                    std::cout << "MEASURE recovery " << gap.label << " approach="
                        << (approach > 0.0f ? "from-left" : "from-right") << " exit="
                        << (exit > 0.0f ? "right" : "left") << ": " << path[0] << " -> " << path[1]
                        << " -> " << path[2] << " (stopped on " << path[3]
                        << ") | steering throughout the exit jump stops on " << steered[3] << '\n';
                }
            }
        }
        // The rescue lane is recovery_floor plus the recovery pairs. From
        // beneath the staircase and the far right, the floor leads back to the
        // running-gap pair without any jump.
        const Gap& running = routes.back();
        for (const float start : { s.course.named("high_step").rectangle.center().x,
            s.course.named("spawn_b_runway").rectangle.right() - 60.0f })
        {
            for (const float exit : { 1.0f, -1.0f })
            {
                const auto path = climb_pair(s, running, start, exit);
                const std::string& platform = exit > 0.0f ? running.right->name : running.left->name;
                require(path[2] == platform && path[3] == platform,
                    "the rescue lane did not return to the course");
                std::cout << "MEASURE rescue-lane from x=" << fixed(start, 0) << " -> " << path[0]
                    << " -> " << path[1] << " -> " << path[2] << '\n';
            }
        }
        std::cout << "PASS recovery: every pair from rest, both approaches and exits; rescue lane from the staircase underside\n";
    }

    struct Rise
    {
        const Surface* lower;
        const Surface* upper;
        float direction;

        float height() const { return this->lower->rectangle.top() - this->upper->rectangle.top(); }
        float face() const
        {
            return this->direction > 0.0f ? this->upper->rectangle.left() : this->upper->rectangle.right();
        }
    };

    // Press within 30 units of the face, holding direction until landing.
    std::string climb_rise(const Services& s, const Rise& rise, bool running, bool held)
    {
        const float d = rise.direction;
        const float speed = s.tuning.player().max_velocity.x;
        const float room = rise.lower->rectangle.width - s.tuning.player().size.x - 8.0f - speed * dt;
        const float distance = running ? std::min(300.0f, room) : 4.0f;
        Run run(s, front_at(s, *rise.lower, rise.face() - d * (distance + (running ? speed * dt : 0.0f)), d),
            { running ? d * speed : 0.0f, 0.0f });
        run.settle(1, input(false, false, running ? d : 0.0f));
        require(run.support() == rise.lower->name, "a staircase trial did not start on its lower step");
        int press = -1;
        for (int tick = 0; tick < 400; ++tick)
        {
            if (press < 0 && d * (rise.face() - run.front(d)) <= 30.0f) { press = tick; }
            const bool holding = press >= 0 && (held ? run.landings.empty() : tick == press);
            run.tick(input(tick == press, holding, run.landings.empty() ? d : 0.0f));
            if (!run.landings.empty() && supported(*run.player) && run.player->velocity().x == 0.0f) { break; }
        }
        require(press >= 0 && !run.landings.empty(), "a staircase jump never landed");
        return run.landings.front().surface;
    }

    // Hold direction throughout and jump with a held press within 30 units of
    // each next face. Records the path; the per-rise trials make the claims.
    std::vector<std::string> run_staircase(const Services& s, const std::vector<const Surface*>& route,
        float d, bool running)
    {
        const float speed = s.tuning.player().max_velocity.x;
        const auto face = [d](const Surface& surface)
            { return d > 0.0f ? surface.rectangle.left() : surface.rectangle.right(); };
        const float distance = running ? 300.0f : 4.0f;
        Run run(s, front_at(s, *route[0], face(*route[1]) - d * (distance + (running ? speed * dt : 0.0f)), d),
            { running ? d * speed : 0.0f, 0.0f });
        run.settle(1, input(false, false, running ? d : 0.0f));
        std::vector<std::string> path;
        size_t reached = 0;
        bool jumping = false;
        bool released = true;
        for (int tick = 0; tick < 900; ++tick)
        {
            const size_t next = reached + 1;
            bool pressing = false;
            if (!jumping && next < route.size() && supported(*run.player) && released &&
                d * (face(*route[next]) - run.front(d)) <= 30.0f)
            {
                pressing = true;
                jumping = true;
            }
            released = !jumping;
            const size_t landings = run.landings.size();
            run.tick(input(pressing, jumping, next < route.size() ? d : 0.0f));
            if (run.landings.size() > landings)
            {
                jumping = false;
                path.push_back(run.landings.back().surface);
                for (size_t index = next; index < route.size(); ++index)
                {
                    if (route[index]->name == path.back()) { reached = index; }
                }
            }
            if (reached + 1 == route.size() && supported(*run.player) &&
                run.player->velocity().x == 0.0f) { break; }
        }
        path.push_back("stopped on " + run.support());
        return path;
    }

    void staircases(const Services& s)
    {
        const CourseData& c = s.course;
        const Surface& running = c.named("running_landing");
        const Surface& short_step = c.named("short_step");
        const Surface& held_step = c.named("held_step");
        const Surface& high_step = c.named("high_step");
        const Surface& high_return = c.named("high_step_return");
        const Surface& short_return = c.named("short_step_return");
        const Surface& runway_b = c.named("spawn_b_runway");
        const std::vector<Rise> rises{ { &running, &short_step, 1.0f }, { &short_step, &held_step, 1.0f },
            { &held_step, &high_step, 1.0f }, { &runway_b, &short_return, -1.0f },
            { &short_return, &high_return, -1.0f }, { &high_return, &high_step, -1.0f } };
        for (const Rise& rise : rises)
        {
            // The authored 60-unit rises are the short-jump stations.
            const bool tap_station = rise.upper == &short_step || rise.upper == &short_return;
            for (const bool moving : { false, true })
            {
                const std::string held = climb_rise(s, rise, moving, true);
                const std::string tap = climb_rise(s, rise, moving, false);
                std::cout << "MEASURE stair rise=" << fixed(rise.height(), 0) << ' ' << rise.lower->name
                    << " -> " << rise.upper->name << " dir=" << (rise.direction > 0.0f ? "right" : "left")
                    << " start=" << (moving ? "run800" : "rest") << ": held=" << held << " tap=" << tap << '\n';
                // Clearing means landing at or above the upper step; a held jump
                // may carry on to a higher one.
                const auto clears = [&](const std::string& name)
                    { return c.named(name).rectangle.top() <= rise.upper->rectangle.top(); };
                require(clears(held), "a held jump did not clear a staircase rise");
                require(clears(tap) == tap_station,
                    "a tap cleared a held-jump rise or failed the short step");
                if (tap_station)
                {
                    const std::string& next = rise.direction > 0.0f ? held_step.name : high_return.name;
                    require(tap == rise.upper->name && held == next,
                        "tap and held jumps did not select the short rise and the next higher step");
                }
            }
        }

        for (const bool moving : { false, true })
        {
            for (const auto& [d, route] : { std::pair{ 1.0f, std::vector<const Surface*>{
                &running, &short_step, &held_step, &high_step } }, std::pair{ -1.0f,
                std::vector<const Surface*>{ &runway_b, &short_return, &high_return, &high_step } } })
            {
                std::string text;
                for (const std::string& step : run_staircase(s, route, d, moving)) { text += " > " + step; }
                std::cout << "MEASURE staircase-ascent dir=" << (d > 0.0f ? "right" : "left") << " start="
                    << (moving ? "run800" : "rest") << ":" << text << '\n';
            }
        }

        // Descending: walk off each step from rest, then run the whole way down.
        for (const auto& [from, d] : { std::pair{ &high_step, 1.0f }, std::pair{ &high_return, 1.0f },
            std::pair{ &short_return, 1.0f }, std::pair{ &high_step, -1.0f },
            std::pair{ &held_step, -1.0f }, std::pair{ &short_step, -1.0f } })
        {
            const float edge = d > 0.0f ? from->rectangle.right() : from->rectangle.left();
            Run run(s, front_at(s, *from, edge - d * 2.0f, d));
            run.settle();
            for (int tick = 0; tick < 240 && run.landings.empty(); ++tick) { run.tick(input(false, false, d)); }
            require(!run.landings.empty() && run.count(MovementFeedbackKind::takeoff) == 0,
                "walking off a step did not land without a jump");
            const Surface& below = c.named(run.landings.front().surface);
            require(below.rectangle.top() > from->rectangle.top(), "walking off a step landed higher");
            if (from == &high_return || from == &held_step)
            {
                require(below.name == (d > 0.0f ? runway_b.name : running.name),
                    "walking off the middle step did not carry over the narrow lower step to the runway");
            }
            brake(run);
            std::cout << "MEASURE stair-descent-from-rest " << from->name << " dir="
                << (d > 0.0f ? "right" : "left") << ": landed=" << below.name
                << " drop=" << fixed(below.rectangle.top() - from->rectangle.top(), 0) << '\n';
        }
        for (const float d : { 1.0f, -1.0f })
        {
            const Surface& bottom = d > 0.0f ? runway_b : running;
            Run run(s, standing_on(s, high_step, high_step.rectangle.center().x),
                { d * s.tuning.player().max_velocity.x, 0.0f });
            run.settle(1, input(false, false, d));
            std::string text;
            for (int tick = 0; tick < 600 && (run.landings.empty() ||
                run.landings.back().surface != bottom.name); ++tick)
            {
                const size_t landings = run.landings.size();
                run.tick(input(false, false, d));
                if (run.landings.size() > landings) { text += " > " + run.landings.back().surface; }
            }
            require(!run.landings.empty() && run.landings.back().surface == bottom.name,
                "a full-speed descent did not reach the runway level");
            brake(run);
            std::cout << "MEASURE stair-descent-run800 from high_step dir=" << (d > 0.0f ? "right" : "left")
                << ":" << text << " > stopped on " << run.support() << '\n';
        }
        std::cout << "PASS staircases: held jumps clear every rise and taps only the 60-unit steps, "
            "both directions from rest and at speed; descents land without intersecting a step\n";
    }

    void ceiling(const Services& s)
    {
        const Surface& floor = s.course.named("running_landing");
        const Surface& block = s.course.named("low_ceiling");
        const float clearance = floor.rectangle.top() - block.rectangle.bottom() -
            s.tuning.player().size.y;
        const float speed = s.tuning.player().max_velocity.x;

        // Middle bonk, and the reference timing for buffered presses.
        int bonk = -1;
        int landing = -1;
        {
            Run run(s, standing_on(s, floor, block.rectangle.center().x));
            run.settle();
            const float feet = run.player->rectangle().bottom();
            float rise = 0.0f;
            for (int tick = 0; tick < 120 && landing < 0; ++tick)
            {
                run.tick(input(tick == 0, true));
                rise = std::max(rise, feet - run.player->rectangle().bottom());
                if (bonk < 0 && run.player->move_state() == PlayerMoveState::on_ceiling) { bonk = tick; }
                if (!run.landings.empty()) { landing = tick; }
            }
            require(bonk >= 0 && landing > bonk && run.landings.front().surface == floor.name &&
                run.player->velocity().y == 0.0f, "a middle bonk did not return to the floor");
            require(rise <= clearance + surface_tolerance, "a jump rose through the low ceiling");
            require(run.count(MovementFeedbackKind::takeoff) == 1 &&
                run.count(MovementFeedbackKind::landing) == 1,
                "a ceiling bonk emitted a landing or duplicated a cue");
            std::cout << "MEASURE ceiling middle bonk: clearance=" << fixed(clearance, 0) << " rise="
                << fixed(rise, 2) << " bonk_tick=" << bonk << " landing_tick=" << landing << '\n';
        }

        // A fresh press after the bonk launches only on the floor landing, and
        // only while the player's own buffer arithmetic says it is still live.
        const float buffer = s.tuning.player().jump_buffer_time;
        std::string fired;
        for (int press = bonk + 1; press <= landing; ++press)
        {
            Run run(s, standing_on(s, floor, block.rectangle.center().x));
            run.settle();
            int second = -1;
            for (int tick = 0; tick < landing + 30; ++tick)
            {
                run.tick(tick <= bonk ? input(tick == 0, true) : input(tick == press, false));
                if (second < 0 && run.count(MovementFeedbackKind::takeoff) == 2) { second = tick; }
            }
            const bool expected = window_open(buffer, landing - press);
            require(second == (expected ? landing : -1),
                "a press during a bonk jumped off the ceiling or missed its buffered floor launch");
            fired += " " + std::to_string(landing - press) + (second >= 0 ? ":launch" : ":none");
        }
        std::cout << "MEASURE ceiling buffered press (ticks before landing:result):" << fired << '\n';

        // Running passes in both directions, pressing at positions from outside
        // the near end to the far end. Brake once 20 past the far end.
        for (const float d : { 1.0f, -1.0f })
        {
            const float near_end = d > 0.0f ? block.rectangle.left() : block.rectangle.right();
            const float far_end = d > 0.0f ? block.rectangle.right() : block.rectangle.left();
            std::string row;
            for (const float press_at : { near_end - d * 60.0f, near_end + d * 20.0f,
                block.rectangle.center().x, far_end - d * 60.0f, far_end - d * 20.0f, far_end - d * 4.0f })
            {
                Run run(s, standing_on(s, floor, press_at - d * (80.0f + speed * dt)), { d * speed, 0.0f });
                run.settle(1, input(false, false, d));
                const float feet = run.player->rectangle().bottom();
                float rise = 0.0f;
                bool bonked = false;
                bool pressed = false;
                bool steering = true;
                for (int tick = 0; tick < 300; ++tick)
                {
                    const bool press = !pressed && d * (run.player->center().x - press_at) >= 0.0f;
                    pressed = pressed || press;
                    const RectangleF& body = run.player->rectangle();
                    if (!run.landings.empty() || (d > 0.0f ? body.left() > far_end + 20.0f :
                        body.right() < far_end - 20.0f)) { steering = false; }
                    run.tick(input(press, pressed && run.landings.empty(), steering ? d : 0.0f));
                    rise = std::max(rise, feet - run.player->rectangle().bottom());
                    bonked = bonked || run.player->move_state() == PlayerMoveState::on_ceiling;
                    if (!run.landings.empty() && supported(*run.player) &&
                        run.player->velocity().x == 0.0f) { break; }
                }
                require(pressed && run.count(MovementFeedbackKind::takeoff) == 1 &&
                    run.landings.size() == 1 && run.count(MovementFeedbackKind::landing) == 1,
                    "a ceiling pass took off twice or landed more than once");
                require(!bonked || rise <= clearance + surface_tolerance,
                    "a bonked jump continued through the ceiling");
                row += " press@" + fixed(d * (press_at - far_end), 0) + ":" + (bonked ? "bonk" : "clear") +
                    "/rise=" + fixed(rise, 0) + "/" + run.landings.front().surface;
            }
            std::cout << "MEASURE ceiling pass dir=" << (d > 0.0f ? "right" : "left")
                << " (press position relative to the far end):" << row << '\n';
        }
        std::cout << "PASS low ceiling: bonk returns once, buffered presses launch only from the floor, "
            "passes both ways take off once\n";
    }

    void drop_through(const Services& s)
    {
        const Surface& platform = s.course.named("drop_through_platform");
        const Surface& runway = s.course.named("spawn_a_runway");
        const float buffer = s.tuning.player().jump_buffer_time;
        const auto climb = [&](Run& run)
        {
            run.settle();
            require(jump_and_land(run, 0.0f) == platform.name &&
                run.player->move_state() == PlayerMoveState::on_drop_down_ground &&
                std::fabs(run.player->rectangle().bottom() - platform.rectangle.top()) < surface_tolerance &&
                run.count(MovementFeedbackKind::takeoff) == 1 && run.count(MovementFeedbackKind::landing) == 1,
                "a held jump did not pass up through and land once on the drop platform");
        };
        // Released runs first: its runway landing tick times the late press.
        enum class Variant { released, held, fresh_press, late_press };
        int drop_landing = -1;
        for (const Variant variant : { Variant::released, Variant::held, Variant::fresh_press,
            Variant::late_press })
        {
            Run run(s, standing_on(s, runway, platform.rectangle.center().x));
            climb(run);
            run.tick(input(false, variant == Variant::held, 0.0f, true));
            require(run.player->move_state() == PlayerMoveState::dropping_down, "drop did not leave the platform");
            const int dropped = run.tick_count;
            int press = -1;
            int landed = -1;
            for (int tick = 0; tick < 120 && landed < 0; ++tick)
            {
                PlayerInputData actions = input(false, variant == Variant::held);
                if (variant == Variant::fresh_press && tick == 0) { actions = input(true, true); }
                if (variant == Variant::late_press && run.tick_count + 1 == drop_landing - 3)
                {
                    actions = input(true, false);
                    press = run.tick_count + 1;
                }
                const size_t takeoffs = run.count(MovementFeedbackKind::takeoff);
                run.tick(actions);
                for (const Run::Event& event : run.events)
                {
                    if (event.kind == MovementFeedbackKind::landing && event.tick == run.tick_count &&
                        std::fabs(event.position.y - runway.rectangle.top()) < surface_tolerance)
                    {
                        landed = run.tick_count;
                    }
                }
                require(run.count(MovementFeedbackKind::takeoff) == takeoffs || landed == run.tick_count,
                    "a platform drop produced an airborne jump");
            }
            require(landed > 0, "a drop did not land on the runway");
            if (variant == Variant::released) { drop_landing = landed; }
            for (int tick = 0; tick < 40; ++tick) { run.tick(input(false, variant == Variant::held)); }
            const size_t takeoffs = run.count(MovementFeedbackKind::takeoff);
            if (variant == Variant::late_press)
            {
                require(press > 0 && takeoffs == (window_open(buffer, drop_landing - press) ? 2u : 1u),
                    "a deliberate press after a drop did not follow the landing buffer");
            }
            else
            {
                require(takeoffs == 1 && run.player->velocity().y == 0.0f && run.support() == runway.name,
                    "drop input became a held, grace or buffered jump");
            }
            const char* name = variant == Variant::held ? "jump held through the drop" :
                variant == Variant::fresh_press ? "fresh press the tick after the drop" :
                variant == Variant::released ? "released" : "press 3 ticks before landing";
            std::cout << "MEASURE drop-through " << name << ": fall_ticks=" << landed - dropped
                << " extra_takeoffs=" << takeoffs - 1 << '\n';
            for (int tick = 0; tick < 120 && !supported(*run.player); ++tick) { run.tick(); }
            run.tick();
            run.tick(input(true, true));
            require(run.player->move_state() == PlayerMoveState::jumping &&
                run.player->velocity().y < 0.0f, "a normal jump did not follow the drop");
        }
        std::cout << "PASS authored drop-through: climb, single landing, no held/grace/buffered drop jump, normal jump after\n";
    }

    enum class Transition { none, pause, death };

    const char* transition_name(Transition transition)
    {
        return transition == Transition::none ? "none" :
            transition == Transition::pause ? "pause" : "death+respawn";
    }

    // A player on long_landing_running_takeoff after the transition, at rest.
    // The respawn point is a test position on that platform, at the authored
    // spawns' 5-unit settle height; respawn itself is the real path.
    std::unique_ptr<Run> lip_run(const Services& s, Transition transition)
    {
        const Surface& platform = s.course.named("long_landing_running_takeoff");
        const Vector2F respawn(platform.rectangle.left() + 220.0f,
            platform.rectangle.top() - s.tuning.player().size.y - 5.0f);
        auto run = std::make_unique<Run>(s, respawn, Vector2F::ZERO, WeaponType::sprayer,
            PlayerTeam::a, respawn);
        run->settle(30);
        if (transition == Transition::pause)
        {
            run->tick(input(false, false, 1.0f));
            // Level::suspend -> pause_player_sounds calls this for every player.
            run->player->reset_jump_input();
            run->tick(input(true, true, 0.0f)); // the menu's activating press
            require(run->player->velocity().y == 0.0f, "a pause press jumped on resume");
            run->tick();
        }
        else if (transition == Transition::death)
        {
            run->tick(input(false, false, 1.0f));
            run->kill();
            run->tick(input(true, true, 1.0f, true));
            require(run->player->state() == PlayerState::dead, "fatal damage did not retire the player");
            for (int tick = 0; tick < 400 && run->player->state() == PlayerState::dead; ++tick)
            {
                run->tick(input(true, true, 1.0f, true));
            }
            require(run->player->state() == PlayerState::alive, "eliminated player did not respawn");
            for (int tick = 0; tick < 30; ++tick) { run->tick(input(false, true)); }
            require(run->support() == platform.name && run->player->velocity() == Vector2F::ZERO &&
                run->player->rectangle().x == respawn.x &&
                run->count(MovementFeedbackKind::takeoff) == 0,
                "respawn did not settle at its point without carried input");
            run->tick();
        }
        run->landings.clear();
        run->events.clear();
        return run;
    }

    int walk_off(Run& run)
    {
        for (int tick = 0; tick < 240; ++tick)
        {
            run.tick(input(false, false, 1.0f));
            if (run.player->move_state() == PlayerMoveState::in_air) { return run.tick_count; }
        }
        fail("the player never walked off the 2480 lip");
    }

    void lip_timing(const Services& s)
    {
        const float grace = s.tuning.player().ledge_grace_time;
        const float buffer = s.tuning.player().jump_buffer_time;
        for (const Transition transition : { Transition::none, Transition::pause, Transition::death })
        {
            std::string table;
            int last_grace = 0;
            for (int after = 1; after <= 8; ++after)
            {
                auto run = lip_run(s, transition);
                walk_off(*run);
                for (int tick = 1; tick < after; ++tick) { run->tick(input(false, false, 1.0f)); }
                run->tick(input(true, true, 1.0f));
                const bool jumped = run->count(MovementFeedbackKind::takeoff) == 1;
                // Grace was set on the last supported update; the leaving
                // update and each one to the press count it down.
                const bool expected = window_open(grace, after + 1);
                require(jumped == expected, "a ledge press did not follow the grace window");
                last_grace = jumped ? after : last_grace;
                table += " +" + std::to_string(after) + (jumped ? ":jump" : ":none");
            }
            require(last_grace >= 1, "no ledge grace press succeeded");

            auto reference = lip_run(s, transition);
            const int leave = walk_off(*reference);
            for (int tick = 0; tick < 240 && reference->landings.empty(); ++tick)
            {
                reference->tick(input(false, false, 1.0f));
            }
            require(!reference->landings.empty(), "walking off the lip never landed");
            const int landing = reference->landings.front().tick;
            const std::string surface = reference->landings.front().surface;
            std::string buffered;
            for (int press = std::max(leave + last_grace + 1, landing - 8); press <= landing; ++press)
            {
                auto run = lip_run(s, transition);
                walk_off(*run);
                int launched = -1;
                for (int tick = 0; tick < 240 && run->tick_count < landing + 20; ++tick)
                {
                    run->tick(input(run->tick_count + 1 == press, false, 1.0f));
                    if (launched < 0 && run->count(MovementFeedbackKind::takeoff) == 1) { launched = run->tick_count; }
                }
                const bool expected = window_open(buffer, landing - press);
                require(launched == (expected ? landing : -1),
                    "a falling press did not launch exactly on the landing contact tick");
                buffered += " -" + std::to_string(landing - press) + (launched >= 0 ? ":launch" : ":none");
            }
            std::cout << "MEASURE lip-2480 after " << transition_name(transition)
                << ": press ticks after leaving" << table << " | fall lands on " << surface
                << ", press ticks before landing" << buffered << '\n';
        }
        std::cout << "PASS ledge grace, late press and buffered landing on the authored 2480 lip after none/pause/death\n";
    }

    void spawns(const Services& s)
    {
        const CourseData& course = s.course;
        constexpr int deaths = 3;
        for (const PlayerTeam team : { PlayerTeam::a, PlayerTeam::b })
        {
            const std::vector<Vector2F> points = team == PlayerTeam::a ?
                course.info.team_a_spawns() : course.info.team_b_spawns();
            const Surface& runway = course.named(team == PlayerTeam::a ? "spawn_a_runway" : "spawn_b_runway");
            const float d = team == PlayerTeam::a ? 1.0f : -1.0f;
            for (size_t index = 0; index < points.size(); ++index)
            {
                const Vector2F spawn = points[index];
                Run run(s, spawn, Vector2F::ZERO, WeaponType::sprayer, team, spawn);
                int respawn_ticks = 0;
                for (int life = 0; life <= deaths; ++life)
                {
                    for (int tick = 0; tick < 60; ++tick) { run.tick(input(false, life > 0)); }
                    require(run.support() == runway.name && run.player->rectangle().x == spawn.x &&
                        std::fabs(run.player->rectangle().bottom() - runway.rectangle.top()) < surface_tolerance &&
                        run.player->velocity() == Vector2F::ZERO &&
                        run.player->move_state() == PlayerMoveState::on_ground,
                        "a spawn did not settle on its runway at the authored point");
                    require(run.count(MovementFeedbackKind::takeoff) == 0,
                        "a held jump carried across death into the new life");
                    if (life == deaths) { break; }
                    for (int tick = 0; tick < 20; ++tick) { run.tick(input(tick == 0, true, d)); }
                    run.events.clear();
                    run.kill();
                    const int died = run.tick_count;
                    while (run.tick_count - died < 600 &&
                        (run.player->state() == PlayerState::dead || run.tick_count == died))
                    {
                        run.tick(input(true, true, d, true));
                    }
                    require(run.player->state() == PlayerState::alive, "a spawn never respawned");
                    respawn_ticks = run.tick_count - died;
                    run.events.clear();
                }
                run.tick();
                run.tick(input(true, true));
                require(run.player->move_state() == PlayerMoveState::jumping,
                    "repeated respawns did not rearm the jump");
                std::cout << "MEASURE spawn team_" << (team == PlayerTeam::a ? "a" : "b") << '[' << index
                    << "] x=" << fixed(spawn.x, 0) << ": settled feet=" << fixed(runway.rectangle.top(), 0)
                    << " after " << deaths << " deaths, respawn_ticks=" << respawn_ticks << '\n';
            }
        }
        std::cout << "PASS repeated spawns: every authored spawn settles at feet Y=1000 after repeated deaths with no carried input\n";
    }

    void aim(const Services& s)
    {
        const Surface& lane = s.course.named("recovery_floor");
        // Keyboard/mouse: Level requests the cursor direction on every tick
        // while the window is focused, so aim is re-supplied, not retained.
        {
            Run run(s, standing_on(s, lane, 1000.0f));
            run.settle();
            for (int tick = 0; tick < 160; ++tick)
            {
                const float angle = tick < 120 ? -0.6f : 2.6f;
                PlayerInputData actions = aimed(angle, input(false, false, tick < 60 ? 1.0f : -1.0f));
                actions.primary_shoot = true;
                run.tick(actions);
                require(run.player->primary_weapon().rotation() == angle,
                    "a requested aim changed while moving, reversing or firing");
            }
            require(run.shots > 0 && !run.player->facing_right(),
                "the aim fixture did not fire or reverse facing");
            std::cout << "MEASURE aim keyboard/mouse model: requested every tick, held through 160 ticks of "
                "moving, reversing and firing; shots=" << run.shots << '\n';
        }
        // Gamepad: once an aimed tool receives an explicit direction, the
        // neutral/deadzoned stick must retain it independently of movement.
        for (const WeaponType type : { WeaponType::sprayer, WeaponType::sniper,
            WeaponType::mister, WeaponType::bucket })
        {
            Run run(s, standing_on(s, lane, lane.rectangle.center().x), Vector2F::ZERO, type);
            run.settle();
            run.tick(input(false, false, 1.0f));
            require(run.player->primary_weapon().rotation() == 0.0f,
                "an unaimed weapon did not follow the initial right facing");
            run.tick(input(false, false, -1.0f));
            require(run.player->primary_weapon().rotation() == PI,
                "an unaimed weapon did not follow the initial left facing");
            const Vector2F up_left(-0.7f, -0.7f);
            const float wanted = up_left.angle();
            const auto holds = [&run](float angle)
                { return std::fabs(run.player->primary_weapon().rotation() - angle) < 0.0001f; };
            for (int tick = 0; tick < 30; ++tick)
            {
                PlayerInputData actions = pad_aim(up_left, s.preferences, input(false, false, 1.0f));
                actions.primary_shoot = true;
                run.tick(actions);
                require(holds(wanted), "a held pad aim was not applied while moving and firing");
            }
            const int shots_before_release = run.shots;
            for (int tick = 0; tick < 90; ++tick)
            {
                const Vector2F stick = tick < 30 ? Vector2F(-0.1f, -0.1f) : Vector2F::ZERO;
                PlayerInputData actions = pad_aim(stick, s.preferences,
                    input(false, false, tick < 30 ? 1.0f : -1.0f));
                actions.primary_shoot = true;
                run.tick(actions);
                require(holds(wanted), "released or deadzoned pad aim changed during movement, reversal or firing");
            }
            require(run.shots > shots_before_release && !run.player->facing_right() &&
                run.player->velocity().x < 0.0f,
                "the released-aim fixture did not fire and reverse movement");

            const Vector2F replacement(0.8f, -0.3f);
            const float replaced = replacement.angle();
            run.tick(pad_aim(replacement, s.preferences));
            require(holds(replaced), "a fresh pad aim did not replace the retained direction");
            brake(run);
            require(holds(replaced), "released pad aim changed while coming to rest");

            // These are the player hooks used when Level suspends/resumes;
            // no simulation time passes while the menu owns the match.
            run.player->pause_sounds();
            run.player->reset_jump_input();
            run.player->resume_sounds();
            run.tick();
            require(holds(replaced), "pause and resume discarded retained pad aim");

            run.kill();
            run.tick();
            require(run.player->state() == PlayerState::dead, "the aim fixture did not enter death");
            for (int tick = 0; tick < 400 && run.player->state() == PlayerState::dead; ++tick)
            {
                run.tick();
                require(holds(replaced), "death and respawn discarded retained pad aim");
            }
            require(run.player->state() == PlayerState::alive, "the aim fixture did not respawn");
            run.tick();
            require(holds(replaced), "released pad aim changed in the new life");
            std::cout << "MEASURE aim pad model tool=" << static_cast<int>(type)
                << ": retained=" << fixed(wanted, 3) << " during deadzone/release/movement/reversal/fire"
                << " replaced=" << fixed(replaced, 3) << " after rest/pause/death+respawn"
                << " shots=" << run.shots << '\n';
        }
        {
            Run run(s, standing_on(s, lane, lane.rectangle.center().x), Vector2F::ZERO, WeaponType::roller);
            run.settle();
            for (int tick = 0; tick < 90; ++tick)
            {
                const bool right = tick < 30;
                PlayerInputData actions = pad_aim(tick < 60 ? Vector2F(-0.7f, -0.7f) : Vector2F::ZERO,
                    s.preferences, input(false, false, right ? 1.0f : -1.0f));
                actions.primary_shoot = true;
                run.tick(actions);
                const float wanted = right ? PI / 4.0f : 3.0f * PI / 4.0f;
                require(run.player->primary_weapon().rotation() == wanted,
                    "the roller stopped using its fixed facing-down aim");
            }
            require(run.shots > 0, "the fixed-aim roller fixture never fired");
        }
        std::cout << "PASS aim: every aimed tool retains/replaces pad aim through deadzone, release, movement, reversal, "
            "firing, pause and respawn; initial facing fallback and the roller's fixed aim remain\n";
    }

    float face_paint(const StructurePaintable& surface, bool underside, float thickness)
    {
        const RectangleF r = surface.bounds();
        float area = 0.0f;
        for (const PaintTile& tile : surface.paint_tiles_)
        {
            const RectangleF b = tile.bounds();
            const bool on_face = underside
                ? std::fabs(b.height - thickness) < 0.01f && std::fabs(b.bottom() - r.bottom()) < 0.01f
                : std::fabs(b.width - thickness) < 0.01f && std::fabs(b.left() - r.left()) < 0.01f;
            if (on_face && tile.team() == PlayerTeam::a) { area += tile.area(); }
        }
        return area;
    }

    void painting(const Services& s)
    {
        const Surface& floor = s.course.named("running_landing");
        const Surface& block = s.course.named("low_ceiling");
        const Surface& step = s.course.named("short_step");
        const float thickness = s.tuning.paint().thickness;
        for (const WeaponType type : { WeaponType::sprayer, WeaponType::roller, WeaponType::sniper,
            WeaponType::mister, WeaponType::bucket })
        {
            const char* name = type == WeaponType::sprayer ? "sprayer" : type == WeaponType::roller ? "roller" :
                type == WeaponType::sniper ? "sniper" : type == WeaponType::mister ? "mister" : "bucket";
            // Fire for 90 ticks while sweeping aim from `first` to `last`, then
            // let flying paint settle. Walking into a face lets the roller paint
            // by contact. A muzzle inside cover paints through the launch
            // obstruction instead of flying, so `shots` counts only flyers and
            // `swept` only the face area painted while flying projectiles sweep
            // through Projectile::resolve_movement_contacts.
            struct Painted
            {
                float area = 0.0f;
                float swept = 0.0f;
                int shots = 0;
            };
            const auto station = [&](const Vector2F& top_left, float first, float last, float walk,
                const Surface& target, bool underside)
            {
                Run run(s, top_left, Vector2F::ZERO, type);
                run.settle(5);
                run.watched_paint = [&run, &target, underside, thickness]
                    { return face_paint(run.paintable(target.name), underside, thickness); };
                for (int tick = 0; tick < 240; ++tick)
                {
                    const float angle = first + (last - first) * static_cast<float>(std::min(tick, 89)) / 89.0f;
                    PlayerInputData actions = aimed(angle, input(false, false, tick < 90 ? walk : 0.0f));
                    actions.primary_shoot = tick < 90;
                    run.tick(actions);
                }
                return Painted{ run.watched_paint(), run.swept_paint, run.shots };
            };
            // Underside: from directly beneath (muzzle in contact range), then
            // from outside its left end so the paint has to fly. The ceiling
            // clears a standing head by only 40 units, so a steep straight shot
            // passes over its end. The sniper, which launches once here, stands
            // back past its long muzzle and holds a shallow aim; the bucket's
            // tall, dropping blob needs a steep lob from close outside.
            float outside = 100.0f;
            float first = -1.25f;
            float last = -0.55f;
            if (type == WeaponType::sniper) { outside = 300.0f; first = last = -0.15f; }
            if (type == WeaponType::bucket) { outside = 150.0f; first = last = -0.8f; }
            const auto under = station(standing_on(s, floor, block.rectangle.center().x),
                -PI / 2.0f, -PI / 2.0f, 0.0f, block, true);
            const auto under_far = station(standing_on(s, floor, block.rectangle.left() - outside),
                first, last, 0.0f, block, true);
            // Wall: walk into the short step's left face aiming slightly down,
            // then sweep across it from 200 units away.
            const auto wall = station(front_at(s, floor, step.rectangle.left() - 40.0f, 1.0f),
                0.35f, 0.35f, 1.0f, step, false);
            const auto wall_far = station(front_at(s, floor, step.rectangle.left() - 200.0f, 1.0f),
                -0.3f, 0.3f, 0.0f, step, false);
            const auto report = [](const Painted& painted)
            {
                std::ostringstream text;
                text << fixed(painted.area) << " (swept " << fixed(painted.swept) << ", flying shots "
                    << painted.shots << ")";
                return text.str();
            };
            std::cout << "MEASURE paint " << name << ": low_ceiling underside team_a beneath=" << report(under)
                << " from-outside=" << report(under_far) << " | short_step left face team_a contact="
                << report(wall) << " from-200=" << report(wall_far) << "\n";
            if (type == WeaponType::roller)
            {
                // The roller's aim is fixed forward-down: it paints by contact.
                require(wall.area > 0.0f, "the roller did not paint the step face by contact");
            }
            else
            {
                require(under.area > 0.0f && wall.area > 0.0f,
                    "an aimed tool did not paint the authored ceiling underside and step face");
                // The far stations must paint by flight, not launch obstruction.
                require(under_far.shots > 0 && under_far.swept > 0.0f &&
                    wall_far.shots > 0 && wall_far.swept > 0.0f,
                    "an aimed tool's flying paint did not reach the ceiling underside and step face");
            }
        }
        std::cout << "PASS painting: aimed tools paint the low_ceiling underside and a staircase face, "
            "from close range and by swept flight from outside cover; the roller the face by contact\n";
    }
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "Usage: movement_course.exe <content directory>");
        const std::filesystem::path content(argv[1]);
        RenderResources render;
        for (const char* name : { "sprite_sheet_1", "painter", "weapon_paint", "paint_fx" })
        {
            render.add_sprite_sheet(name, read_sprite_sheet(
                (content / "textures" / (std::string(name) + ".json")).string().c_str(),
                TextureHandle{}));
        }
        AudioResources audio;
        audio.add_sound_bank("paint_exe", SoundBank::silent());
        const PlaySettings preferences;
        const GameTuning tuning((content / "tuning.json").string().c_str());
        const Presentation presentation((content / "presentation.json").string().c_str());
        require(presentation.movement_feedback().enabled,
            "route trials count takeoffs and landings through movement feedback");
        const TeamColour colours;
        const LevelObjectBuilder builder(&render, &audio, &preferences, &tuning.paint());
        const CourseData course(content / "levels" / "movement_course.json");
        const Services services{ tuning, presentation.movement_feedback(),
            presentation.combat_feedback(), presentation.respawn_redraw(), render, audio,
            preferences, builder, colours, course };
        std::cout << "NOTE dt=" << dt << "; every trial builds a fresh authored Scene and steps it in "
            "Level::update_level_logic order (no const_cast placement)\n";

        const std::vector<Gap> routes{
            { "short", &course.named("spawn_a_runway"), &course.named("short_landing_long_takeoff"),
                &course.named("short_gap_recovery_low"), &course.named("short_gap_recovery_high") },
            { "long", &course.named("short_landing_long_takeoff"), &course.named("long_landing_running_takeoff"),
                &course.named("long_gap_recovery_low"), &course.named("long_gap_recovery_high") },
            { "running", &course.named("long_landing_running_takeoff"), &course.named("running_landing"),
                &course.named("running_gap_recovery_low"), &course.named("running_gap_recovery_high") } };
        for (const Gap& gap : routes)
        {
            require(gap.width() > 0.0f, "a Movement Course gap closed");
        }

        from_rest_travel(services);
        analog_movement(services);
        gaps(services, routes);
        recovery(services, routes);
        staircases(services);
        ceiling(services);
        drop_through(services);
        lip_timing(services);
        spawns(services);
        aim(services);
        painting(services);
        std::cout << "PASS Movement Course route trials on authored geometry\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
