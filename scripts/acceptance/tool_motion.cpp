#include "game/pch.h"
#include "game/objects/player.h"
#include "game/objects/structure.h"
#include "game/objects/weapon_builder.h"
#include "game/presentation.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/core/step_timer.h"
#include "engine/scene/scene.h"

#include <array>
#include <filesystem>
#include <iostream>

using namespace labrador;
using namespace mattmath;

namespace
{
    void require(bool condition, const char* message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    const float dt = static_cast<float>(StepTimer::TicksToSeconds(
        StepTimer::SecondsToTicks(1.0 / 60.0)));

    constexpr WeaponType tools[] = { WeaponType::sprayer, WeaponType::sniper,
        WeaponType::roller, WeaponType::mister, WeaponType::bucket };

    auto observation(const Weapon& weapon)
    {
        const ToolMotionState& state = weapon.motion_state();
        return std::array{ static_cast<float>(state.pose), state.pressure, state.valve,
            state.travel_phase, state.lift, state.compression, state.bucket_swing,
            state.mist_tail_remaining, state.hose_sway,
            static_cast<float>(weapon.visual_pose()) };
    }

    PlayerInputData input(bool held = false, float movement = 0.0f,
        float angle = 0.0f)
    {
        PlayerInputData result;
        result.connected = true;
        result.primary_shoot = held;
        result.x_movement = movement;
        result.shoot_direction_requested = true;
        result.shoot_angle = angle;
        result.shoot_direction = Vector2F::unit_vec_from_angle(angle);
        return result;
    }

    struct World
    {
        RenderResources render;
        AudioResources audio;
        PlaySettings preferences;
        GameTuning tuning;
        Presentation presentation;

        explicit World(const std::filesystem::path& content) :
            tuning((content / "tuning.json").string().c_str()),
            presentation((content / "presentation.json").string().c_str())
        {
            for (const char* sheet : { "sprite_sheet_1", "painter", "weapon_paint", "paint_fx" })
            {
                this->render.add_sprite_sheet(sheet, read_sprite_sheet(
                    (content / "textures" / (std::string(sheet) + ".json")).string().c_str(),
                    TextureHandle{}));
            }
            this->audio.add_sound_bank("paint_exe", SoundBank::silent());
        }

        std::unique_ptr<Weapon> weapon(WeaponType type, bool motion = true)
        {
            auto result = WeaponBuilder::build_weapon(&this->tuning, PlayerTeam::a,
                0, 0, Colour::white, type, Vector2F::ZERO,
                &this->render, &this->audio, &this->preferences);
            result->set_motion_presentation(motion ? &this->presentation.tool_motion() : nullptr);
            return result;
        }
    };

    auto update(Weapon& weapon, const PlayerInputData& actions, float elapsed = dt,
        const Vector2F& position = Vector2F::ZERO,
        const Vector2F& velocity = Vector2F::ZERO)
    {
        return weapon.update_and_get_projectiles(actions, position, velocity,
            actions.x_movement >= 0.0f, elapsed);
    }

    void prime(Weapon& weapon, const GameTuning& tuning)
    {
        const float interval = tuning.weapon(weapon.type()).details.shoot_interval;
        for (int tick = 0; static_cast<float>(tick) * dt <= interval + dt; ++tick)
        {
            require(update(weapon, input()).empty(), "Released tool unexpectedly fired while priming");
        }
        require(weapon.ready_to_shoot(), "Tool was not ready after its configured shot interval");
    }

    void check_frozen_updates(World& world)
    {
        for (const WeaponType type : tools)
        {
            auto weapon = world.weapon(type);
            const auto initial = observation(*weapon);
            prime(*weapon, world.tuning);
            require(!update(*weapon, input(true), 0.0f).empty() &&
                observation(*weapon) == initial,
                "Zero-delta paid shot advanced tool motion");
            prime(*weapon, world.tuning);
            require(!update(*weapon, input(true, 1.0f)).empty(), "Frozen fixture did not pay a shot");
            weapon->update_resolved_motion(true, true, 7.0f, dt);
            const auto before = observation(*weapon);
            for (const float frozen : { 0.0f, -1.0f })
            {
                std::ignore = update(*weapon, input(false, -1.0f), frozen);
                weapon->update_resolved_motion(false, true, 1000.0f, frozen);
                require(observation(*weapon) == before,
                    "Nonpositive delta advanced or emitted tool secondary motion");
            }
            // This is the actual respawn hook, rather than only the motion
            // reset helper. Infinite-paint refills use reset_ammo(false).
            weapon->reset_ammo();
            const auto reset = observation(*weapon);
            weapon->reset_motion();
            require(reset == initial && observation(*weapon) == reset &&
                weapon->ammo() == world.tuning.starting_ammo() && weapon->visual_pose() == 0,
                "Tool reset did not settle idempotently into its idle pose");
        }
        std::cout << "PASS tool motion: frozen deltas and idempotent reset for all five tools\n";
    }

    void check_paid_shot_edges(World& world)
    {
        for (const WeaponType type : { WeaponType::sprayer, WeaponType::mister })
        {
            auto weapon = world.weapon(type);
            prime(*weapon, world.tuning);
            std::array<bool, 4> seen{};
            for (int tick = 0; tick < 90; ++tick)
            {
                std::ignore = update(*weapon, input(true));
                seen.at(static_cast<size_t>(weapon->visual_pose())) = true;
            }
            require(seen[1] && seen[2],
                "Continuous stream firing never showed both pressure/valve mechanism poses");
        }
        for (const WeaponType type : { WeaponType::sniper, WeaponType::bucket })
        {
            auto weapon = world.weapon(type);
            prime(*weapon, world.tuning);
            require(!update(*weapon, input(true)).empty(), "Precision tool did not pay its primed shot");
            const float first = weapon->motion_state().pressure;
            require(first > 0.0f, "Paid precision shot did not start its pressure-release motion");
            float previous = first;
            bool decayed = false;
            const float interval = world.tuning.weapon(type).details.shoot_interval;
            for (int tick = 0; static_cast<float>(tick + 2) * dt < interval; ++tick)
            {
                require(update(*weapon, input(true)).empty(), "Precision tool fired during its cooldown");
                const float pressure = weapon->motion_state().pressure;
                require(pressure <= previous,
                    "Held fire retriggered pressure motion without a paid precision shot");
                decayed = decayed || pressure < previous;
                previous = pressure;
            }
            require(decayed, "Precision-shot pressure never recovered during its existing cooldown");
        }

        auto dry_mister = world.weapon(WeaponType::mister);
        require(update(*dry_mister, input(true)).empty(),
            "Dry mist fixture unexpectedly bypassed its initial shot interval");
        require(update(*dry_mister, input(false)).empty() &&
            dry_mister->motion_state().mist_tail_remaining == 0.0f,
            "Initial held request produced release mist before any paid shot");

        auto mister = world.weapon(WeaponType::mister);
        prime(*mister, world.tuning);
        require(!update(*mister, input(true)).empty(), "Mist fixture did not fire");
        std::ignore = update(*mister, input(false));
        float tail = mister->motion_state().mist_tail_remaining;
        require(tail > 0.0f, "Releasing the mister omitted its cosmetic mist tail");
        for (int tick = 0; tick < 120 && tail > 0.0f; ++tick)
        {
            require(update(*mister, input(false)).empty(), "Cosmetic mist tail emitted gameplay projectiles");
            const float next = mister->motion_state().mist_tail_remaining;
            require(next < tail, "Released mist tail restarted or failed to dissipate");
            tail = next;
        }
        require(tail == 0.0f, "Released mist tail exceeded its bounded lifetime");
        std::cout << "PASS tool motion: paid-shot pressure edges and non-projectile mist release tail\n";
    }

    void compare_projectiles(std::vector<std::unique_ptr<Projectile>>& animated,
        std::vector<std::unique_ptr<Projectile>>& disabled)
    {
        require(animated.size() == disabled.size(), "Tool motion changed gameplay volley size or cadence");
        for (size_t index = 0; index < animated.size(); ++index)
        {
            auto& first = *animated[index];
            auto& second = *disabled[index];
            require(first.player_num() == second.player_num() && first.tag() == second.tag(),
                "Tool motion changed projectile ownership or kind");
            // Observe the real launch volume and subsequent integrator, rather
            // than copying any tool-specific velocity or trajectory calculation.
            for (int travel = 0; travel < 6; ++travel)
            {
                require(first.shape()->bounding_box() == second.shape()->bounding_box() &&
                    first.bounds() == second.bounds() && first.for_deletion() == second.for_deletion(),
                    "Tool motion changed projectile launch position, travel or lifetime");
                first.update(dt);
                second.update(dt);
            }
        }
    }

    void check_cosmetic_equivalence(World& world)
    {
        ToolMotionPresentation disabled_look = world.presentation.tool_motion();
        disabled_look.enabled = false;
        for (const WeaponType type : tools)
        {
            auto animated = world.weapon(type);
            auto disabled = world.weapon(type, false);
            disabled->set_motion_presentation(&disabled_look);
            bool changed = false;
            size_t shots = 0;
            const auto initial = observation(*animated);
            for (int tick = 0; tick < 1200; ++tick)
            {
                const float movement = tick % 180 < 90 ? 1.0f : -1.0f;
                const float angle = PI * static_cast<float>((tick / 15) % 8 - 4) / 4.0f;
                const auto actions = input(tick % 240 < 150, movement, angle);
                const Vector2F center(static_cast<float>(tick % 71), static_cast<float>(tick % 37));
                const Vector2F velocity(movement * 300.0f, static_cast<float>(tick % 5) * 40.0f);
                auto first = update(*animated, actions, dt, center, velocity);
                auto second = update(*disabled, actions, dt, center, velocity);
                shots += first.empty() ? 0 : 1;
                compare_projectiles(first, second);
                const bool supported = tick % 90 < 60;
                animated->update_resolved_motion(supported, true, movement * 5.0f, dt);
                disabled->update_resolved_motion(supported, true, movement * 5.0f, dt);
                changed = changed || observation(*animated) != initial;
                require(animated->ammo() == disabled->ammo() &&
                    animated->ready_to_shoot() == disabled->ready_to_shoot() &&
                    animated->rotation() == disabled->rotation() &&
                    animated->nozzle_pos() == disabled->nozzle_pos(),
                    "Tool secondary motion changed ammunition, shot readiness, aim or the launch muzzle");
            }
            require(changed && shots > 0, "Cosmetic equivalence sequence did not exercise motion and firing");
        }
        std::cout << "PASS tool motion: enabled/disabled cosmetics preserve five tools' ammo, cadence, muzzle and projectile travel\n";
    }

    // The same movement -> weapon -> resolve -> end_contacts ordering as Level.
    // Real geometry distinguishes actual supported travel from a velocity request
    // into a wall and from airborne displacement. No renderer/device/save is used.
    struct Course
    {
        Scene scene{ nullptr, nullptr };
        Player* player;

        explicit Course(World& world)
        {
            const RectangleF body(0, -world.tuning.player().size.y,
                world.tuning.player().size.x, world.tuning.player().size.y);
            auto weapon = world.weapon(WeaponType::roller);
            this->player = this->scene.add(std::make_unique<Player>(body, &world.tuning,
                &world.presentation.movement_feedback(), &world.presentation.combat_feedback(), &world.presentation.respawn_redraw(),
                &world.render, &world.audio, &world.preferences, 0, PlayerTeam::a,
                WeaponType::roller, std::move(weapon), WeaponType::none, Colour::white,
                Viewport{}, body.position()));
            for (const auto rectangle : { RectangleF(-1000, 0, 3000, 100),
                RectangleF(400, -4000, 100, 4000) })
            {
                this->scene.add(std::make_unique<Structure>("sprite_sheet_1", "square_white_4",
                    rectangle, std::make_unique<RectangleF>(rectangle), &world.render,
                    CollisionObjectType::structure));
            }
            this->scene.end_tick();
        }

        void tick(const PlayerInputData& actions = input())
        {
            this->player->set_player_input(actions);
            this->scene.update(dt);
            std::ignore = this->player->update_weapon_and_get_projectiles(dt,
                this->scene.collision_objects());
            this->scene.resolve();
            this->player->end_contacts(this->scene.contacts());
            this->player->update_prev_rectangle();
            this->scene.end_tick();
        }
    };

    void check_roller_contacts_and_lifecycle(World& world)
    {
        auto roller = world.weapon(WeaponType::roller);
        const float initial = roller->motion_state().travel_phase;
        roller->update_resolved_motion(false, true, 10.0f, dt);
        require(roller->motion_state().travel_phase == initial,
            "Unsupported roller travel rotated the drum");
        roller->update_resolved_motion(true, true, 0.0f, dt);
        require(roller->motion_state().travel_phase == initial,
            "Stationary supported roller rotated the drum");
        roller->update_resolved_motion(true, false, 7.0f, dt);
        require(roller->motion_state().travel_phase == initial,
            "Spawn-settling separation rotated the roller drum");
        roller->update_resolved_motion(true, true, 7.0f, dt);
        require(roller->motion_state().travel_phase != initial,
            "Actual supported roller travel did not rotate the drum");
        roller->update_resolved_motion(true, true, -7.0f, dt);
        require(std::abs(roller->motion_state().travel_phase - initial) < 0.000001f,
            "Reversing supported travel failed to reverse the roller drum");

        Course course(world);
        for (int tick = 0; tick < 5; ++tick) { course.tick(); }
        const float rest_phase = course.player->primary_weapon().motion_state().travel_phase;
        bool rolled = false;
        for (int tick = 0; tick < 150; ++tick)
        {
            course.tick(input(false, 1.0f));
            rolled = rolled || course.player->primary_weapon().motion_state().travel_phase != rest_phase;
        }
        require(rolled, "Player contact phase never delivered supported travel to its roller");
        const float at_wall = course.player->rectangle().x;
        const float wall_phase = course.player->primary_weapon().motion_state().travel_phase;
        for (int tick = 0; tick < 30; ++tick)
        {
            course.tick(input(false, 1.0f));
            require(course.player->rectangle().x == at_wall &&
                course.player->primary_weapon().motion_state().travel_phase == wall_phase,
                "Walking into an immovable wall rotated the drum without resolved travel");
        }
        auto jump = input(false, -1.0f);
        jump.jump_pressed = true;
        jump.jump_held = true;
        course.tick(jump);
        require(course.player->move_state() == PlayerMoveState::jumping,
            "Roller airborne fixture did not jump");
        const float air_phase = course.player->primary_weapon().motion_state().travel_phase;
        jump.jump_pressed = false;
        for (int tick = 0; tick < 8; ++tick)
        {
            course.tick(jump);
            require(course.player->primary_weapon().motion_state().travel_phase == air_phase,
                "Airborne Player movement rotated a lifted roller drum");
        }
        course.player->kill();
        course.tick();
        auto fresh = world.weapon(WeaponType::roller);
        require(course.player->state() == PlayerState::dead &&
            observation(course.player->primary_weapon()) == observation(*fresh),
            "Elimination retained an active roller motion or release tail");
        course.player->force_respawn();
        const auto reset = observation(course.player->primary_weapon());
        require(reset == observation(*fresh) &&
            course.player->weapon_ammo() == world.tuning.starting_ammo(),
            "Respawn retained old roller secondary motion or failed to restore paint");
        std::cout << "PASS tool motion: actual supported travel, blocked/airborne movement and respawn reset\n";
    }
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "Usage: tool_motion.exe <content directory>");
        World world{ std::filesystem::path(argv[1]) };
        check_frozen_updates(world);
        check_paid_shot_edges(world);
        check_cosmetic_equivalence(world);
        check_roller_contacts_and_lifecycle(world);
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL tool motion: " << error.what() << '\n';
        return 1;
    }
}
